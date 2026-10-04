/*
 * XR32-SEMANTIC-MASK-PIPELINE-01 -- explicit semantic masks for makeobj.
 * NOT FOR COMMIT. Laboratory only. See stlab_semantic_masks.h for the contract.
 *
 * The maths, per pixel and channel, in linear light (L = sRGB decode, integer tables):
 *
 *     base  B = SUM c_i * S_i + (1 - SUM c_i) * U          (what the exporter rendered)
 *     =>    U = (B - SUM c_i * S_i) / (1 - SUM c_i)         (exact algebra, nothing guessed)
 *
 * c_i are the mask values, S_i the authoring colours of the semantic ids. U is clamped to
 * [0, 1], stored as sRGB8, and the pixel is recomposed to check that base and masks agree.
 * Everything is integer arithmetic over stlab_xr32_linear.h, so every platform writes the
 * same bytes.
 */
#include "stlab_semantic_masks.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../simdebug.h"
#include "../../io/raw_image.h"
#include "../image.h"
#include "../../display/stlab_xr32_linear.h"
#include "../../utils/searchfolder.h"
#include "../../utils/simstring.h"


const stlab_sem_vocab_t stlab_sem_vocab[STLAB_SEM_ID_COUNT] = {
	{  0, "pc1-0" }, {  1, "pc1-1" }, {  2, "pc1-2" }, {  3, "pc1-3" },
	{  4, "pc1-4" }, {  5, "pc1-5" }, {  6, "pc1-6" }, {  7, "pc1-7" },
	{  8, "pc2-0" }, {  9, "pc2-1" }, { 10, "pc2-2" }, { 11, "pc2-3" },
	{ 12, "pc2-4" }, { 13, "pc2-5" }, { 14, "pc2-6" }, { 15, "pc2-7" },
	{ 16, "window-dark" },  { 17, "window-light" },
	{ 18, "lamp-yellow" },  { 19, "lamp-red" },     { 20, "lamp-green" },
	{ 21, "fixed-6b" },     { 22, "fixed-9b" },     { 23, "fixed-b3" },
	{ 24, "fixed-c9" },     { 25, "fixed-df" },
	{ 26, "window-e3" },    { 27, "window-c1" },    { 28, "window-4d" },
	{ 29, "lamp-purple" },  { 30, "lamp-blue" }
};

stlab_sem_counters_t stlab_sem_counters;

static bool requested = false;

void stlab_semantic_masks_set_requested(bool on) { requested = on; }
bool stlab_semantic_masks_requested() { return requested; }


static uint32 key_of(uint8 id)
{
	return image_t::rgbtab[id] & 0x00FFFFFFu;
}


/// load one mask as coverage 0..255; false if the file does not exist
static bool load_mask(const std::string &fname, int w, int h, std::vector<uint8> &out)
{
	FILE *probe = fopen(fname.c_str(), "rb");
	if(  probe == NULL  ) {
		return false;
	}
	fclose(probe);

	raw_image_t img;
	if(  !img.read_from_file(fname.c_str())  ) {
		dbg->fatal("stlab_semantic_masks", "cannot read semantic mask '%s'", fname.c_str());
	}
	if(  (int)img.get_width() != w  ||  (int)img.get_height() != h  ) {
		dbg->fatal("stlab_semantic_masks", "semantic mask '%s' is %ux%u, its sheet is %dx%d",
		           fname.c_str(), img.get_width(), img.get_height(), w, h);
	}
	out.assign((size_t)w * h, 0);
	for(  int y = 0;  y < h;  y++  ) {
		for(  int x = 0;  x < w;  x++  ) {
			const uint8 *p = img.access_pixel(x, y);
			uint8 v = 0;
			switch(  img.get_format()  ) {
				case raw_image_t::FMT_GRAY8:
					v = p[0];
					break;
				case raw_image_t::FMT_RGB888:
				case raw_image_t::FMT_RGBA8888:
					if(  p[0] != p[1]  ||  p[1] != p[2]  ) {
						dbg->fatal("stlab_semantic_masks", "semantic mask '%s' is not grey at (%d,%d): "
						           "a mask is coverage, not colour", fname.c_str(), x, y);
					}
					v = p[0];
					if(  img.get_format() == raw_image_t::FMT_RGBA8888  ) {
						// painted on a transparent layer: transparent means no coverage
						v = (uint8)(((uint32)v * p[3] + 127) / 255);
					}
					break;
				default:
					dbg->fatal("stlab_semantic_masks", "semantic mask '%s' has an unsupported format", fname.c_str());
			}
			out[(size_t)y * w + x] = v;
		}
	}
	return true;
}


