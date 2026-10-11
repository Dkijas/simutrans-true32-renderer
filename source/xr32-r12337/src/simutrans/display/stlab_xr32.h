/*
 * Consumer of the XR32 image node, format candidate v1.
 *
 * XR32 is a candidate 4CC and a candidate v1 layout: the runtime reads it and an
 * engine that does not know it skips it, but the layout is not a settled ABI and
 * no compatibility is promised across future revisions of it.
 *
 * Layering, deliberately kept separate (the renderer must never learn the format):
 *
 *   FORMAT      stlab_xr32_parse()    bytes -> stlab_xr32_image_t
 *   SEMANTICS   stlab_xr32_resolve()  image + player + night -> ARGB8888 raster
 *   RENDERER    simgraph32            raster -> its own screen cache -> blit
 *
 * Nothing in this file produces or consumes STORED_PIXVAL.
 */
#ifndef DISPLAY_STLAB_XR32_H
#define DISPLAY_STLAB_XR32_H

#include "../simtypes.h"
#include "simimg.h"

/// one decoded semantic run, in the runtime representation (not the on-disk encoding)
struct stlab_xr32_run_t {
	uint32 start;      ///< first pixel index, y*width + x
	uint32 length;     ///< >= 1
	uint8  cls;        ///< 1 PLAYER_COLOUR_1, 2 PLAYER_COLOUR_2, 3 LIGHT
	uint8  param;      ///< player tone 0..7, or light index
	uint8  coverage;   ///< 1..255
	bool   under_is_framebuffer;
	uint8  under_r, under_g, under_b;
};

/// the runtime representation of one candidate node
struct stlab_xr32_image_t {
	uint16 width = 0, height = 0;
	sint16 x = 0, y = 0;
	uint8  zoomable = 0;
	uint16 img_index = 0;
	uint8  version = 0;
	uint8  *rgba = NULL;              ///< width*height*4, straight RGBA8888, owned here
	stlab_xr32_run_t *runs = NULL;    ///< owned here
	uint32 run_count = 0;
	uint32 runs_skipped_unknown_class = 0;
	/// XR32-PRISSI-NIGHT-IMAGE-PROTOTYPE-01 (EXPERIMENT): the full-night UNDER plane, same
	/// geometry as rgba, straight RGBA8888, owned here. NULL = no night image.
	uint8  *night = NULL;

	~stlab_xr32_image_t();
};

/* --- XR32-PRISSI-NIGHT-IMAGE-PROTOTYPE-01: EXPERIMENTAL night node "XNEX" (NOT A FORMAT) ---
 * A sibling of the IMG and its XR32 v2 node; carries the full-night under plane of one image.
 * Layout (little-endian), all of it experimental and free to change:
 *   0..3 "XNEX"  4 version=1  5 flags (bit0 deflated, bit1 CRC-32 trailer; both required)
 *   6 transition model (0 = 'abc')  7 reserved=0  8..9 w  10..11 h  12..13 x  14..15 y
 *   16..17 img_index  18..19 reserved=0  20..23 plane_bytes   then the plane, then CRC-32. */
struct stlab_xnex_t {
	uint16 width = 0, height = 0;
	sint16 x = 0, y = 0;
	uint16 img_index = 0;
	uint8  *rgba = NULL;              ///< width*height*4, owned here until attached
	~stlab_xnex_t();
};

/// generate DAY+NIGHT planes for every legacy image at registration (env SIMUTRANS_XR32_NIGHTGEN=1)
bool stlab_nightgen_enabled();

/**
 * One DAY+NIGHT pixel at engine level n2 = min(n, 4): the 'abc' transition per channel with
 * lut_rg / lut_b = the current level's (int)(v * m) tables. p, q = straight RGBA8888 of the
 * day and night under planes. Returns ARGB (straight alpha) or 0 for "no pixel".
 */
uint32 stlab_dn_pixel(const uint8 *p, const uint8 *q, int n2, const uint8 *lut_rg, const uint8 *lut_b);

/// what happened to one candidate node
enum stlab_xr32_status_t {
	STLAB_XR32_OK = 0,
	STLAB_XR32_REJECTED,       ///< structurally invalid -> fall back to IMG
	STLAB_XR32_UNSUPPORTED     ///< known tag, future version -> fall back to IMG, no error
};

/// XR32-PRISSI-NIGHT-IMAGE-PROTOTYPE-01: parse one XNEX body (layout above); atomic like stlab_xr32_parse
stlab_xr32_status_t stlab_xnex_parse(const uint8 *body, uint32 len, stlab_xnex_t *&out, const char **why);

/// is the XR32 consumer enabled at all? (env SIMUTRANS_XR32=1)
bool stlab_xr32_enabled();

/**
 * Parse one candidate node body. Atomic: on any failure nothing is handed back and the
 * caller must use the legacy IMG.
 */
stlab_xr32_status_t stlab_xr32_parse(const uint8 *body, uint32 len, stlab_xr32_image_t *&out,
                                     const char **why);

/**
 * SEMANTIC resolution. Produces width*height ARGB8888 pixels: alpha 0 means "no pixel".
 * Night is applied exactly once, in RGB888 arithmetic, never through a 555-indexed table.
 */
void stlab_xr32_resolve(const stlab_xr32_image_t *img, sint8 player_nr, uint32 *out_argb);

/* --- provided by simgraph32.cc so the semantics can read the live palette state --- */
uint32 stlab_player_colour_argb(sint8 player_nr, uint8 channel, uint8 tone);
uint32 stlab_light_colour_argb(uint8 light_index);
void   stlab_night_multipliers(double *rg, double *b);
int    stlab_night_level();          ///< XR32-PRISSI-NIGHT-IMAGE-PROTOTYPE-01: the engine level n (>= 0)
uint32 stlab_pack_argb(int r, int g, int b);

/// hand a parsed node to the renderer, paired with the image id of its sibling IMG
bool stlab_xr32_attach(image_id id, stlab_xr32_image_t *img,
                       sint16 img_x, sint16 img_y, sint16 img_w, sint16 img_h, uint8 img_zoomable);

/// does this image carry a candidate node?
bool stlab_xr32_has(image_id n);

/// XR32-FIT-IMG-TO-WIDTH-01: fit_img_to_width() chose the zoom step num/den for image @p id; its screen cache
/// must come from that step, not from the world zoom. 1/1 (or an image without a node) clears it.
void stlab_xr32_set_fit(image_id id, sint32 num, sint32 den);

/// XR32-PRISSI-NIGHT-IMAGE-PROTOTYPE-01: give image @p id (already carrying a node) its night plane.
/// Returns NULL on success, otherwise why the night node is not used.
const char *stlab_xnex_attach(image_id id, stlab_xnex_t *night);

/// replacement producer of the renderer's screen cache; true if it served the image
bool stlab_xr32_recode(image_id n, sint8 player_nr);

/// drop everything the consumer holds (called at shutdown and when images are freed)
void stlab_xr32_release_all();

#endif
