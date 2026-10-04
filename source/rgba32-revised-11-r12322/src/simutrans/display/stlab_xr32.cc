/*
 * Consumer of the XR32 image node, format candidate v1: parsing and semantic resolve.
 *
 * The on-disk layout is implemented here directly from its specification rather than
 * shared with the writer, so that a round-trip which passes is evidence about the
 * format and not about one implementation of it.
 */
#include "stlab_xr32.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <algorithm>
#include <vector>

#include "../simdebug.h"
#include "stlab_xr32_linear.h"

static const uint8 XR32_MAGIC[4] = { 'X', 'R', '3', '2' };
static const uint8 XR32_SUPPORTED_VERSION = 2;   ///< XR32-SEMANTIC-MASK-PIPELINE-01: v2 = explicit masks
static const uint8 XR32_HEADER_LEN = 32;
static const uint8 XR32_FLAG_BASE_DEFLATED = 0x01;
static const uint8 XR32_FLAG_CRC32 = 0x02;           ///< body ends in a CRC-32 of everything before it
static const uint8 XR32_UNDER_IS_FRAMEBUFFER = 0x80;

static const uint8 XR32_CLASS_PLAYER_1 = 1;
static const uint8 XR32_CLASS_PLAYER_2 = 2;
static const uint8 XR32_CLASS_LIGHT = 3;

/* XR32-SEMANTIC-MASK-PIPELINE-01 -- v2 (class registry 2). NOT FOR COMMIT. */
static const uint8 XR32_V2_CLASS_REGISTRY = 2;
static const uint8 XR32_V2_ID_COUNT = 31;        ///< ids 0..30 known; higher ids are skipped
static const uint8 XR32_V2_MAX_CONTRIB = 4;      ///< per pixel, == STLAB_SEM_MAX_CONTRIB in makeobj


stlab_xr32_image_t::~stlab_xr32_image_t()
{
	free(rgba);
	free(runs);
	free(night);
}


stlab_xnex_t::~stlab_xnex_t()
{
	free(rgba);
}


bool stlab_nightgen_enabled()
{
	static int cached = -1;
	if(  cached < 0  ) {
		const char *e = getenv("SIMUTRANS_XR32_NIGHTGEN");
		cached = (e != NULL  &&  e[0] == '1') ? 1 : 0;
	}
	return cached == 1;
}


bool stlab_xr32_enabled()
{
	static int cached = -1;
	if(  cached < 0  ) {
		const char *e = getenv("SIMUTRANS_XR32");
		cached = (e != NULL  &&  e[0] == '1') ? 1 : 0;
	}
	return cached == 1;
}


/* ------------------------------------------------------------------ little-endian readers */
static inline uint16 rd16(const uint8 *p) { return (uint16)(p[0] | (p[1] << 8)); }
static inline sint16 rds16(const uint8 *p) { return (sint16)rd16(p); }
static inline uint32 rd32(const uint8 *p)
{
	return (uint32)p[0] | ((uint32)p[1] << 8) | ((uint32)p[2] << 16) | ((uint32)p[3] << 24);
}

/// unsigned LEB128; returns false on truncation or an over-long encoding
static bool read_varint(const uint8 *b, uint32 len, uint32 &off, uint32 &out)
{
	uint32 shift = 0, val = 0;
	while(  true  ) {
		if(  off >= len  ) return false;
		const uint8 c = b[off++];
		val |= (uint32)(c & 0x7F) << shift;
		if(  (c & 0x80) == 0  ) { out = val; return true; }
		shift += 7;
		if(  shift > 28  ) return false;
	}
}