bool stlab_semantic_masks_solve(const std::string &sheet_png, const uint8 *rgba, int w, int h,
                                std::vector<uint8> &under, std::vector<stlab_sem_contrib_t> &contribs)
{
	static bool report_registered = false;
	if(  !report_registered  ) {
		report_registered = true;
		const char *e = getenv("STLAB_SEM_REPORT");
		if(  e != NULL  &&  e[0] == '1'  ) {
			atexit(stlab_semantic_masks_report);
		}
	}
	stlab_sem_counters.sheets_opted_in++;
	contribs.clear();

	const size_t dot = sheet_png.rfind(".png");
	const std::string stem = (dot == std::string::npos) ? sheet_png : sheet_png.substr(0, dot);

	std::vector<uint8> masks[STLAB_SEM_ID_COUNT];
	bool any = false;
	for(  int i = 0;  i < STLAB_SEM_ID_COUNT;  i++  ) {
		const std::string f = stem + ".sem." + stlab_sem_vocab[i].name + ".png";
		if(  load_mask(f, w, h, masks[i])  ) {
			any = true;
			stlab_sem_counters.masks_loaded++;
		}
	}
	// a mask whose name is not in the vocabulary would otherwise be ignored without a word
	{
		const size_t cut = stem.find_last_of("/\\");
		const std::string dir = cut == std::string::npos ? std::string(".") : stem.substr(0, cut);
		const std::string prefix = (cut == std::string::npos ? stem : stem.substr(cut + 1)) + ".sem.";
		searchfolder_t find;
		find.search(dir + "/", "png", searchfolder_t::SF_NONE);   // a trailing slash means "list this directory"
		for(  searchfolder_t::const_iterator it = find.begin();  it != find.end();  ++it  ) {
			std::string f = *it;
			const size_t c2 = f.find_last_of("/\\");
			if(  c2 != std::string::npos  ) f = f.substr(c2 + 1);
			if(  f.size() <= prefix.size() + 4  ||  STRNICMP(f.c_str(), prefix.c_str(), prefix.size()) != 0  ) continue;
			const std::string token = f.substr(prefix.size(), f.size() - prefix.size() - 4);
			bool known = false;
			for(  int i = 0;  i < STLAB_SEM_ID_COUNT  &&  !known;  i++  ) {
				known = STRICMP(token.c_str(), stlab_sem_vocab[i].name) == 0;
			}
			if(  !known  ) {
				dbg->warning("stlab_semantic_masks", "'%s': '%s' is not a semantic name, the mask is ignored",
				             sheet_png.c_str(), f.c_str());
				stlab_sem_counters.unknown_mask_names++;
			}
		}
	}
	// XR32-PRISSI-NIGHT-IMAGE-PROTOTYPE-01: prissi's single image per player-colour channel
	for(  int ch = 0;  ch < 2;  ch++  ) {
		const std::string f = stem + (ch == 0 ? ".pc1.png" : ".pc2.png");
		FILE *probe = fopen(f.c_str(), "rb");
		if(  probe == NULL  ) continue;
		fclose(probe);
		for(  int k = 0;  k < 8;  k++  ) {
			if(  !masks[ch * 8 + k].empty()  ) {
				dbg->fatal("stlab_semantic_masks", "'%s' and '%s.sem.%s.png' both describe player colour %d: use one or the other",
				           f.c_str(), stem.c_str(), stlab_sem_vocab[ch * 8 + k].name, ch + 1);
			}
		}
		raw_image_t img;
		if(  !img.read_from_file(f.c_str())  ) {
			dbg->fatal("stlab_semantic_masks", "cannot read player-colour image '%s'", f.c_str());
		}
		if(  (int)img.get_width() != w  ||  (int)img.get_height() != h  ) {
			dbg->fatal("stlab_semantic_masks", "player-colour image '%s' is %ux%u, its sheet is %dx%d",
			           f.c_str(), img.get_width(), img.get_height(), w, h);
		}
		for(  int k = 0;  k < 8;  k++  ) masks[ch * 8 + k].assign((size_t)w * h, 0);
		for(  int y = 0;  y < h;  y++  ) {
			for(  int x = 0;  x < w;  x++  ) {
				const uint8 *p = img.access_pixel(x, y);
				uint8 grey = 0, cov = 255;
				switch(  img.get_format()  ) {
					case raw_image_t::FMT_GRAY8:
						grey = p[0];
						break;
					case raw_image_t::FMT_RGB888:
					case raw_image_t::FMT_RGBA8888:
						if(  img.get_format() == raw_image_t::FMT_RGBA8888  ) cov = p[3];
						if(  cov  &&  (p[0] != p[1]  ||  p[1] != p[2])  ) {
							dbg->fatal("stlab_semantic_masks", "player-colour image '%s' is not grey at (%d,%d): the grey "
							           "says which shade, the alpha how much", f.c_str(), x, y);
						}
						grey = p[0];
						break;
					default:
						dbg->fatal("stlab_semantic_masks", "player-colour image '%s' has an unsupported format", f.c_str());
				}
				if(  cov == 0  ||  grey == 0  ) continue;          // black or transparent: no player colour
				int best = 0, bd = 256;
				for(  int k = 0;  k < 8;  k++  ) {
					const int code = (int)floor(255.0 * (0.15 + k * 0.65 / 7.0) + 0.5);
					const int d = abs((int)grey - code);
					if(  d < bd  ) { bd = d; best = k; }
				}
				if(  bd > 11  ) {
					dbg->fatal("stlab_semantic_masks", "player-colour image '%s' (%d,%d): grey %u is not a shade code "
					           "(PC0..PC7 = 38, 62, 86, 109, 133, 157, 180, 204)", f.c_str(), x, y, grey);
				}
				masks[ch * 8 + best][(size_t)y * w + x] = cov;
				stlab_night_counters.pc_image_pixels++;
			}
		}
		any = true;
		stlab_sem_counters.masks_loaded++;
	}
	if(  !any  ) {
		dbg->warning("stlab_semantic_masks", "semantic_masks=1 but no mask was found for '%s' "
		             "(expected %s.sem.<name>.png)", sheet_png.c_str(), stem.c_str());
		return false;
	}
	stlab_sem_counters.sheets_with_masks++;

	under.assign(rgba, rgba + (size_t)w * h * 4);
	uint32 reported = 0;

	for(  int y = 0;  y < h;  y++  ) {
		for(  int x = 0;  x < w;  x++  ) {
			const size_t pi = (size_t)y * w + x;
			const uint8 *b = rgba + pi * 4;
			const uint32 rgb = ((uint32)b[0] << 16) | ((uint32)b[1] << 8) | b[2];
			const bool hole = b[3] <= 7  ||  rgb == SPECIAL_TRANSPARENT;

			uint8 ids[STLAB_SEM_ID_COUNT], covs[STLAB_SEM_ID_COUNT];
			int n = 0;
			uint32 sum = 0;
			for(  int i = 0;  i < STLAB_SEM_ID_COUNT;  i++  ) {
				if(  !masks[i].empty()  &&  masks[i][pi]  ) {
					ids[n] = (uint8)i;
					covs[n] = masks[i][pi];
					sum += covs[n];
					n++;
				}
			}
			if(  hole  ) {
				if(  n  ) {
					stlab_sem_counters.dropped_on_holes++;
				}
				continue;
			}
			if(  n == 0  ) {
				// The masks DEFINE the semantics of an object that opted in: an exact key colour
				// nobody masked is ordinary paint here, whatever it happens to equal. The legacy
				// IMG is built from the base as always and still reads it as special, so say so.
				for(  int i = 0;  i < STLAB_SEM_ID_COUNT;  i++  ) {
					if(  key_of((uint8)i) == rgb  ) {
						if(  stlab_sem_counters.undeclared_key_pixels++ < 4  ) {
							dbg->warning("stlab_semantic_masks", "'%s' pixel (%d,%d) is the special colour of '%s' but "
							             "no mask declares it: ordinary in XR32, still special in the legacy image",
							             sheet_png.c_str(), x, y, stlab_sem_vocab[i].name);
						}
						break;
					}
				}
				continue;            // an ordinary pixel: the under plane is the base pixel
			}
			if(  n > STLAB_SEM_MAX_CONTRIB  ) {
				dbg->fatal("stlab_semantic_masks", "'%s' pixel (%d,%d) carries %d semantics, at most %d are allowed",
				           sheet_png.c_str(), x, y, n, STLAB_SEM_MAX_CONTRIB);
			}
			if(  sum > 255  ) {
				// masks rounded independently may sum one or two above full; more is an error
				if(  sum > 255u + (uint32)(n - 1)  ) {
					dbg->fatal("stlab_semantic_masks", "'%s' pixel (%d,%d): the masks sum to %u/255 (more than full)",
					           sheet_png.c_str(), x, y, sum);
				}
				while(  sum > 255  ) {
					int big = 0;
					for(  int j = 1;  j < n;  j++  ) {
						if(  covs[j] > covs[big]  ) big = j;
					}
					covs[big]--;
					sum--;
				}
				stlab_sem_counters.trimmed_pixels++;
			}

			// the under-colour, solved per channel
			uint8 *u = &under[pi * 4];
			if(  sum < 255  ) {
				const sint64 den = 255 - (sint64)sum;
				for(  int ch = 0;  ch < 3;  ch++  ) {
					sint64 num = 255 * (sint64)stlab_lin16[b[ch]];
					for(  int j = 0;  j < n;  j++  ) {
						const uint8 kc = (uint8)(key_of(ids[j]) >> (16 - 8 * ch));
						num -= (sint64)covs[j] * stlab_lin16[kc];
					}
					sint64 acc = num <= 0 ? 0 : (num * 255 + den / 2) / den;
					if(  acc > 65535 * 255  ) acc = 65535 * 255;
					u[ch] = stlab_lin_encode((uint32)acc);
				}
				if(  u[0] == 0xE7  &&  u[1] == 0xFF  &&  u[2] == 0xFF  ) {
					u[2] = 0xFE;                 // never let an under-colour read as a hole
					stlab_sem_counters.key_nudged++;
				}
			}

			// recompose and compare: masks and base must describe the same pixel
			uint32 worst = 0;
			for(  int ch = 0;  ch < 3;  ch++  ) {
				uint32 acc = (255 - sum) * (uint32)stlab_lin16[u[ch]];
				for(  int j = 0;  j < n;  j++  ) {
					const uint8 kc = (uint8)(key_of(ids[j]) >> (16 - 8 * ch));
					acc += (uint32)covs[j] * stlab_lin16[kc];
				}
				const int d = abs((int)stlab_lin_encode(acc) - (int)b[ch]);
				if(  (uint32)d > worst  ) worst = (uint32)d;
			}
			if(  worst > 2  ) {
				stlab_sem_counters.mismatch_pixels++;
				if(  worst > stlab_sem_counters.mismatch_max  ) stlab_sem_counters.mismatch_max = worst;
				if(  reported++ < 8  ) {
					dbg->warning("stlab_semantic_masks", "'%s' pixel (%d,%d): base and masks disagree by %u "
					             "(the base is not the mix the masks describe)", sheet_png.c_str(), x, y, worst);
				}
			}

			if(  n > 1  ) {
				stlab_sem_counters.multi_pixels++;
			}
			for(  int j = 0;  j < n;  j++  ) {   // ascending id, by construction
				if(  covs[j] == 0  ) continue;
				stlab_sem_contrib_t c;
				c.position = (uint32)pi;
				c.id = ids[j];
				c.coverage = covs[j];
				contribs.push_back(c);
				stlab_sem_counters.contributors++;
			}
		}
	}
	return true;
}


