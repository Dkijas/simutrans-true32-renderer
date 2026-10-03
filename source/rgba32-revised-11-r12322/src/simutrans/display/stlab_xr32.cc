/*
 * Consumer of the XR32 image node, format candidate v1: parsing and semantic resolve.
 *
 * The on-disk layout is implemented here directly from its specification rather than
 * shared with the writer, so that a round-trip which passes is evidence about the
 * format and not about one implementation of it.
 */
#include "stlab_xr32.h"

#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "../simdebug.h"

static const uint8 XR32_MAGIC[4] = { 'X', 'R', '3', '2' };
static const uint8 XR32_SUPPORTED_VERSION = 1;
static const uint8 XR32_HEADER_LEN = 32;
static const uint8 XR32_FLAG_BASE_DEFLATED = 0x01;
static const uint8 XR32_FLAG_CRC32 = 0x02;           ///< body ends in a CRC-32 of everything before it
static const uint8 XR32_UNDER_IS_FRAMEBUFFER = 0x80;

static const uint8 XR32_CLASS_PLAYER_1 = 1;
static const uint8 XR32_CLASS_PLAYER_2 = 2;
static const uint8 XR32_CLASS_LIGHT = 3;


stlab_xr32_image_t::~stlab_xr32_image_t()
{
	free(rgba);
	free(runs);
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
	if(  (uint64)run_count * 5u > (uint64)run_bytes  ) {
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
	for(  uint32 i = 0;  i < run_count;  i++  ) {
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
	if(  off != run_bytes  ) { free(plane); free(runs); *why = "trailing bytes in the run block"; return STLAB_XR32_REJECTED; }

	stlab_xr32_image_t *img = new stlab_xr32_image_t();
	img->width = w; img->height = h; img->x = x; img->y = y;
	img->zoomable = zoomable; img->img_index = img_index; img->version = version;
	img->rgba = plane; img->runs = runs; img->run_count = kept;
	img->runs_skipped_unknown_class = skipped;
	out = img;
	return STLAB_XR32_OK;
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
	for(  uint32 i = 0;  i < npx;  i++, p += 4  ) {
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