stlab_xr32_status_t stlab_xr32_parse(const uint8 *body, uint32 len, stlab_xr32_image_t *&out,
                                     const char **why)
{
	out = NULL;
	*why = "";
	if(  len < XR32_HEADER_LEN  ) { *why = "body shorter than the header"; return STLAB_XR32_REJECTED; }
	if(  memcmp(body, XR32_MAGIC, 4) != 0  ) { *why = "bad magic"; return STLAB_XR32_REJECTED; }

	const uint8 version = body[4];
	const uint8 flags = body[5];
	if(  version > XR32_SUPPORTED_VERSION  ) {
		*why = "node version is newer than this reader supports";
		return STLAB_XR32_UNSUPPORTED;   // fall back to IMG, not an error
	}
	if(  body[7] != 0  ||  body[17] != 0  ) { *why = "reserved byte not zero"; return STLAB_XR32_REJECTED; }
	if(  (flags & ~(XR32_FLAG_BASE_DEFLATED | XR32_FLAG_CRC32)) != 0  ) { *why = "unknown flag bit"; return STLAB_XR32_REJECTED; }

	// integrity first: nothing is allocated or inflated before the trailer has been checked
	if(  flags & XR32_FLAG_CRC32  ) {
		if(  len < (uint32)XR32_HEADER_LEN + 4  ) { *why = "body too short for its checksum"; return STLAB_XR32_REJECTED; }
		len -= 4;
		const uint32 stored = rd32(body + len);
		const uint32 actual = (uint32)crc32( crc32(0L, Z_NULL, 0), (const Bytef *)body, (uInt)len );
		if(  stored != actual  ) { *why = "checksum mismatch"; return STLAB_XR32_REJECTED; }
	}
	else if(  (flags & XR32_FLAG_BASE_DEFLATED) == 0  ) {
		// a raw plane carries no check of its own, so without the trailer corruption is invisible
		*why = "raw plane without checksum"; return STLAB_XR32_REJECTED;
	}

	const uint16 w = rd16(body + 8);
	const uint16 h = rd16(body + 10);
	const sint16 x = rds16(body + 12);
	const sint16 y = rds16(body + 14);
	const uint8 zoomable = body[16];
	const uint16 img_index = rd16(body + 18);
	const uint32 base_bytes = rd32(body + 20);
	const uint32 run_count = rd32(body + 24);
	const uint32 run_bytes = rd32(body + 28);

	if(  w == 0  ||  h == 0  ) { *why = "zero dimension"; return STLAB_XR32_REJECTED; }
	if(  (uint64)XR32_HEADER_LEN + base_bytes + run_bytes != (uint64)len  ) {
		*why = "declared lengths do not match the body"; return STLAB_XR32_REJECTED;
	}

	const uint64 want_plane = (uint64)w * h * 4u;
	if(  want_plane > 0x40000000ull  ) { *why = "implausible plane size"; return STLAB_XR32_REJECTED; }
	// nothing is allocated for what the node cannot actually contain
	if(  (flags & XR32_FLAG_BASE_DEFLATED)  &&  want_plane > (uint64)base_bytes * 1032u + 1024u  ) {
		*why = "declared plane larger than its compressed stream can expand to"; return STLAB_XR32_REJECTED;
	}
	if(  (flags & XR32_FLAG_BASE_DEFLATED) == 0  &&  (uint64)base_bytes != want_plane  ) {
		*why = "raw base plane has the wrong length"; return STLAB_XR32_REJECTED;
	}
	if(  version == 2  &&  body[6] != XR32_V2_CLASS_REGISTRY  ) {
		*why = "v2 node with an unknown class registry";
		return STLAB_XR32_UNSUPPORTED;
	}
	if(  (uint64)run_count * (version == 2 ? 4u : 5u) > (uint64)run_bytes  ) {
		*why = "more runs than the run block can hold"; return STLAB_XR32_REJECTED;
	}

	uint8 *plane = (uint8 *)malloc((size_t)want_plane);
	if(  plane == NULL  ) { *why = "out of memory for the plane"; return STLAB_XR32_REJECTED; }

	if(  flags & XR32_FLAG_BASE_DEFLATED  ) {
		uLongf dst = (uLongf)want_plane;
		const int rc = uncompress((Bytef *)plane, &dst, (const Bytef *)(body + XR32_HEADER_LEN),
		                          (uLong)base_bytes);
		if(  rc != Z_OK  ||  (uint64)dst != want_plane  ) {
			free(plane);
			*why = "base plane inflate failed";
			return STLAB_XR32_REJECTED;
		}
	}
	else {
		memcpy(plane, body + XR32_HEADER_LEN, (size_t)want_plane);   // length checked above
	}

	stlab_xr32_run_t *runs = NULL;
	if(  run_count  ) {
		runs = (stlab_xr32_run_t *)calloc(run_count, sizeof(stlab_xr32_run_t));
		if(  runs == NULL  ) { free(plane); *why = "out of memory for the runs"; return STLAB_XR32_REJECTED; }
	}

	const uint8 *blob = body + XR32_HEADER_LEN + base_bytes;
	uint32 off = 0, kept = 0, skipped = 0, prev_end = 0;
	const uint32 npx = (uint32)w * h;
	if(  version == 2  ) {
		// contributor runs: start is relative to the previous START, runs may overlap.
		// Per pixel: at most XR32_V2_MAX_CONTRIB contributors, coverage sum <= 255, no id twice.
		uint16 *sum = (uint16 *)calloc(npx, sizeof(uint16));
		uint8  *cnt = (uint8 *)calloc(npx, 1);
		uint32 *ids = (uint32 *)calloc(npx, sizeof(uint32));
		const char *err = NULL;
		if(  sum == NULL  ||  cnt == NULL  ||  ids == NULL  ) {
			err = "out of memory for the run check";
		}
		uint32 prev_start = 0;
		for(  uint32 i = 0;  err == NULL  &&  i < run_count;  i++  ) {
			uint32 d, ln;
			if(  !read_varint(blob, run_bytes, off, d)  ||  !read_varint(blob, run_bytes, off, ln)  ) { err = "truncated run"; break; }
			if(  off + 2 > run_bytes  ) { err = "truncated run payload"; break; }
			const uint8 id = blob[off++];
			const uint8 coverage = blob[off++];
			if(  coverage == 0  ) { err = "coverage 0 is not representable"; break; }
			if(  ln == 0  ) { err = "run length 0"; break; }
			if(  d > npx  ||  prev_start > npx - d  ) { err = "run start out of range"; break; }
			const uint32 start = prev_start + d;
			if(  start >= npx  ||  ln > npx - start  ) { err = "run end out of range"; break; }
			prev_start = start;
			for(  uint32 k = 0;  k < ln;  k++  ) {
				const uint32 px = start + k;
				if(  ++cnt[px] > XR32_V2_MAX_CONTRIB  ) { err = "too many contributors on one pixel"; break; }
				sum[px] += coverage;
				if(  sum[px] > 255  ) { err = "contributors sum above full coverage"; break; }
				if(  id < 32  ) {
					if(  ids[px] & (1u << id)  ) { err = "the same id twice on one pixel"; break; }
					ids[px] |= 1u << id;
				}
			}
			if(  err  ) break;
			if(  id >= XR32_V2_ID_COUNT  ) {
				skipped++;          // forward compatibility: a future id, its share falls to the under plane
				continue;
			}
			stlab_xr32_run_t &r = runs[kept++];
			r.start = start; r.length = ln; r.coverage = coverage; r.under_is_framebuffer = false;
			r.cls = id < 8 ? XR32_CLASS_PLAYER_1 : id < 16 ? XR32_CLASS_PLAYER_2 : XR32_CLASS_LIGHT;
			r.param = id < 16 ? (uint8)(id & 7) : (uint8)(id - 16);
		}
		if(  err == NULL  &&  off != run_bytes  ) err = "trailing bytes in the run block";
		free(sum); free(cnt); free(ids);
		if(  err  ) { free(plane); free(runs); *why = err; return STLAB_XR32_REJECTED; }
	}
	for(  uint32 i = 0;  version != 2  &&  i < run_count;  i++  ) {
		uint32 d, ln;
		if(  !read_varint(blob, run_bytes, off, d)  ||  !read_varint(blob, run_bytes, off, ln)  ) {
			free(plane); free(runs); *why = "truncated run"; return STLAB_XR32_REJECTED;
		}
		if(  off + 3 > run_bytes  ) { free(plane); free(runs); *why = "truncated run payload"; return STLAB_XR32_REJECTED; }
		const uint8 cls = blob[off++];
		const uint8 param = blob[off++];
		const uint8 coverage = blob[off++];
		const bool under_fb = (cls & XR32_UNDER_IS_FRAMEBUFFER) != 0;
		const uint8 base_cls = cls & 0x7F;

		if(  coverage == 0  ) { free(plane); free(runs); *why = "coverage 0 is not representable"; return STLAB_XR32_REJECTED; }
		if(  ln == 0  ) { free(plane); free(runs); *why = "run length 0"; return STLAB_XR32_REJECTED; }

		uint8 ur = 0, ug = 0, ub = 0;
		if(  coverage != 255  &&  !under_fb  ) {
			if(  off + 3 > run_bytes  ) { free(plane); free(runs); *why = "truncated under-colour"; return STLAB_XR32_REJECTED; }
			ur = blob[off++]; ug = blob[off++]; ub = blob[off++];
		}

		// overflow-safe bounds check
		if(  d > npx  ||  prev_end > npx - d  ) { free(plane); free(runs); *why = "run start out of range"; return STLAB_XR32_REJECTED; }
		const uint32 start = prev_end + d;
		if(  ln > npx - start  ) { free(plane); free(runs); *why = "run end out of range"; return STLAB_XR32_REJECTED; }
		prev_end = start + ln;

		if(  base_cls != XR32_CLASS_PLAYER_1  &&  base_cls != XR32_CLASS_PLAYER_2
		     &&  base_cls != XR32_CLASS_LIGHT  ) {
			skipped++;          // forward compatibility: drop the run, keep the node
			continue;
		}
		stlab_xr32_run_t &r = runs[kept++];
		r.start = start; r.length = ln; r.cls = base_cls; r.param = param;
		r.coverage = coverage; r.under_is_framebuffer = under_fb;
		r.under_r = ur; r.under_g = ug; r.under_b = ub;
	}
	if(  version != 2  &&  off != run_bytes  ) { free(plane); free(runs); *why = "trailing bytes in the run block"; return STLAB_XR32_REJECTED; }

	stlab_xr32_image_t *img = new stlab_xr32_image_t();
	img->width = w; img->height = h; img->x = x; img->y = y;
	img->zoomable = zoomable; img->img_index = img_index; img->version = version;
	img->rgba = plane; img->runs = runs; img->run_count = kept;
	img->runs_skipped_unknown_class = skipped;
	out = img;
	return STLAB_XR32_OK;
}