/* ------------------------------------------- XR32-PRISSI-NIGHT-IMAGE-PROTOTYPE-01: night sheets */

stlab_night_counters_t stlab_night_counters;
static bool night_requested = false;

void stlab_night_images_set_requested(bool on) { night_requested = on; }
bool stlab_night_images_requested() { return night_requested; }


const uint8 *stlab_night_sheet(const std::string &sheet_png, int w, int h)
{
	static std::string cached_for;
	static std::vector<uint8> cached;
	static bool cached_present = false;
	if(  cached_for == sheet_png  ) {
		return cached_present ? &cached[0] : NULL;
	}
	cached_for = sheet_png;
	cached_present = false;
	cached.clear();
	const size_t dot = sheet_png.rfind(".png");
	const std::string f = ((dot == std::string::npos) ? sheet_png : sheet_png.substr(0, dot)) + ".night.png";
	FILE *probe = fopen(f.c_str(), "rb");
	if(  probe == NULL  ) {
		return NULL;
	}
	fclose(probe);
	raw_image_t img;
	if(  !img.read_from_file(f.c_str())  ) {
		dbg->fatal("stlab_night", "cannot read night image '%s'", f.c_str());
	}
	if(  (int)img.get_width() != w  ||  (int)img.get_height() != h  ) {
		dbg->fatal("stlab_night", "night image '%s' is %ux%u, its day sheet is %dx%d: a night image must have the "
		           "day sheet's size and layout", f.c_str(), img.get_width(), img.get_height(), w, h);
	}
	cached.assign((size_t)w * h * 4, 0);
	for(  int y = 0;  y < h;  y++  ) {
		for(  int x = 0;  x < w;  x++  ) {
			const uint8 *p = img.access_pixel(x, y);
			uint8 *o = &cached[((size_t)y * w + x) * 4];
			switch(  img.get_format()  ) {
				case raw_image_t::FMT_RGBA8888: o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = p[3]; break;
				case raw_image_t::FMT_RGB888:   o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = 255;  break;
				case raw_image_t::FMT_GRAY8:    o[0] = o[1] = o[2] = p[0]; o[3] = 255;              break;
				default: dbg->fatal("stlab_night", "night image '%s' has an unsupported format", f.c_str());
			}
		}
	}
	cached_present = true;
	stlab_night_counters.sheets_with_night++;
	static bool report_registered = false;
	if(  !report_registered  ) {
		report_registered = true;
		const char *e = getenv("STLAB_SEM_REPORT");
		if(  e != NULL  &&  e[0] == '1'  &&  stlab_sem_counters.sheets_opted_in == 0  ) {
			atexit(stlab_semantic_masks_report);
		}
	}
	return &cached[0];
}


