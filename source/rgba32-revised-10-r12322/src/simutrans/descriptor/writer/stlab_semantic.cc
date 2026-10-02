/*
 * STLAB-RGBA32-END-TO-END-MAKEOBJ-RUNTIME-01 -- semantic extractor for makeobj.
 * NOT FOR COMMIT. Laboratory only.
 *
 * A mechanical port of probe.py::reconstruct() and records.py::build(). Every fidelity
 * decision is written down beside the code it governs, because the acceptance gate of this
 * mission is exact equality with the Python oracle, record for record.
 */
#include "stlab_semantic.h"

#include <algorithm>
#include <cfenv>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "../image.h"

stlab_semantic_counters_t stlab_semantic_counters;

/* ---------------------------------------------------------------- CONFIG (probe.py) --
 * Copied verbatim from CONFIG in tools/probe.py. Changing any of these changes what the
 * oracle means, so they are not tunable here.
 */
static const double CFG_ALPHA_MIN               = 0.04;
static const double CFG_ALPHA_MAX               = 0.96;
static const double CFG_RESIDUAL_CLEAN          = 2.0;
static const double CFG_RESIDUAL_WITHERROR      = 6.0;
static const double CFG_RESIDUAL_SLACK          = 0.5;
static const double CFG_COMPLEX_UNDER_SEPARATION = 16.0;
static const double CFG_RIVAL_DNORM_FRACTION    = 0.8;

/* The 8-neighbourhood, in the order probe.py builds it:
 *     NB = [(dy, dx) for dy in (-1, 0, 1) for dx in (-1, 0, 1) if (dy, dx) != (0, 0)]
 * The order is load-bearing: it fixes the insertion order of the M and N candidate sets,
 * which in turn fixes the tie-breaks below. */
static const int NB[8][2] = {
	{ -1, -1 }, { -1, 0 }, { -1, 1 },
	{  0, -1 },            {  0, 1 },
	{  1, -1 }, {  1, 0 }, {  1, 1 }
};


bool stlab_writer_enabled()
{
	static int cached = -1;
	if(  cached < 0  ) {
		const char *e = getenv("STLAB_XR32_WRITE");
		cached = (e != NULL  &&  e[0] == '1') ? 1 : 0;
	}
	return cached == 1;
}


stlab_taxonomy_t stlab_taxonomy_of(stlab_solver_class_t c)
{
	switch(  c  ) {
		case STLAB_CLEANLY_RECONSTRUCTABLE:  return STLAB_CLEAN;
		case STLAB_RECONSTRUCTABLE_WITH_ERROR:
		case STLAB_COMPLEX_INTERNAL_BLEND:   return STLAB_AMBIGUOUS;
		default:                             return STLAB_UNRECOVERABLE;
	}
}


const char *stlab_solver_class_name(stlab_solver_class_t c)
{
	static const char *N[] = { "CLEANLY_RECONSTRUCTABLE", "RECONSTRUCTABLE_WITH_ERROR",
	                           "COMPLEX_INTERNAL_BLEND", "NOT_AN_EDGE",
	                           "NOT_RECONSTRUCTABLE", "AMBIGUOUS" };
	return (c < STLAB_SOLVER_CLASS_COUNT) ? N[c] : "?";
}


const char *stlab_taxonomy_name(stlab_taxonomy_t t)
{
	static const char *N[] = { "CLEAN", "AMBIGUOUS", "UNRECOVERABLE" };
	return (t < STLAB_TAXONOMY_COUNT) ? N[t] : "?";
}


/**
 * Python's round() on a float is round-half-to-even, not round-half-away-from-zero.
 * nearbyint() under the default FE_TONEAREST is exactly that, so the two agree on the
 * halfway cases that a naive (int)(x + 0.5) would get wrong.
 */
static inline int py_round(double v)
{
	return (int)std::nearbyint(v);
}


namespace {

struct rgb_t {
	uint8 r, g, b;
	bool operator==(const rgb_t &o) const { return r == o.r && g == o.g && b == o.b; }
};

struct fit_t {
	double res;
	double alpha;      ///< raw, unclamped
	double alpha_c;    ///< clamped to [0,1]
	double dnorm;      ///< Chebyshev norm of M-N
	rgb_t  M;
	int    mi;
	rgb_t  N;
	int    order;      ///< insertion order, so the sort can be made stable explicitly
};

/// an ordered set with Python dict semantics: first insertion fixes the position, a later
/// assignment overwrites the value but does not move the key
struct ordered_markers_t {
	std::vector<rgb_t> keys;
	std::vector<int>   vals;
	void put(const rgb_t &k, int v)
	{
		for(  size_t i = 0;  i < keys.size();  i++  ) {
			if(  keys[i] == k  ) { vals[i] = v; return; }
		}
		keys.push_back(k);
		vals.push_back(v);
	}
};

struct ordered_unders_t {
	std::vector<rgb_t> keys;
	void put(const rgb_t &k)
	{
		for(  size_t i = 0;  i < keys.size();  i++  ) {
			if(  keys[i] == k  ) return;
		}
		keys.push_back(k);
	}
};

} // namespace