/* ------------------------------------------------- XR32-PRISSI-NIGHT-IMAGE-PROTOTYPE-01: XNEX */

static const uint8 XNEX_HEADER_LEN = 24;

stlab_xr32_status_t stlab_xnex_parse(const uint8 *body, uint32 len, stlab_xnex_t *&out, const char **why)
{
	out = NULL;
	*why = "";
	if(  len < (uint32)XNEX_HEADER_LEN + 4  ) { *why = "night node shorter than its header"; return STLAB_XR32_REJECTED; }
	if(  memcmp(body, "XNEX", 4) != 0  ) { *why = "night node: bad magic"; return STLAB_XR32_REJECTED; }
	if(  body[4] != 1  ) { *why = "night node: unknown version"; return STLAB_XR32_UNSUPPORTED; }
	if(  body[5] != 0x03  ) { *why = "night node: flags must be deflated + CRC-32"; return STLAB_XR32_REJECTED; }
	if(  body[6] != 0  ) { *why = "night node: unknown transition model"; return STLAB_XR32_UNSUPPORTED; }
	if(  body[7] != 0  ||  body[18] != 0  ||  body[19] != 0  ) { *why = "night node: reserved byte not zero"; return STLAB_XR32_REJECTED; }
	// integrity first: nothing is allocated or inflated before the trailer has been checked
	len -= 4;
	if(  rd32(body + len) != (uint32)crc32( crc32(0L, Z_NULL, 0), (const Bytef *)body, (uInt)len )  ) {
		*why = "night node: checksum mismatch"; return STLAB_XR32_REJECTED;
	}
	const uint16 w = rd16(body + 8), h = rd16(body + 10);
	const uint32 plane_bytes = rd32(body + 20);
	if(  w == 0  ||  h == 0  ) { *why = "night node: zero dimension"; return STLAB_XR32_REJECTED; }
	if(  (uint64)XNEX_HEADER_LEN + plane_bytes != (uint64)len  ) { *why = "night node: declared length does not match"; return STLAB_XR32_REJECTED; }
	const uint64 want_plane = (uint64)w * h * 4u;
	if(  want_plane > 0x40000000ull  ||  want_plane > (uint64)plane_bytes * 1032u + 1024u  ) {
		*why = "night node: implausible plane size"; return STLAB_XR32_REJECTED;
	}
	uint8 *plane = (uint8 *)malloc((size_t)want_plane);
	if(  plane == NULL  ) { *why = "night node: out of memory"; return STLAB_XR32_REJECTED; }
	uLongf dst = (uLongf)want_plane;
	if(  uncompress((Bytef *)plane, &dst, (const Bytef *)(body + XNEX_HEADER_LEN), (uLong)plane_bytes) != Z_OK
	     ||  (uint64)dst != want_plane  ) {
		free(plane);
		*why = "night node: plane inflate failed";
		return STLAB_XR32_REJECTED;
	}
	stlab_xnex_t *n = new stlab_xnex_t();
	n->width = w; n->height = h; n->x = rds16(body + 12); n->y = rds16(body + 14);
	n->img_index = rd16(body + 16);
	n->rgba = plane;
	out = n;
	return STLAB_XR32_OK;
}