void stlab_night_under(const std::string &sheet_png, const uint8 *day_rgba, const uint8 *night_rgba, int w, int h,
                       const std::vector<uint8> &day_under, const std::vector<stlab_sem_contrib_t> &contribs,
                       std::vector<uint8> &night_under)
{
	night_under.assign(night_rgba, night_rgba + (size_t)w * h * 4);
	std::vector<uint16> csum((size_t)w * h, 0);
	for(  size_t i = 0;  i < contribs.size();  i++  ) {
		if(  contribs[i].id >= 16  ) {
			const int x = (int)(contribs[i].position % (uint32)w), y = (int)(contribs[i].position / (uint32)w);
			dbg->fatal("stlab_night", "'%s' (%d,%d): a light mask (%s) and a night image on the same sheet would "
			           "light the pixel twice; paint the light into the night image instead",
			           sheet_png.c_str(), x, y, stlab_sem_vocab[contribs[i].id].name);
		}
		csum[contribs[i].position] += contribs[i].coverage;
	}
	for(  size_t i = 0;  i < (size_t)w * h;  i++  ) {
		const uint8 *dp = day_rgba + i * 4, *np = night_rgba + i * 4;
		const bool day_hole = dp[3] <= 7  ||  (((uint32)dp[0] << 16 | (uint32)dp[1] << 8 | dp[2]) == SPECIAL_TRANSPARENT);
		const bool night_hole = np[3] <= 7  ||  (((uint32)np[0] << 16 | (uint32)np[1] << 8 | np[2]) == SPECIAL_TRANSPARENT);
		if(  day_hole  &&  !night_hole  ) stlab_night_counters.night_only_pixels++;
		if(  !day_hole  &&  night_hole  ) stlab_night_counters.day_only_pixels++;
		if(  day_hole  ||  night_hole  ||  csum[i] == 0  ||  csum[i] >= 255  ) {
			continue;                      // no player colour to take out, or nothing left under it
		}
		uint8 *o = &night_under[i * 4];
		const uint8 *ud = &day_under[i * 4];
		for(  int ch = 0;  ch < 3;  ch++  ) {
			const uint32 ld = stlab_lin16[dp[ch]];
			if(  ld == 0  ) continue;      // no day light to take a ratio from: keep the night pixel
			uint64 acc = (uint64)stlab_lin16[np[ch]] * stlab_lin16[ud[ch]] * 255u / ld;
			if(  acc > 65535u * 255u  ) acc = 65535u * 255u;
			o[ch] = stlab_lin_encode((uint32)acc);
		}
		if(  o[0] == 0xE7  &&  o[1] == 0xFF  &&  o[2] == 0xFF  ) o[2] = 0xFE;
	}
}


void stlab_semantic_masks_report()
{
	const stlab_sem_counters_t &c = stlab_sem_counters;
	if(  c.sheets_opted_in == 0  &&  stlab_night_counters.sheets_with_night == 0  ) {
		return;
	}
	printf("semantic masks: sheets opted-in %u, with masks %u, masks %u, contributors %u, multi %u, "
	       "undeclared keys %u, dropped-on-holes %u, trimmed %u, mismatch %u (max %u), key-nudged %u, unknown names %u, v2 nodes %u\n",
	       c.sheets_opted_in, c.sheets_with_masks, c.masks_loaded, c.contributors, c.multi_pixels,
	       c.undeclared_key_pixels, c.dropped_on_holes, c.trimmed_pixels, c.mismatch_pixels, c.mismatch_max,
	       c.key_nudged, c.unknown_mask_names, c.v2_nodes);
	const stlab_night_counters_t &n = stlab_night_counters;
	printf("night images: sheets %u, nodes %u, night-only px %u, day-only px %u, boxes grown %u, pc-image px %u\n",
	       n.sheets_with_night, n.nodes, n.night_only_pixels, n.day_only_pixels, n.bbox_grown, n.pc_image_pixels);
}