/* SPECIAL_TRANSPARENT, and the alpha rule probe.py::masks() uses:
 *     hole   = (packed == 0xE7FFFF) | (alpha <= 7)
 *     opaque = (~hole) & (alpha == 255)
 * Note that a pixel with 7 < alpha < 255 is neither hole nor opaque: it takes part in
 * nothing, exactly as in the oracle. */
#define STLAB_SPECIAL_TRANSPARENT (0x00E7FFFFu)


void stlab_extract_semantics(const uint8 *rgba, int w, int h,
                             std::vector<stlab_semantic_record_t> &out,
                             std::vector<stlab_edge_candidate_t> *edges)
{
	out.clear();
	if(  edges  ) edges->clear();
	if(  w <= 0  ||  h <= 0  ) {
		return;
	}
	const size_t npx = (size_t)w * h;

	std::vector<uint8> hole(npx, 0), opaque(npx, 0);
	std::vector<sint16> midx(npx, -1);

	for(  size_t i = 0;  i < npx;  i++  ) {
		const uint8 *p = rgba + i * 4;
		const uint32 packed = ((uint32)p[0] << 16) | ((uint32)p[1] << 8) | (uint32)p[2];
		const uint8 a = p[3];
		hole[i] = (packed == STLAB_SPECIAL_TRANSPARENT  ||  a <= 7) ? 1 : 0;
		opaque[i] = (!hole[i]  &&  a == 255) ? 1 : 0;
		// probe.py assigns midx marker by marker in table order, so a duplicate RGB would
		// leave the LAST index. image_t::rgbtab has no duplicates, but the loop keeps the
		// same rule rather than relying on that.
		for(  int m = 0;  m < SPECIAL;  m++  ) {
			if(  image_t::rgbtab[m] == packed  ) {
				midx[i] = (sint16)m;
			}
		}
	}

	// ---- 1. exact marker pixels, coverage 255 (records.py step 1) -------------------
	// Order: records.py fills a dict keyed by linear index and sorts at the end, so the
	// emission order is by position. The same is achieved here by walking in order.
	for(  size_t i = 0;  i < npx;  i++  ) {
		if(  !opaque[i]  ||  midx[i] < 0  ) continue;
		const int m = midx[i];
		stlab_semantic_record_t r;
		r.position = (uint32)i;
		if(  m < 8  )        { r.semantic_class = STLAB_CLASS_PLAYER_1; r.semantic_parameter = (uint8)m; }
		else if(  m < 16  )  { r.semantic_class = STLAB_CLASS_PLAYER_2; r.semantic_parameter = (uint8)(m - 8); }
		else                 { r.semantic_class = STLAB_CLASS_LIGHT;    r.semantic_parameter = (uint8)(m - 16); }
		r.coverage_alpha8 = 255;
		r.under_r = r.under_g = r.under_b = 0;
		r.has_under = false;
		out.push_back(r);
	}

	// ---- 2. the edge candidates (probe.py::reconstruct) -----------------------------
	// cand = dilate(player markers) & opaque & (midx < 0). Only PLAYER markers dilate:
	// LIGHT edge semantics are NOT certified and are deliberately not produced here.
	std::vector<uint8> pm(npx, 0);
	for(  size_t i = 0;  i < npx;  i++  ) {
		pm[i] = (opaque[i]  &&  midx[i] >= 0  &&  midx[i] < 16) ? 1 : 0;
	}
	bool any_pm = false;
	for(  size_t i = 0;  i < npx  &&  !any_pm;  i++  ) any_pm = pm[i] != 0;

	std::vector<stlab_semantic_record_t> edge_records;

	if(  any_pm  ) {
		for(  int y = 0;  y < h;  y++  ) {
			for(  int x = 0;  x < w;  x++  ) {
				const size_t i = (size_t)y * w + x;
				if(  !opaque[i]  ||  midx[i] >= 0  ) continue;

				bool dil = false;
				for(  int k = 0;  k < 8  &&  !dil;  k++  ) {
					const int yy = y + NB[k][0], xx = x + NB[k][1];
					if(  yy < 0  ||  yy >= h  ||  xx < 0  ||  xx >= w  ) continue;
					dil = pm[(size_t)yy * w + xx] != 0;
				}
				if(  !dil  ) continue;

				const uint8 *ep = rgba + i * 4;
				const double E[3] = { (double)ep[0], (double)ep[1], (double)ep[2] };

				ordered_markers_t Ms;
				ordered_unders_t  Ns;
				bool touches_hole = false;
				for(  int k = 0;  k < 8;  k++  ) {
					const int yy = y + NB[k][0], xx = x + NB[k][1];
					if(  yy < 0  ||  yy >= h  ||  xx < 0  ||  xx >= w  ) continue;
					const size_t j = (size_t)yy * w + xx;
					if(  hole[j]  ) { touches_hole = true; continue; }
					if(  !opaque[j]  ) continue;
					const uint8 *q = rgba + j * 4;
					const rgb_t key = { q[0], q[1], q[2] };
					if(  midx[j] >= 0  &&  midx[j] < 16  ) Ms.put(key, midx[j]);
					else                                   Ns.put(key);
				}
				if(  Ms.keys.empty()  ) continue;

				// least squares over the three channels, exactly the CONFIG formula:
				//     alpha = <E-N, M-N> / <M-N, M-N>
				std::vector<fit_t> fits;
				fits.reserve(Ms.keys.size() * Ns.keys.size());
				int ord = 0;
				for(  size_t a = 0;  a < Ms.keys.size();  a++  ) {
					const rgb_t &M = Ms.keys[a];
					const double Md[3] = { (double)M.r, (double)M.g, (double)M.b };
					for(  size_t b = 0;  b < Ns.keys.size();  b++  ) {
						const rgb_t &N = Ns.keys[b];
						const double Nd[3] = { (double)N.r, (double)N.g, (double)N.b };
						const double d[3] = { Md[0] - Nd[0], Md[1] - Nd[1], Md[2] - Nd[2] };
						const double dd = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
						if(  dd < 1e-9  ) continue;
						const double num = (E[0] - Nd[0]) * d[0] + (E[1] - Nd[1]) * d[1]
						                 + (E[2] - Nd[2]) * d[2];
						const double al = num / dd;
						const double ac = al < 0.0 ? 0.0 : (al > 1.0 ? 1.0 : al);
						double res = 0.0, dnorm = 0.0;
						for(  int c = 0;  c < 3;  c++  ) {
							const double pred = Nd[c] + ac * d[c];
							res   = std::max(res,   std::fabs(E[c] - pred));
							dnorm = std::max(dnorm, std::fabs(d[c]));
						}
						fit_t f;
						f.res = res; f.alpha = al; f.alpha_c = ac; f.dnorm = dnorm;
						f.M = M; f.mi = Ms.vals[a]; f.N = N; f.order = ord++;
						fits.push_back(f);
					}
				}

				stlab_edge_candidate_t rec;
				rec.y = (uint32)y; rec.x = (uint32)x;
				rec.er = ep[0]; rec.eg = ep[1]; rec.eb = ep[2];
				rec.touches_hole = touches_hole;
				rec.has_rival = false;
				rec.rival_r = rec.rival_g = rec.rival_b = 0;

				if(  fits.empty()  ) {
					// probe.py: "no opaque under-colour in the neighbourhood"; it records
					// the FIRST marker of Ms and no under-colour at all.
					rec.cls = STLAB_NO_UNDER_COLOUR;
					rec.marker_index = Ms.vals[0];
					rec.residual = -1.0;
					rec.alpha_raw = rec.alpha_clamped = -1.0;
					rec.coverage = -1;
					rec.has_under = false;
					rec.under_r = rec.under_g = rec.under_b = 0;
					if(  edges  ) edges->push_back(rec);
					stlab_semantic_counters.unrecoverable_edges++;
					continue;
				}

				// fits.sort(key=res) in Python is STABLE, so equal residuals keep the
				// insertion order fixed by the neighbourhood scan above.
				std::stable_sort(fits.begin(), fits.end(),
				                 [](const fit_t &a, const fit_t &b) { return a.res < b.res; });

				const double rmin = fits[0].res;
				size_t near_end = 0;
				while(  near_end < fits.size()  &&  fits[near_end].res <= rmin + CFG_RESIDUAL_SLACK  ) {
					near_end++;
				}
				// max(near, key=dnorm): Python's max returns the FIRST maximum, so the
				// comparison must be strict.
				size_t bi = 0;
				for(  size_t k = 1;  k < near_end;  k++  ) {
					if(  fits[k].dnorm > fits[bi].dnorm  ) bi = k;
				}
				const fit_t &best = fits[bi];

				// is the under-colour determined? First rival wins, iterating in order.
				const fit_t *rival = NULL;
				for(  size_t k = 0;  k < near_end;  k++  ) {
					if(  k == bi  ||  fits[k].dnorm < best.dnorm * CFG_RIVAL_DNORM_FRACTION  ) continue;
					const double sep = std::max(std::max(
						std::fabs((double)fits[k].N.r - best.N.r),
						std::fabs((double)fits[k].N.g - best.N.g)),
						std::fabs((double)fits[k].N.b - best.N.b));
					if(  sep >= CFG_COMPLEX_UNDER_SEPARATION  ) { rival = &fits[k]; break; }
				}

				// the class test uses the RAW alpha, not the clamped one
				const bool a_ok = (CFG_ALPHA_MIN <= best.alpha  &&  best.alpha <= CFG_ALPHA_MAX);
				if(  !a_ok  &&  best.res <= CFG_RESIDUAL_WITHERROR  ) {
					rec.cls = STLAB_NOT_AN_EDGE;
				}
				else if(  best.res > CFG_RESIDUAL_WITHERROR  ) {
					rec.cls = STLAB_NOT_RECONSTRUCTABLE;
				}
				else if(  rival != NULL  ) {
					rec.cls = STLAB_COMPLEX_INTERNAL_BLEND;
				}
				else if(  best.res <= CFG_RESIDUAL_CLEAN  ) {
					rec.cls = STLAB_CLEANLY_RECONSTRUCTABLE;
				}
				else {
					rec.cls = STLAB_RECONSTRUCTABLE_WITH_ERROR;
				}

				rec.marker_index = best.mi;
				rec.residual = best.res;
				rec.alpha_raw = best.alpha;
				rec.alpha_clamped = best.alpha_c;
				rec.coverage = py_round(best.alpha_c * 255.0);
				rec.has_under = true;
				rec.under_r = best.N.r; rec.under_g = best.N.g; rec.under_b = best.N.b;
				if(  rival  ) {
					rec.has_rival = true;
					rec.rival_r = rival->N.r; rec.rival_g = rival->N.g; rec.rival_b = rival->N.b;
				}
				if(  edges  ) edges->push_back(rec);

				switch(  stlab_taxonomy_of(rec.cls)  ) {
					case STLAB_CLEAN:       stlab_semantic_counters.clean_semantic_edges++; break;
					case STLAB_AMBIGUOUS:   stlab_semantic_counters.ambiguous_edges++;      break;
					default:                stlab_semantic_counters.unrecoverable_edges++;  break;
				}

				// records.py step 2: only the three usable classes, only with an
				// under-colour, only with 0 < coverage < 255.
				const bool usable = (rec.cls == STLAB_CLEANLY_RECONSTRUCTABLE
				                  || rec.cls == STLAB_RECONSTRUCTABLE_WITH_ERROR
				                  || rec.cls == STLAB_COMPLEX_INTERNAL_BLEND);
				if(  usable  &&  rec.coverage > 0  &&  rec.coverage < 255  ) {
					stlab_semantic_record_t s;
					s.position = (uint32)i;
					s.semantic_class = (best.mi < 8) ? STLAB_CLASS_PLAYER_1 : STLAB_CLASS_PLAYER_2;
					s.semantic_parameter = (uint8)(best.mi % 8);
					s.coverage_alpha8 = (uint8)rec.coverage;
					s.under_r = best.N.r; s.under_g = best.N.g; s.under_b = best.N.b;
					s.has_under = true;
					edge_records.push_back(s);
				}
			}
		}
	}

	// records.py builds one dict and sorts by linear index at the end; an edge record
	// replaces a marker record at the same position, but the candidate set excludes marker
	// pixels, so the two sets are disjoint by construction.
	out.insert(out.end(), edge_records.begin(), edge_records.end());
	std::stable_sort(out.begin(), out.end(),
	                 [](const stlab_semantic_record_t &a, const stlab_semantic_record_t &b) {
		                 return a.position < b.position;
	                 });

	for(  size_t i = 0;  i < out.size();  i++  ) {
		switch(  out[i].semantic_class  ) {
			case STLAB_CLASS_PLAYER_1: stlab_semantic_counters.player1_records++; break;
			case STLAB_CLASS_PLAYER_2: stlab_semantic_counters.player2_records++; break;
			default:                   stlab_semantic_counters.light_marker_records++; break;
		}
		if(  out[i].coverage_alpha8 == 255  ) stlab_semantic_counters.exact_marker_records++;
	}
	stlab_semantic_counters.semantic_records_total += (uint32)out.size();
	stlab_semantic_counters.rgba32_images_seen++;
}