/* The 'abc' transition of a DAY (D) + NIGHT (N) under colour, per channel, at engine level n:
 *   a = min(D, floor((D - N) / (1 - m4) + 0.5)) for D > N, else 0   the part that darkens like legacy
 *   b = D - a                                                       day-only
 *   c = N - (int)(a * m4)                                           night-only (signed)
 *   out = (int)(a * m_n) + ((b * (4 - n2) + c * n2) >> 2)           clamped to 0..255
 * f(0) = D and f(4) = N exactly; an ordinary pixel whose N is the legacy night of D gives the
 * legacy value at every level, a legacy light (N >= D) gives the legacy light interpolation.
 * m4 is the legacy multiplier of level 4, same expression as stlab_night_multipliers(). */
static uint8  stlab_abc_lut4[2][256];      ///< (int)(v * m4): [0] R,G  [1] B
static uint16 stlab_abc_a[2][256];          ///< a for D - N = 0..255, before the min with D
static bool   stlab_abc_ready = false;

static void stlab_abc_init()
{
	if(  stlab_abc_ready  ) return;
	const double m4[2] = { pow(0.75, 4) * ((0 + 8.0) / 8.0), pow(0.83, 4) * ((0 + 8.0) / 8.0) };
	for(  int k = 0;  k < 2;  k++  ) {
		for(  int v = 0;  v < 256;  v++  ) {
			stlab_abc_lut4[k][v] = (uint8)(int)(v * m4[k]);
			const int a = v == 0 ? 0 : (int)floor(v / (1.0 - m4[k]) + 0.5);
			stlab_abc_a[k][v] = (uint16)(a > 65535 ? 65535 : a);
		}
	}
	stlab_abc_ready = true;
}

/// one channel; lut = the current level's (int)(v * m_n) table, k = 0 for R/G, 1 for B
static inline uint8 stlab_abc(uint8 D, uint8 N, int n2, const uint8 *lut, int k)
{
	const int a = D > N ? (stlab_abc_a[k][D - N] < D ? stlab_abc_a[k][D - N] : D) : 0;
	const int b = D - a;
	const int c = (int)N - (int)stlab_abc_lut4[k][a];
	const int v = (int)lut[a] + ((b * (4 - n2) + c * n2) >> 2);
	return (uint8)(v < 0 ? 0 : v > 255 ? 255 : v);
}


uint32 stlab_dn_pixel(const uint8 *p, const uint8 *q, int n2, const uint8 *lut_rg, const uint8 *lut_b)
{
	stlab_abc_init();
	const bool hd = p[3] <= 7  ||  (p[0] == 0xE7 && p[1] == 0xFF && p[2] == 0xFF);
	const bool hn = q[3] <= 7  ||  (q[0] == 0xE7 && q[1] == 0xFF && q[2] == 0xFF);
	if(  hd  &&  hn  ) {
		return 0;
	}
	// a hole in one plane only: the colour comes from the other plane, the alpha fades in quarters
	const uint8 *dc = hd ? q : p, *nc = hn ? p : q;
	const int ad = hd ? 0 : p[3], an = hn ? 0 : q[3];
	const int a = (ad * (4 - n2) + an * n2) >> 2;
	if(  a <= 7  ) {
		return 0;
	}
	return ((uint32)a << 24) | ((uint32)stlab_abc(dc[0], nc[0], n2, lut_rg, 0) << 16)
	                         | ((uint32)stlab_abc(dc[1], nc[1], n2, lut_rg, 0) << 8)
	                         | (uint32)stlab_abc(dc[2], nc[2], n2, lut_b, 1);
}


/* ------------------------------------------------------------------ semantic resolution */

/* CANDIDATE C2 -- the night transform of an 8-bit channel has 256 answers.
 *
 * Built with the identical expression the pixel loop used, so every byte it
 * produces is the byte the pixel loop produced. Rebuilt only when the
 * multipliers actually move, which is on a night step.
 */
static double stlab_lut_rgm = -1.0, stlab_lut_bm = -1.0;
static uint8  stlab_lut_rg[256], stlab_lut_b[256];


static void stlab_night_lut(double rgm, double bm)
{
	if(  rgm == stlab_lut_rgm  &&  bm == stlab_lut_bm  ) {
		return;
	}
	for(  int v = 0;  v < 256;  v++  ) {
		int r = (int)(v * rgm);
		if(  r < 0  ) r = 0;
		if(  r > 255  ) r = 255;
		stlab_lut_rg[v] = (uint8)r;
		int c = (int)(v * bm);
		if(  c < 0  ) c = 0;
		if(  c > 255  ) c = 255;
		stlab_lut_b[v] = (uint8)c;
	}
	stlab_lut_rgm = rgm;
	stlab_lut_bm = bm;
}


void stlab_xr32_resolve(const stlab_xr32_image_t *img, sint8 player_nr, uint32 *out_argb)
{
	double rgm = 1.0, bm = 1.0;
	stlab_night_multipliers(&rgm, &bm);

	// 1. the appearance plane. Night is applied here, in RGB888, ONCE per pixel; there is
	//    no 555 index and no rgbmap lookup anywhere on this path.
	const uint32 npx = (uint32)img->width * img->height;
	stlab_night_lut( rgm, bm );
	const uint8 *lrg = stlab_lut_rg;
	const uint8 *lb  = stlab_lut_b;
	const uint8 *p = img->rgba;
	if(  img->night != NULL  ) {
		// XR32-PRISSI-NIGHT-IMAGE-PROTOTYPE-01: the under colour comes from DAY + NIGHT through
		// the 'abc' transition instead of the night LUT. A pixel that is a hole in one plane only
		// takes its colour from the other plane and fades its alpha in quarters, like a light.
		const int n2 = stlab_night_level() < 4 ? stlab_night_level() : 4;
		const uint8 *q = img->night;
		for(  uint32 i = 0;  i < npx;  i++, p += 4, q += 4  ) {
			out_argb[i] = stlab_dn_pixel( p, q, n2, lrg, lb );
		}
		p = NULL;
	}
	for(  uint32 i = 0;  p != NULL  &&  i < npx;  i++, p += 4  ) {
		const uint8 a = p[3];
		if(  a <= 7  ||  (p[0] == 0xE7 && p[1] == 0xFF && p[2] == 0xFF)  ) {
			out_argb[i] = 0;                      // hole
			continue;
		}
		// the straight alpha travels with the colour (0xFF when opaque); night touches the
		// colour only. The renderer composites 8..254 over the framebuffer.
		out_argb[i] = ((uint32)a << 24) | ((uint32)lrg[p[0]] << 16)
		                                | ((uint32)lrg[p[1]] << 8)
		                                | (uint32)lb[p[2]];
	}


	// 2v2. XR32-SEMANTIC-MASK-PIPELINE-01 -- NOT FOR COMMIT. Explicit contributors, composed
	//    in LINEAR light over the darkened under plane, integer tables only:
	//        out = enc( SUM c_i * L(S_i) + (255 - SUM c_i) * L(night(U)) )
	//    S_i is the live palette colour (player and night already inside it). The work is
	//    proportional to the contributors, never to the image area: one entry per contributor,
	//    sorted by pixel, summed per pixel (integer sums, so the order cannot change a byte).
	if(  img->version == 2  ) {
		struct contrib_t { uint32 idx, r, g, b, c; };
		size_t total = 0;
		for(  uint32 r = 0;  r < img->run_count;  r++  ) {
			total += img->runs[r].length;
		}
		if(  total == 0  ) {
			return;
		}
		std::vector<contrib_t> v;
		v.reserve(total);
		for(  uint32 r = 0;  r < img->run_count;  r++  ) {
			const stlab_xr32_run_t &run = img->runs[r];
			const uint32 fg = run.cls == XR32_CLASS_LIGHT ? stlab_light_colour_argb(run.param)
			                                              : stlab_player_colour_argb(player_nr, run.cls, run.param);
			const uint32 c = run.coverage;
			contrib_t e;
			e.r = c * stlab_lin16[(fg >> 16) & 0xFF];
			e.g = c * stlab_lin16[(fg >> 8) & 0xFF];
			e.b = c * stlab_lin16[fg & 0xFF];
			e.c = c;
			for(  uint32 k = 0;  k < run.length;  k++  ) {
				e.idx = run.start + k;           // bounds proven by the parser
				v.push_back(e);
			}
		}
		std::sort(v.begin(), v.end(), [](const contrib_t &a, const contrib_t &b) { return a.idx < b.idx; });
		for(  size_t i = 0;  i < v.size();  ) {
			const uint32 idx = v[i].idx;
			uint32 ar = 0, ag = 0, ab = 0, cs = 0;
			for(  ;  i < v.size()  &&  v[i].idx == idx;  i++  ) {
				ar += v[i].r; ag += v[i].g; ab += v[i].b; cs += v[i].c;
			}
			const uint32 u = out_argb[idx];       // night(U), already in the raster
			if(  u == 0  ) {
				continue;                         // a hole stays a hole
			}
			const uint32 rest = 255u - cs;        // cs <= 255, proven by the parser
			const uint8 r = stlab_lin_encode(ar + rest * stlab_lin16[(u >> 16) & 0xFF]);
			const uint8 g = stlab_lin_encode(ag + rest * stlab_lin16[(u >> 8) & 0xFF]);
			const uint8 b = stlab_lin_encode(ab + rest * stlab_lin16[u & 0xFF]);
			out_argb[idx] = (stlab_pack_argb(r, g, b) & 0x00FFFFFFu) | (u & 0xFF000000u);
		}
		return;
	}

	// 2. the semantic runs, resolved against the live palette. Night is already inside the
	//    palette entry, so it is applied once here too.
	for(  uint32 r = 0;  r < img->run_count;  r++  ) {
		const stlab_xr32_run_t &run = img->runs[r];
		uint32 fg;
		if(  run.cls == XR32_CLASS_LIGHT  ) {
			fg = stlab_light_colour_argb(run.param);
		}
		else {
			fg = stlab_player_colour_argb(player_nr, run.cls, run.param);
		}
		uint32 value = fg;
		if(  run.coverage != 255  &&  !run.under_is_framebuffer  ) {
			// the under-colour is an INTERNAL colour of the sprite: darken it once, in
			// RGB888, and blend the resolved foreground over it with the 8-bit coverage
			const int ur = (int)(run.under_r * rgm);
			const int ug = (int)(run.under_g * rgm);
			const int ub = (int)(run.under_b * bm);
			const int a = run.coverage;
			const int fr = (int)((fg >> 16) & 0xFF);
			const int fgn = (int)((fg >> 8) & 0xFF);
			const int fb = (int)(fg & 0xFF);
			value = stlab_pack_argb((fr * a + ur * (255 - a) + 127) / 255,
			                        (fgn * a + ug * (255 - a) + 127) / 255,
			                        (fb * a + ub * (255 - a) + 127) / 255);
		}
		for(  uint32 k = 0;  k < run.length;  k++  ) {
			const uint32 idx = run.start + k;
			if(  idx < npx  &&  out_argb[idx] != 0  ) {
				// the record sets the colour, the plane keeps deciding the alpha
				out_argb[idx] = (value & 0x00FFFFFFu) | (out_argb[idx] & 0xFF000000u);
			}
		}
	}
}
