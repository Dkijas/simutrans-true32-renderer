/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * 32 bit (ARGB8888) software renderer -- SKELETON.
 *
 * Built only for COLOUR_DEPTH=32. It owns a 4 byte per pixel framebuffer and
 * the backend lifecycle, and nothing else yet: image blitting, alpha, player
 * colours, day/night, zoom and text are deliberately still stubs, exactly as
 * in simgraph0. The 16 bit renderer in simgraph16.cc is untouched and is not
 * compiled into this configuration.
 */

#include "../simconst.h"
#include "../sys/simsys.h"
#include "../descriptor/image.h"
#include "../io/raw_image.h"
#include "../dataobj/ribi.h"          // draw_signal_direction
#include "../dataobj/environment.h"   // TILE_HEIGHT_STEP, draw_signal_direction

#include "simgraph.h"
#include "font.h"
#include "../utils/unicode.h"
#include "../utils/simstring.h"
#include "../dataobj/translator.h"
#include "simgraph32.h"
#include "simgraph_palette.h"

#include "../macros.h"
#include "../simmem.h"

#include <cmath>


#define DIRTY_TILE_SIZE  16
#define DIRTY_TILE_SHIFT  4

static void mark_tile_dirty32(int x, int y);
static void mark_rect_dirty_nc(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2);
static void simgraph32_mark_rect_dirty_wc(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2);
static void simgraph32_mark_rect_dirty_clip(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2  CLIP_NUM_DEF);
static void dirty_state_alloc();
static void dirty_state_free();
static void dirty_old_clear();

extern const sint32 zoom_num[MAX_ZOOM_FACTOR+1] = { 2, 3, 4, 1, 3, 5, 1, 3, 1, 1 };
extern const sint32 zoom_den[MAX_ZOOM_FACTOR+1] = { 1, 2, 3, 1, 4, 8, 2, 8, 4, 8 };


/*
 * The framebuffer. Owned by the backend (dr_textur_init), addressed here as
 * PIXVAL, which is 32 bit wide in this configuration -- 4 bytes per pixel.
 */
static PIXVAL *textur = NULL;

static scr_coord_val disp_width        = 0; // pitch, in pixels
static scr_coord_val disp_actual_width = 0; // window width
static scr_coord_val disp_height       = 0;


/*
 * ---------------------------------------------------------------------------
 * Screen colour tables, ARGB8888
 *
 * Same index space and same arithmetic as simgraph16.cc: the source values are
 * RGB555 plus the special indices held in STORED_PIXVAL. The only difference is
 * the output width, and that difference lives entirely inside
 * get_system_color(), which the backend supplies. Nothing here is quantised to
 * RGB565 first -- the day/night arithmetic is already RGB888, so it lands in
 * the 32 bit table at full precision.
 * ---------------------------------------------------------------------------
 */

#define RGBMAPSIZE      (0x8000 + LIGHT_COUNT + MAX_PLAYER_COUNT + 1024 /* 343 transparent */)
#define TRANSPARENT_RUN (0x8000u)

static PIXVAL rgbmap_day_night[RGBMAPSIZE];
static PIXVAL rgbmap_all_day[RGBMAPSIZE];
static PIXVAL *rgbmap_current = rgbmap_day_night;

static PIXVAL specialcolormap_day_night[256];
static PIXVAL specialcolormap_all_day[256];

static uint8 player_offsets[MAX_PLAYER_COUNT][2];

static int  light_level  = 0;
static int  night_shift  = -1;
static sint8 player_night = 0;
static sint8 player_day   = 0;


/**
 * Builds the ARGB8888 screen colour tables for a given night level.
 * Mirrors calc_base_pal_from_night_shift() in simgraph16.cc exactly.
 */
static void calc_base_pal_from_night_shift(const int night)
{
	const int night2 = min(night, 4);
	const int day    = 4 - night2;
	unsigned int i;

	const double RG_night_multiplier = pow(0.75, night) * ((light_level + 8.0) / 8.0);
	const double B_night_multiplier  = pow(0.83, night) * ((light_level + 8.0) / 8.0);

	// ordinary colours: RGB555 index -> RGB888 arithmetic -> screen colour
	for(  i = 0;  i < 0x8000;  i++  ) {
		int R = (i & 0x7C00) >> 7;
		int G = (i & 0x03E0) >> 2;
		int B = (i & 0x001F) << 3;

		R = (int)(R * RG_night_multiplier);
		G = (int)(G * RG_night_multiplier);
		B = (int)(B * B_night_multiplier);

		rgbmap_day_night[i] = get_system_color({ (uint8)R, (uint8)G, (uint8)B });
	}

	// the same again for the RGB343 semitransparent colours
	for(  i = 0;  i < 0x0400;  i++  ) {
		int R = (i & 0x0380) >> 2;
		int G = (i & 0x0078) << 1;
		int B = (i & 0x0007) << 5;

		R = (int)(R * RG_night_multiplier);
		G = (int)(G * RG_night_multiplier);
		B = (int)(B * B_night_multiplier);

		rgbmap_day_night[0x8000 + MAX_PLAYER_COUNT + LIGHT_COUNT + i] =
			get_system_color({ (uint8)R, (uint8)G, (uint8)B });
	}

	// player colours (also used for the map and the GUI)
	for(  i = 0;  i < SPECIAL_COLOR_COUNT;  i++  ) {
		const int R = (int)(special_pal[i].r * RG_night_multiplier);
		const int G = (int)(special_pal[i].g * RG_night_multiplier);
		const int B = (int)(special_pal[i].b * B_night_multiplier);

		specialcolormap_day_night[i] = get_system_color({ (uint8)R, (uint8)G, (uint8)B });
	}

	// special light colours
	for(  i = 0;  i < LIGHT_COUNT;  i++  ) {
		specialcolormap_day_night[SPECIAL_COLOR_COUNT + i] = get_system_color( display_day_lights[i] );
	}

	// black for the forbidden entries
	for(  i = SPECIAL_COLOR_COUNT + LIGHT_COUNT;  i < 256;  i++  ) {
		specialcolormap_day_night[i] = 0;
	}

	// default player colours
	for(  i = 0;  i < 8;  i++  ) {
		rgbmap_day_night[0x8000 + i] = specialcolormap_day_night[player_offsets[0][0] + i];
		rgbmap_day_night[0x8008 + i] = specialcolormap_day_night[player_offsets[0][1] + i];
	}
	player_night = 0;

	// lights: interpolate between the day and the night table
	for(  i = 0;  i < LIGHT_COUNT;  i++  ) {
		const int day_R = display_day_lights[i].r;
		const int day_G = display_day_lights[i].g;
		const int day_B = display_day_lights[i].b;

		const int night_R = display_night_lights[i].r;
		const int night_G = display_night_lights[i].g;
		const int night_B = display_night_lights[i].b;

		const int R = (day_R * day + night_R * night2) >> 2;
		const int G = (day_G * day + night_G * night2) >> 2;
		const int B = (day_B * day + night_B * night2) >> 2;

		rgbmap_day_night[0x8000 + MAX_PLAYER_COUNT + i] =
			get_system_color({ (uint8)max(R, 0), (uint8)max(G, 0), (uint8)max(B, 0) });
	}
}


/**
 * Brings the colour tables up, exactly as simgraph16_init() does: default
 * player colour offsets first, then the day tables.
 */
static void init_colour_tables()
{
	for(  int i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
		player_offsets[i][0] = i * 8;
		player_offsets[i][1] = i * 8 + 24;
	}

	calc_base_pal_from_night_shift( 0 );
	memcpy( rgbmap_all_day, rgbmap_day_night, RGBMAPSIZE * sizeof(PIXVAL) );
	memcpy( specialcolormap_all_day, specialcolormap_day_night, 256 * sizeof(PIXVAL) );
	rgbmap_current = rgbmap_day_night;
	night_shift = 0;
}

/**
 * Select the player colours and the table the colored/daytime families read.
 *
 * Transposed from activate_player_color() in simgraph16.cc, including its
 * caching by player_day / player_night: the colored family draws through
 * rgbmap_day_night, the daytime family through rgbmap_all_day, and which one
 * rgbmap_current points at is the whole difference between them.
 */
static void activate_player_color32(sint8 player_nr, bool daynight)
{
	if(  !daynight  ) {
		if(  player_day != player_nr  ) {
			player_day = player_nr;
			for(  int i = 0;  i < 8;  i++  ) {
				rgbmap_all_day[0x8000 + i] = specialcolormap_all_day[player_offsets[player_day][0] + i];
				rgbmap_all_day[0x8008 + i] = specialcolormap_all_day[player_offsets[player_day][1] + i];
			}
		}
		rgbmap_current = rgbmap_all_day;
	}
	else {
		if(  player_night != player_nr  ) {
			player_night = player_nr;
			for(  int i = 0;  i < 8;  i++  ) {
				rgbmap_day_night[0x8000 + i] = specialcolormap_day_night[player_offsets[player_night][0] + i];
				rgbmap_day_night[0x8008 + i] = specialcolormap_day_night[player_offsets[player_night][1] + i];
			}
		}
		rgbmap_current = rgbmap_day_night;
	}
}


/// clip_intv / clip_wh of simgraph16.cc, the part the colour families need
static int clip_wh32(scr_coord_val *x, scr_coord_val *w, const scr_coord_val left, const scr_coord_val right)
{
	scr_coord_val xx = *x + *w;
	scr_coord_val xoff = left - *x;

	if(  *x < left  ) {
		*x = left;
	}
	else {
		xoff = 0;
	}
	if(  xx > right  ) {
		xx = right;
	}
	*w = xx - *x;
	return xoff;
}



/*
 * ---------------------------------------------------------------------------
 * Legacy image recode
 *
 * base_data stays STORED_PIXVAL (16 bit) forever. Only the recoded, per player,
 * screen space cache is PIXVAL wide. The run structure is copied across
 * unchanged; only the colour words are looked up.
 *
 * Semitransparent pixels are deliberately NOT resolved to a screen colour here.
 * simgraph16 leaves them as a class+alpha value for the alpha blitter to decode,
 * and this cut reproduces that exactly rather than inventing an alpha
 * representation before the blitter exists.
 * ---------------------------------------------------------------------------
 */

struct imd32 {
	sint16 x, y, w, h;               ///< current (zoomed) offset and size
	sint16 base_x, base_y, base_w, base_h;
	uint8  recode_flags;             ///< FLAG_REZOOM / FLAG_ZOOMABLE
	uint32 len;                      ///< length of the CURRENT stored data, in STORED_PIXVAL units
	STORED_PIXVAL *base_data;        ///< 16 bit storage space, never widened
	STORED_PIXVAL *zoom_data;        ///< zoomed original data, still 16 bit storage space
	PIXVAL *data[MAX_PLAYER_COUNT];  ///< 32 bit screen space cache
	uint16 player_flags;             ///< bit n set => player n needs a recode
};

#define FLAG_HAS_PLAYER_COLOR      (1)
#define FLAG_HAS_TRANSPARENT_COLOR (2)
#define FLAG_ZOOMABLE (4)
#define FLAG_REZOOM   (8)

static imd32   *images      = NULL;
static image_id anz_images   = 0;
static image_id alloc_images = 0;


/**
 * Reduces an ARGB8888 screen colour to the RGB343 index used by the
 * semitransparent class encoding. Counterpart of pixval_to_rgb343() in
 * simgraph16.cc, which does the same from RGB565.
 */
static inline uint16 pixval_to_rgb343(PIXVAL c)
{
	//         msb                       lsb
	// argb8888: aaaaaaaarrrrrrrrggggggggbbbbbbbb
	// rgb343  :                       rrrggggbbb
	return (uint16)( ((c >> 14) & 0x0380) | ((c >> 9) & 0x0078) | ((c >> 5) & 0x07) );
}


/**
 * Recodes one image from storage space into ARGB8888 screen space.
 * The shape of the loop is that of recode_img_src_target() in simgraph16.cc.
 */
static void recode_img_src_target(scr_coord_val h, const STORED_PIXVAL *src, PIXVAL *target)
{
	if(  h > 0  ) {
		do {
			uint16 runlen = *src++;
			*target++ = runlen; // leading clear run, structure not colour
			do {
				runlen = *src++;
				*target++ = runlen; // colour run count, TRANSPARENT_RUN bit included
				if(  runlen & TRANSPARENT_RUN  ) {
					runlen &= ~TRANSPARENT_RUN;
					while(  runlen--  ) {
						if(  *src < 0x8020 + (31 * 16)  ) {
							// transparent player colour: resolve the player entry,
							// keep the class+alpha encoding for the alpha blitter
							const uint8 alpha  = (uint8)((*src - 0x8020) % 31);
							const PIXVAL colour = rgbmap_day_night[(*src - 0x8020) / 31 + 0x8000];
							*target++ = 0x8020 + 31 * 31 + pixval_to_rgb343(colour) * 31 + alpha;
							src++;
						}
						else {
							*target++ = *src++;
						}
					}
				}
				else {
					while(  runlen--  ) {
						*target++ = rgbmap_day_night[*src++];
					}
				}
				runlen = *src++;
				*target++ = runlen; // next clear run, zero ends the row
			} while(  runlen  );
		} while(  --h  );
	}
}


/*
 * ---------------------------------------------------------------------------
 * U5: the core blitter family.
 *
 *      recoded ARGB8888 cache  ->  nc / wc / pc  ->  ARGB8888 framebuffer
 *
 * The run encoding is the historical one and is not reinterpreted here: a row
 * is [clear][count][pixels...][clear]...[0], TRANSPARENT_RUN marks a
 * semi-transparent colour run, and a semi-transparent pixel carries the
 * class+alpha code the recode produced.  Only the width of a screen pixel
 * differs from simgraph16.
 * ---------------------------------------------------------------------------
 */

/* Blend two ARGB8888 colours, alpha 0..32.
 *
 * Same arithmetic as colors_blend_alpha32() in simgraph16.cc - the historical
 * contract - carried out at 8 bits per channel instead of 5/6, so no
 * intermediate quantisation happens. */
static inline PIXVAL blend_alpha32_argb(PIXVAL background, PIXVAL foreground, int alpha)
{
	const uint32 br = (background >> 16) & 0xFF, bg = (background >> 8) & 0xFF, bb = background & 0xFF;
	const uint32 fr = (foreground >> 16) & 0xFF, fg = (foreground >> 8) & 0xFF, fb = foreground & 0xFF;
	const uint32 r = (fr * alpha + (32 - alpha) * br) >> 5;
	const uint32 g = (fg * alpha + (32 - alpha) * bg) >> 5;
	const uint32 b = (fb * alpha + (32 - alpha) * bb) >> 5;
	return 0xFF000000u | (r << 16) | (g << 8) | b;
}


/** Copy opaque screen pixels. */
static inline void pixcopy32(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end)
{
	while(  src < end  ) {
		*dest++ = *src++;
	}
}


/**
 * Composite semi-transparent pixels that are already in SCREEN space.
 *
 * Counterpart of colorpixcopy_screen() in simgraph16.cc.  The caller has
 * already established from the run header that this run is semi-transparent,
 * so no classification by pixel value happens here.
 */
static inline void colorpixcopy_screen32(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end)
{
	while(  src < end  ) {
		const uint32 aux   = (uint32)*src++ - 0x8020u;
		const uint32 alpha = (aux % 31u) + 1u;

		*dest = blend_alpha32_argb( *dest, rgbmap_day_night[0x8000 + aux / 31u], (int)alpha );
		dest++;
	}
}


/**
 * Copy from STORED space, replacing player colour. The colored family.
 *
 * Transposed from colorpixcopy_stored() in simgraph16.cc. Two things are
 * deliberately kept and are load-bearing:
 *
 *  - the class test is on the FIRST pixel of the run and decides the whole
 *    run. That is the historical stored encoding, not an assumption;
 *  - the semi-transparent branch resolves against rgbmap_day_night, not
 *    rgbmap_current.
 *
 * What is NOT kept is the arithmetic width. The legacy routine blends in
 * RGB565 because that is what a screen pixel was; here the same historical
 * operation - blend family R, 32 steps - runs at 8 bits per channel, because
 * that is what a screen pixel is now. The stored input keeps its historical
 * quantization; the screen-space result does not acquire a new one.
 */
static inline void colorpixcopy_stored32(PIXVAL *dest, const STORED_PIXVAL *src, const STORED_PIXVAL * const end)
{
	if(  *src < 0x8020  ) {
		while(  src < end  ) {
			*dest++ = rgbmap_current[*src++];
		}
	}
	else {
		while(  src < end  ) {
			// a semi-transparent pixel
			const uint16 aux   = (uint16)(*src++ - 0x8020);
			const uint16 alpha = (uint16)((aux % 31u) + 1u);

			*dest = blend_alpha32_argb( *dest, rgbmap_day_night[0x8000 + aux / 31u], (int)alpha );
			dest++;
		}
	}
}


/**
 * As colorpixcopy_stored32(), but semi-transparent pixels resolve against the
 * all-day table. The daytime family.
 */
static inline void colorpixcopydaytime_stored32(PIXVAL *dest, const STORED_PIXVAL *src, const STORED_PIXVAL * const end)
{
	if(  *src < 0x8020  ) {
		while(  src < end  ) {
			*dest++ = rgbmap_current[*src++];
		}
	}
	else {
		while(  src < end  ) {
			// a semi-transparent pixel
			const uint16 aux   = (uint16)(*src++ - 0x8020);
			const uint16 alpha = (uint16)((aux % 31u) + 1u);

			*dest = blend_alpha32_argb( *dest, rgbmap_all_day[0x8000 + aux / 31u], (int)alpha );
			dest++;
		}
	}
}


/* ---------------- clipping state ----------------
 *
 * Reproduced from simgraph16.cc.  xrange and clip_line_t are pure integer
 * geometry and contain no colour type at all, so they are the same code.
 *
 * THREADING: simgraph16 keeps one clipping_info_t per thread and selects it
 * with CLIP_NUM.  This renderer keeps exactly one context.  That is enough for
 * U5's gates, and it is a real limitation, not an oversight: it is recorded in
 * the report and belongs to the cut that makes this renderer draw the world.
 */



#include "blit_clip_core.h"

#define MAX_POLY_CLIPS32 6

MSVC_ALIGN(64) struct clipping_info32_t {
	clip_dimension clip_rect;
	clip_dimension clip_rect_swap;
	bool           swap_active;
	int            number_of_clips;
	uint8          active_ribi;
	uint8          clip_ribi[MAX_POLY_CLIPS32];
	clip_line_t    poly_clips[MAX_POLY_CLIPS32];
	xrange         xranges[MAX_POLY_CLIPS32];
} GCC_ALIGN(64); // one per lane, on separate cache lines, as simgraph16 keeps them

/* One clipping context per CLIP_NUM lane. simview.cc hands lane t to
 * worker t and the last lane to the main thread, which also draws the
 * GUI on lane 0 - so the lane index, not the thread, selects the context.
 * Without MULTI_THREAD there is one lane and CR32 is the one object. */
#ifdef MULTI_THREAD
static clipping_info32_t clips32[MAX_THREADS];
#define CR32_0 clips32[0]
#else
static clipping_info32_t clips32;
#define CR32_0 clips32
#endif
#define CR32 clips32 CLIP_NUM_INDEX


static inline void init_ranges32(int y  CLIP_NUM_DEF)
{
	clip_core_init_ranges( y, CR32.number_of_clips, CR32.active_ribi,
	                       CR32.clip_ribi, CR32.poly_clips, CR32.xranges );
}


static inline void get_xrange_and_step_y32(int &xmin, int &xmax  CLIP_NUM_DEF)
{
	xmin = CR32.clip_rect.x;
	xmax = CR32.clip_rect.xx;
	clip_core_step_y( xmin, xmax, CR32.number_of_clips, CR32.active_ribi,
	                  CR32.clip_ribi, CR32.poly_clips, CR32.xranges );
}


/* ---------------- the three blitters ---------------- */

#include "blit_core.h"
#include "zoom_core.h"

/** Pixel policy of the 32 bit renderer. */
struct PixelOps32 {
	typedef PIXVAL screen_t;   ///< ARGB8888

	static inline void copy(screen_t *dest, const screen_t *src, const screen_t * const end)
	{
#ifdef NC_POLICY16
		/* NEGATIVE CONTROL ONLY: the SCREEN16 semantic applied inside the
		 * SCREEN32 policy - every pixel through an RGB565 round trip. */
		while(  src < end  ) { *dest++ = rgb565_to_screen_colour( screen_colour_to_rgb565( *src++ ) ); }
#else
		pixcopy32(dest, src, end);
#endif
	}

	static inline void alpha(screen_t *dest, const screen_t *src, const screen_t * const end)
	{
		colorpixcopy_screen32(dest, src, end);
	}
};


/** Draw each image without clipping. */
static void display_img_nc32(scr_coord_val h, const scr_coord_val xp, const scr_coord_val yp, const PIXVAL *sp  CLIP_NUM_DEF)
{
	blit_core_nc<PixelOps32>( h, textur + xp + yp * disp_width, sp, disp_width );
}


/**
 * Pixel policy of the clipped 32 bit blitters.
 *
 * Separate from PixelOps32 for the same reason the 16 bit renderer keeps two:
 * nc is where a renderer is free to use an unrolled copy, and wc/pc are not.
 * The 32 bit renderer has no unrolled copy today, so the two policies do the
 * same work - but the boundary is where it belongs, and the nc negative
 * control keeps meaning exactly what it meant.
 */
struct PixelOps32c {
	typedef PIXVAL screen_t;   ///< ARGB8888
	typedef PIXVAL source_t;

	static inline void copy(screen_t *dest, const source_t *src, const source_t * const end)
	{
		pixcopy32(dest, src, end);
	}

	static inline void alpha(screen_t *dest, const source_t *src, const source_t * const end)
	{
		colorpixcopy_screen32(dest, src, end);
	}
};


/** Row interval of the polygon-clipped blitter. CR32 stays where it is. */
struct span_poly32_t {
#ifdef MULTI_THREAD
	const sint8 clip_num;
#endif

	inline void next(int &xmin, int &xmax) const { get_xrange_and_step_y32( xmin, xmax  CLIP_NUM_PAR ); }
};


/** Draw image with horizontal clipping. */
static void display_img_wc32(scr_coord_val h, const scr_coord_val xp, const scr_coord_val yp, const PIXVAL *sp  CLIP_NUM_DEF)
{
	if(  h > 0  ) {
		blit_core_clipped<PixelOps32c>( h, textur + yp * disp_width, sp, xp, disp_width,
		                                span_fixed_t{ CR32.clip_rect.x, CR32.clip_rect.xx } );
	}
}


/** Draw image with clipping along arbitrary lines. */
static void display_img_pc32(scr_coord_val h, const scr_coord_val xp, const scr_coord_val yp, const PIXVAL *sp  CLIP_NUM_DEF)
{
	if(  h > 0  ) {
		init_ranges32( yp   CLIP_NUM_PAR);
		blit_core_clipped<PixelOps32c>( h, textur + yp * disp_width, sp, xp, disp_width,
		                                span_poly32_t{ CLIP_NUM_VAR } );
	}
}




/* ---------------- colored / daytime: policies, not a second walker --------
 *
 * source_t is STORED_PIXVAL here, not PIXVAL. These families read the original
 * image data because that is where the class and player information lives; the
 * recoded screen cache has already resolved it away. blit_core_clipped is
 * parameterised on source_t precisely so this does not need its own traversal.
 */

/** colored: day/night lighting, player colours substituted from STORED data. */
struct PixelOps32Colored {
	typedef PIXVAL        screen_t;   ///< ARGB8888
	typedef STORED_PIXVAL source_t;

	static inline void copy(screen_t *dest, const source_t *src, const source_t * const end)
	{
		colorpixcopy_stored32(dest, src, end);
	}

	static inline void alpha(screen_t *dest, const source_t *src, const source_t * const end)
	{
		colorpixcopy_stored32(dest, src, end);
	}
};


/**
 * daytime, as the POLYGON-clipped blitter has it.
 *
 * The asymmetry is historical and is preserved rather than corrected: the
 * opaque run resolves against the all-day table, a run flagged TRANSPARENT_RUN
 * resolves against the day/night one. simgraph16 does exactly this through
 * templated_pixcopy<daytime> / templated_alphacopy<daytime>.
 */
struct PixelOps32Daytime {
	typedef PIXVAL        screen_t;
	typedef STORED_PIXVAL source_t;

	static inline void copy(screen_t *dest, const source_t *src, const source_t * const end)
	{
		colorpixcopydaytime_stored32(dest, src, end);
	}

	static inline void alpha(screen_t *dest, const source_t *src, const source_t * const end)
	{
		colorpixcopy_stored32(dest, src, end);
	}
};


/**
 * daytime, as the RECTANGLE-clipped blitter has it.
 *
 * display_color_img_wc_daytime() discards TRANSPARENT_RUN from the run length
 * and sends every run through the all-day routine, which then classifies by
 * stored value. So for that path the two operations are the same one - and
 * that is a different contract from the polygon case above, not a tidier
 * spelling of it.
 */
struct PixelOps32DaytimeWC {
	typedef PIXVAL        screen_t;
	typedef STORED_PIXVAL source_t;

	static inline void copy(screen_t *dest, const source_t *src, const source_t * const end)
	{
		colorpixcopydaytime_stored32(dest, src, end);
	}

	static inline void alpha(screen_t *dest, const source_t *src, const source_t * const end)
	{
		colorpixcopydaytime_stored32(dest, src, end);
	}
};


/* ---------------- the historical blend families, at 8 bits per channel -----
 *
 * Four of them, and they are NOT interchangeable. The 16 bit renderer performs
 * each one on packed RGB565 fields with masks that stop the carry; the same
 * arithmetic per channel is the faithful transposition, and it is what runs
 * here - no operand and no result is quantised through RGB565 on the way.
 *
 *   A   colors_blend25/50/75      GUI/API. Reached only for alpha 8, 16, 24.
 *   B   blend25_t/50_t/75_t       image and outline.
 *   R   colors_blend_alpha32      alpha compositing, 32 steps.
 *   L   d + ((s-d)*alpha)>>6      tint_rect's arbitrary percentage. A fourth
 *                                 family: it is neither A, B nor R.
 *
 * A and B differ at 25 and 75 and coincide at 50 - by arithmetic accident, not
 * by design - so a test vector whose channels are multiples of four cannot tell
 * them apart. The U7B gates use vectors that can.
 */

#define ARGB_R(c) (((c) >> 16) & 0xFFu)
#define ARGB_G(c) (((c) >>  8) & 0xFFu)
#define ARGB_B(c) ( (c)        & 0xFFu)
#define ARGB(r,g,b) (0xFF000000u | ((uint32)(r) << 16) | ((uint32)(g) << 8) | (uint32)(b))

/// family B, 25%: three quarters background
struct blend_b25_argb {
	static inline PIXVAL blend(PIXVAL bg, PIXVAL fg)
	{
		return ARGB( 3 * (ARGB_R(bg) >> 2) + (ARGB_R(fg) >> 2),
		             3 * (ARGB_G(bg) >> 2) + (ARGB_G(fg) >> 2),
		             3 * (ARGB_B(bg) >> 2) + (ARGB_B(fg) >> 2) );
	}
};

/// family B, 50%
struct blend_b50_argb {
	static inline PIXVAL blend(PIXVAL bg, PIXVAL fg)
	{
		return ARGB( (ARGB_R(bg) >> 1) + (ARGB_R(fg) >> 1),
		             (ARGB_G(bg) >> 1) + (ARGB_G(fg) >> 1),
		             (ARGB_B(bg) >> 1) + (ARGB_B(fg) >> 1) );
	}
};

/// family B, 75%: three quarters foreground
struct blend_b75_argb {
	static inline PIXVAL blend(PIXVAL bg, PIXVAL fg)
	{
		return ARGB( (ARGB_R(bg) >> 2) + 3 * (ARGB_R(fg) >> 2),
		             (ARGB_G(bg) >> 2) + 3 * (ARGB_G(fg) >> 2),
		             (ARGB_B(bg) >> 2) + 3 * (ARGB_B(fg) >> 2) );
	}
};

/// family A, the GUI/API one. Note it is NOT family B at 25 or 75.
static inline PIXVAL colors_blend25_argb(PIXVAL bg, PIXVAL fg)
{
	return ARGB( (ARGB_R(bg) >> 1) + (ARGB_R(bg) >> 2) + (ARGB_R(fg) >> 2),
	             (ARGB_G(bg) >> 1) + (ARGB_G(bg) >> 2) + (ARGB_G(fg) >> 2),
	             (ARGB_B(bg) >> 1) + (ARGB_B(bg) >> 2) + (ARGB_B(fg) >> 2) );
}

static inline PIXVAL colors_blend50_argb(PIXVAL bg, PIXVAL fg)
{
	return ARGB( (ARGB_R(bg) >> 1) + (ARGB_R(fg) >> 1),
	             (ARGB_G(bg) >> 1) + (ARGB_G(fg) >> 1),
	             (ARGB_B(bg) >> 1) + (ARGB_B(fg) >> 1) );
}

static inline PIXVAL colors_blend75_argb(PIXVAL bg, PIXVAL fg)
{
	return ARGB( (ARGB_R(bg) >> 2) + (ARGB_R(fg) >> 1) + (ARGB_R(fg) >> 2),
	             (ARGB_G(bg) >> 2) + (ARGB_G(fg) >> 1) + (ARGB_G(fg) >> 2),
	             (ARGB_B(bg) >> 2) + (ARGB_B(fg) >> 1) + (ARGB_B(fg) >> 2) );
}

/// family L: the arbitrary-percentage interpolation tint_rect falls back to
static inline PIXVAL tint_lerp_argb(PIXVAL bg, PIXVAL fg, int alpha64)
{
	const int dr = (int)ARGB_R(bg), dg = (int)ARGB_G(bg), db = (int)ARGB_B(bg);
	const int sr = (int)ARGB_R(fg), sg = (int)ARGB_G(fg), sb = (int)ARGB_B(fg);
	return ARGB( dr + (((sr - dr) * alpha64) >> 6),
	             dg + (((sg - dg) * alpha64) >> 6),
	             db + (((sb - db) * alpha64) >> 6) );
}

/**
 * The GUI/API entry, mirroring display_blend_colors_alpha32(): family A on the
 * three exact quarters, family R everywhere else.
 */
static PIXVAL blend_colors_alpha32_argb(PIXVAL background, PIXVAL foreground, int alpha)
{
	switch(  alpha  ) {
		case 0:  return background;
		case 8:  return colors_blend25_argb( background, foreground );
		case 16: return colors_blend50_argb( background, foreground );
		case 24: return colors_blend75_argb( background, foreground );
		case 32: return foreground;
		default: return blend_alpha32_argb( background, foreground, alpha );
	}
}


/* ---------------- the pixel policies for the blend/alpha/outline paths ----
 *
 * All of them run on blit_core_clipped. None of them adds a walker.
 */

/// family B over a SCREEN32 source: the rezoomed image blend
template<class F>
struct PixelOps32Blend {
	typedef PIXVAL screen_t;
	typedef PIXVAL source_t;

	inline void copy(screen_t *dest, const source_t *src, const source_t * const end) const
	{
		while(  src < end  ) { *dest = F::blend( *dest, *src ); dest++; src++; }
	}
	inline void alpha(screen_t *dest, const source_t *src, const source_t * const end) const
	{
		copy( dest, src, end );
	}
};

/// family B over a STORED source, recoded per pixel: the base image blend
template<class F>
struct PixelOps32BlendRecode {
	typedef PIXVAL        screen_t;
	typedef STORED_PIXVAL source_t;

	inline void copy(screen_t *dest, const source_t *src, const source_t * const end) const
	{
		while(  src < end  ) { *dest = F::blend( *dest, rgbmap_current[*src] ); dest++; src++; }
	}
	inline void alpha(screen_t *dest, const source_t *src, const source_t * const end) const
	{
		copy( dest, src, end );
	}
};

/// family B against a constant colour, ignoring the source: the outline
template<class F, class Src>
struct PixelOps32Outline {
	typedef PIXVAL screen_t;
	typedef Src    source_t;

	PIXVAL colour;

	inline void copy(screen_t *dest, const source_t *src, const source_t * const end) const
	{
		const size_t len = (size_t)(end - src);
		for(  size_t i = 0;  i < len;  i++  ) { dest[i] = F::blend( dest[i], colour ); }
	}
	inline void alpha(screen_t *dest, const source_t *src, const source_t * const end) const
	{
		copy( dest, src, end );
	}
};

/**
 * family R driven by a separate alpha image.
 *
 * The alpha map has the same run layout as the source, so the legacy walker
 * advances both pointers in lockstep. That means the alpha map offset is
 * exactly the source offset - which is why this needs no second walker: the
 * policy is handed the current source pointer and works the rest out itself.
 *
 * The alpha value itself is read from STORED15 data and summed over three
 * 5 bit fields, exactly as the legacy routine does. That quantisation is
 * intrinsic to the alpha channel's storage, not something introduced here.
 */
template<class Src, bool RECODE>
struct PixelOps32Alpha {
	typedef PIXVAL screen_t;
	typedef Src    source_t;

	const Src           *src_base;
	const STORED_PIXVAL *alphamap_base;
	STORED_PIXVAL        alpha_mask;

	static inline PIXVAL resolve(const Src s)
	{
		return RECODE ? rgbmap_current[s] : (PIXVAL)s;
	}

	inline void copy(screen_t *dest, const source_t *src, const source_t * const end) const
	{
		const STORED_PIXVAL *am = alphamap_base + (src - src_base);
		while(  src < end  ) {
			// read mask components - always 15bpp
			const uint16 masked = (uint16)(*am & alpha_mask);
			uint16 alpha_value = (uint16)((masked & 0x1f) + ((masked >> 5) & 0x1f) + ((masked >> 10) & 0x1f));

			if(  alpha_value > 30  ) {
				*dest = resolve( *src );
			}
			else if(  alpha_value > 0  ) {
				alpha_value = alpha_value > 15 ? (uint16)(alpha_value + 1) : alpha_value;
				*dest = blend_alpha32_argb( *dest, resolve( *src ), (int)alpha_value );
			}
			dest++; src++; am++;
		}
	}
	inline void alpha(screen_t *dest, const source_t *src, const source_t * const end) const
	{
		copy( dest, src, end );
	}
};


/// clip_lr of simgraph16.cc, the part tint_rect needs
static bool clip_lr32(scr_coord_val *x, scr_coord_val *w, const scr_coord_val left, const scr_coord_val right)
{
	clip_wh32( x, w, left, right );
	return *w > 0;
}


/* ---------------- zoom ----------------
 *
 * The resampling lives in zoom_core.h and is the historical algorithm, shared
 * with simgraph16. What differs here is the intermediate representation: this
 * renderer uses zoom_sample_tagged_t, where "no pixel" is a bit beside the
 * value instead of the colour 0x73FE. No image colour can be read as state.
 */

static uint32 zoom_factor32 = ZOOM_NEUTRAL;

/* One scratch pair, not one per thread. simgraph16 keeps MAX_THREADS of them
 * because it rezooms from worker threads; this renderer has a single clipping
 * context and no worker path yet, so a second array would be state without a
 * user. Adding the array later is mechanical and nothing here blocks it. */
static zoom_scratch_t zoom_scratch = { NULL, NULL, 0 };
#ifdef MULTI_THREAD
static pthread_mutex_t rezoom_img_mutex32 = PTHREAD_MUTEX_INITIALIZER;
#ifdef MULTI_THREAD
// recoding is serialised exactly as simgraph16 does it: the caller checks
// player_flags first, then recode_img re-checks it under this lock, so two
// display lanes cannot recode the same image at once
static pthread_mutex_t recode_img_mutex32 = PTHREAD_MUTEX_INITIALIZER;
#endif
#endif


/// mark every zoomable image as needing a new zoom
static void rezoom32()
{
	for(  image_id n = 0;  n < anz_images;  n++  ) {
		if(  (images[n].recode_flags & FLAG_ZOOMABLE) != 0  &&  images[n].base_h > 0  ) {
			images[n].recode_flags |= FLAG_REZOOM;
		}
	}
}


/// Convert base image data to the current zoom. Mirrors rezoom_img().
static void rezoom_img32(const image_id n)
{
	if(  n < anz_images  &&  images[n].base_h > 0  ) {
#ifdef MULTI_THREAD
		pthread_mutex_lock( &rezoom_img_mutex32 );
		if(  (images[n].recode_flags & FLAG_REZOOM) == 0  ) {
			pthread_mutex_unlock( &rezoom_img_mutex32 );
			return;
		}
#endif
		images[n].player_flags = 0xFFFF; // recode all player colors

		if(  images[n].zoom_data != NULL  ) {
			free( images[n].zoom_data );
			images[n].zoom_data = NULL;
		}
		for(  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
			if(  images[n].data[i] != NULL  ) {
				free( images[n].data[i] );
				images[n].data[i] = NULL;
			}
		}

		if(  zoom_factor32 == ZOOM_NEUTRAL  ||  (images[n].recode_flags & FLAG_ZOOMABLE) == 0  ) {
			images[n].x   = images[n].base_x;
			images[n].y   = images[n].base_y;
			images[n].w   = images[n].base_w;
			images[n].h   = images[n].base_h;
			images[n].len = zoom_core_stored_len( images[n].base_data, images[n].base_h );
			images[n].recode_flags &= ~FLAG_REZOOM;
#ifdef MULTI_THREAD
			pthread_mutex_unlock( &rezoom_img_mutex32 );
#endif
			return;
		}

		zoom_image_t out;
		out.x    = images[n].x;
		out.y    = images[n].y;
		out.w    = images[n].w;
		out.h    = images[n].h;
		out.len  = images[n].len;
		out.data = NULL;

		zoom_core_rezoom<zoom_sample_tagged_t>(
			images[n].base_data,
			images[n].base_x, images[n].base_y, images[n].base_w, images[n].base_h,
			g_simgraph32.zoom_num[zoom_factor32], g_simgraph32.zoom_den[zoom_factor32],
			zoom_scratch, out );

		images[n].x         = out.x;
		images[n].y         = out.y;
		images[n].w         = out.w;
		images[n].h         = out.h;
		images[n].len       = out.len;
		images[n].zoom_data = out.data;

		images[n].recode_flags &= ~FLAG_REZOOM;
#ifdef MULTI_THREAD
		pthread_mutex_unlock( &rezoom_img_mutex32 );
#endif
	}
}


/// the stored buffer the colour families must read: zoomed if there is one
static inline const STORED_PIXVAL *current_stored(const image_id n)
{
	return images[n].zoom_data != NULL ? images[n].zoom_data : images[n].base_data;
}


/** Colour-substituted image, clipped by the rectangle. Shared traversal. */
template<class Ops>
static void display_color_img_wc32(const STORED_PIXVAL *sp, scr_coord_val x, scr_coord_val y, scr_coord_val h  CLIP_NUM_DEF)
{
	if(  h > 0  ) {
		blit_core_clipped<Ops>( h, textur + y * disp_width, sp, x, disp_width,
		                        span_fixed_t{ CR32.clip_rect.x, CR32.clip_rect.xx } );
	}
}


/** Colour-substituted image, clipped by the polygon. Shared traversal. */
template<class Ops>
static void display_color_img_pc32(scr_coord_val h, scr_coord_val x, scr_coord_val y, const STORED_PIXVAL *sp  CLIP_NUM_DEF)
{
	if(  h > 0  ) {
		init_ranges32( y   CLIP_NUM_PAR);
		blit_core_clipped<Ops>( h, textur + y * disp_width, sp, x, disp_width,
		                        span_poly32_t{ CLIP_NUM_VAR } );
	}
}


/* ---------------- the blend / alpha / outline entry points ---------------- */

static void recode_img(const image_id n, const sint8 player_nr);   // defined below

/**
 * Skip @p skip_lines rows of a run stream and reduce @p h, exactly as the
 * legacy blend/alpha entry points do before drawing.
 */
template<class Src>
static bool clip_rows32(const Src *&sp, scr_coord_val &yp, scr_coord_val &h  CLIP_NUM_DEF)
{
	const scr_coord_val reduce_h = yp + h - CR32.clip_rect.yy;
	if(  reduce_h > 0  ) {
		h -= reduce_h;
	}
	if(  h <= 0  ) {
		return false;
	}
	scr_coord_val skip_lines = CR32.clip_rect.y - (int)yp;
	if(  skip_lines > 0  ) {
		if(  skip_lines >= h  ) {
			return false;
		}
		h  -= skip_lines;
		yp += skip_lines;
		while(  skip_lines--  ) {
			do {
				sp++;
				sp += (*sp) & (~TRANSPARENT_RUN);
				sp++;
			} while(  *sp  );
			sp++;
		}
	}
	return true;
}


/** Run one blend/outline policy over an image, on the shared traversal. */
template<class Ops>
static void display_img_blend_wc32(scr_coord_val h, scr_coord_val xp, scr_coord_val yp,
                                   const typename Ops::source_t *sp, const Ops &ops  CLIP_NUM_DEF)
{
	if(  h > 0  ) {
		blit_core_clipped<Ops>( h, textur + yp * disp_width, sp, xp, disp_width,
		                        span_fixed_t{ CR32.clip_rect.x, CR32.clip_rect.xx }, ops );
	}
}


/** The transparent (blended or outlined) draw of a zoomed image. */
static void simgraph32_draw_rezoomed_img_blend(const image_id n, scr_coord_val xp, scr_coord_val yp, const sint8, const FLAGGED_PIXVAL color_index, const bool, const bool dirty  CLIP_NUM_DEF)
{
	if(  n >= anz_images  ) {
		return;
	}
	if(  images[n].recode_flags & FLAG_REZOOM  ) {
		rezoom_img32( n );
		recode_img( n, 0 );
	}
	else if(  images[n].player_flags & 1  ) {
		recode_img( n, 0 );
	}
	const PIXVAL *sp = images[n].data[0];
	if(  sp == NULL  ) {
		return;
	}

	xp += images[n].x;
	yp += images[n].y;
	scr_coord_val h = images[n].h;
	if(  !clip_rows32( sp, yp, h   CLIP_NUM_PAR)  ) {
		return;
	}

	if(  dirty  ) {
		simgraph32_mark_rect_dirty_wc( xp, yp, xp + images[n].w - 1, yp + h - 1 );
	}

	const PIXVAL colour = get_flagged_colour( color_index );
	const uint8  level  = get_transparency_level( color_index );

	if(  has_outline_flag( color_index )  ) {
		switch(  level  ) {
			case 1: { PixelOps32Outline<blend_b25_argb, PIXVAL> o; o.colour = colour; display_img_blend_wc32( h, xp, yp, sp, o   CLIP_NUM_PAR); break; }
			case 2: { PixelOps32Outline<blend_b50_argb, PIXVAL> o; o.colour = colour; display_img_blend_wc32( h, xp, yp, sp, o   CLIP_NUM_PAR); break; }
			default:{ PixelOps32Outline<blend_b75_argb, PIXVAL> o; o.colour = colour; display_img_blend_wc32( h, xp, yp, sp, o   CLIP_NUM_PAR); break; }
		}
	}
	else {
		switch(  level  ) {
			case 1: display_img_blend_wc32( h, xp, yp, sp, PixelOps32Blend<blend_b25_argb>()   CLIP_NUM_PAR); break;
			case 2: display_img_blend_wc32( h, xp, yp, sp, PixelOps32Blend<blend_b50_argb>()   CLIP_NUM_PAR); break;
			default:display_img_blend_wc32( h, xp, yp, sp, PixelOps32Blend<blend_b75_argb>()   CLIP_NUM_PAR); break;
		}
	}
}


/** The same, but for the base image: the source is STORED and recoded per pixel. */
static void simgraph32_draw_base_img_blend(const image_id n, scr_coord_val xp, scr_coord_val yp, const sint8 player_nr, const FLAGGED_PIXVAL color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF)
{
	if(  n >= anz_images  ) {
		return;
	}
	activate_player_color32( (player_nr >= 0 && player_nr < MAX_PLAYER_COUNT) ? player_nr : 0, daynight );

	const STORED_PIXVAL *sp = images[n].base_data;
	xp += images[n].base_x;
	yp += images[n].base_y;
	scr_coord_val h = images[n].base_h;
	if(  !clip_rows32( sp, yp, h   CLIP_NUM_PAR)  ) {
		return;
	}

	if(  dirty  ) {
		simgraph32_mark_rect_dirty_wc( xp, yp, xp + images[n].base_w - 1, yp + h - 1 );
	}

	const PIXVAL colour = get_flagged_colour( color_index );
	const uint8  level  = get_transparency_level( color_index );

	if(  has_outline_flag( color_index )  ) {
		switch(  level  ) {
			case 1: { PixelOps32Outline<blend_b25_argb, STORED_PIXVAL> o; o.colour = colour; display_img_blend_wc32( h, xp, yp, sp, o   CLIP_NUM_PAR); break; }
			case 2: { PixelOps32Outline<blend_b50_argb, STORED_PIXVAL> o; o.colour = colour; display_img_blend_wc32( h, xp, yp, sp, o   CLIP_NUM_PAR); break; }
			default:{ PixelOps32Outline<blend_b75_argb, STORED_PIXVAL> o; o.colour = colour; display_img_blend_wc32( h, xp, yp, sp, o   CLIP_NUM_PAR); break; }
		}
	}
	else {
		switch(  level  ) {
			case 1: display_img_blend_wc32( h, xp, yp, sp, PixelOps32BlendRecode<blend_b25_argb>()   CLIP_NUM_PAR); break;
			case 2: display_img_blend_wc32( h, xp, yp, sp, PixelOps32BlendRecode<blend_b50_argb>()   CLIP_NUM_PAR); break;
			default:display_img_blend_wc32( h, xp, yp, sp, PixelOps32BlendRecode<blend_b75_argb>()   CLIP_NUM_PAR); break;
		}
	}
}


/// which colour channels of the alpha image contribute, as simgraph16 has it
static STORED_PIXVAL get_alpha_mask32(const unsigned alpha_flags)
{
	STORED_PIXVAL mask = 0;
	if(  alpha_flags & ALPHA_RED    ) mask |= 0x7c00;
	if(  alpha_flags & ALPHA_GREEN  ) mask |= 0x03e0;
	if(  alpha_flags & ALPHA_BLUE   ) mask |= 0x001f;
	return mask;
}


/** A zoomed image composited through a separate alpha image. */
static void simgraph32_draw_rezoomed_img_alpha(const image_id n, const image_id alpha_n, const unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, const sint8, const FLAGGED_PIXVAL, const bool, const bool dirty  CLIP_NUM_DEF)
{
	if(  n >= anz_images  ||  alpha_n >= anz_images  ) {
		return;
	}
	if(  images[n].recode_flags & FLAG_REZOOM  ) {
		rezoom_img32( n );
		recode_img( n, 0 );
	}
	else if(  images[n].player_flags & 1  ) {
		recode_img( n, 0 );
	}
	if(  images[alpha_n].recode_flags & FLAG_REZOOM  ) {
		rezoom_img32( alpha_n );
	}
	const PIXVAL *sp = images[n].data[0];
	if(  sp == NULL  ) {
		return;
	}
	const STORED_PIXVAL *alphamap = current_stored( alpha_n );

	xp += images[n].x;
	yp += images[n].y;
	scr_coord_val h = images[n].h;
	const PIXVAL *const sp_base = sp;
	if(  !clip_rows32( sp, yp, h   CLIP_NUM_PAR)  ) {
		return;
	}

	if(  dirty  ) {
		simgraph32_mark_rect_dirty_wc( xp, yp, xp + images[n].w - 1, yp + h - 1 );
	}

	PixelOps32Alpha<PIXVAL, false> ops;
	ops.src_base      = sp_base;
	ops.alphamap_base = alphamap;
	ops.alpha_mask    = get_alpha_mask32( alpha_flags );
	display_img_blend_wc32( h, xp, yp, sp, ops   CLIP_NUM_PAR);
}


/** The base image composited through a separate alpha image. */
static void simgraph32_draw_base_img_alpha(const image_id n, const image_id alpha_n, const unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, const sint8 player_nr, const FLAGGED_PIXVAL, const bool daynight, bool dirty  CLIP_NUM_DEF)
{
	if(  n >= anz_images  ||  alpha_n >= anz_images  ) {
		return;
	}
	activate_player_color32( (player_nr >= 0 && player_nr < MAX_PLAYER_COUNT) ? player_nr : 0, daynight );

	const STORED_PIXVAL *sp       = images[n].base_data;
	const STORED_PIXVAL *alphamap = images[alpha_n].base_data;

	xp += images[n].base_x;
	yp += images[n].base_y;
	scr_coord_val h = images[n].base_h;
	const STORED_PIXVAL *const sp_base = sp;
	if(  !clip_rows32( sp, yp, h   CLIP_NUM_PAR)  ) {
		return;
	}

	if(  dirty  ) {
		simgraph32_mark_rect_dirty_wc( xp, yp, xp + images[n].base_w - 1, yp + h - 1 );
	}

	PixelOps32Alpha<STORED_PIXVAL, true> ops;
	ops.src_base      = sp_base;
	ops.alphamap_base = alphamap;
	ops.alpha_mask    = get_alpha_mask32( alpha_flags );
	display_img_blend_wc32( h, xp, yp, sp, ops   CLIP_NUM_PAR);
}


/**
 * The production entry point that chooses between the three, mirroring
 * display_img_aux() in simgraph16.cc: polygon clip wins, otherwise the image
 * is either fully inside the clip rectangle (nc) or straddles it (wc).
 */
static void display_img_aux32(const image_id n, scr_coord_val xp, scr_coord_val yp, const sint8 player_nr_raw, const bool dirty  CLIP_NUM_DEF)
{
	if(  n >= anz_images  ||  images[n].h == 0  ) {
		return;
	}
	const sint8 player_nr = (player_nr_raw >= 0 && player_nr_raw < MAX_PLAYER_COUNT) ? player_nr_raw : 0;
	if(  images[n].data[player_nr] == NULL  ) {
		return;
	}

	const PIXVAL *sp = images[n].data[player_nr];

	yp += images[n].y;
	scr_coord_val h = images[n].h;

	// vertical clipping first, exactly as the legacy renderer does
	const scr_coord_val reduce_h = yp + h - CR32.clip_rect.yy;
	if(  reduce_h > 0  ) {
		h -= reduce_h;
	}
	if(  h <= 0  ) {
		return;
	}
	scr_coord_val skip_lines = CR32.clip_rect.y - (int)yp;
	if(  skip_lines > 0  ) {
		if(  skip_lines >= h  ) {
			return;
		}
		h  -= skip_lines;
		yp += skip_lines;
		while(  skip_lines--  ) {
			do {
				sp++;
				sp += (*sp) & (~TRANSPARENT_RUN);
				sp++;
			} while(  *sp  );
			sp++;
		}
	}

	const scr_coord_val w = images[n].w;
	xp += images[n].x;

	/* The dirty marks sit here, not in draw_img_aux, because THIS is the
	 * function that picks the clipping branch - simgraph16_draw_img_aux
	 * does both jobs and marks three times, once per branch. Order and
	 * marker are the legacy ones: nc marks BEFORE the draw and does not
	 * clamp, pc and wc mark AFTER it through the clipping marker.
	 * The height is the one already reduced by the vertical clip above,
	 * exactly as the legacy comment "since height may be reduced, start
	 * marking here" requires. */
	if(  CR32.number_of_clips > 0  ) {
		display_img_pc32( h, xp, yp, sp   CLIP_NUM_PAR);
		if(  dirty  ) {
			simgraph32_mark_rect_dirty_clip( xp, yp, xp + w - 1, yp + h - 1  CLIP_NUM_DEFAULT );
		}
	}
	else if(  xp >= CR32.clip_rect.x  &&  xp + w <= CR32.clip_rect.xx  ) {
		if(  dirty  ) {
			mark_rect_dirty_nc( xp, yp, xp + w - 1, yp + h - 1 );
		}
		display_img_nc32( h, xp, yp, sp   CLIP_NUM_PAR);
	}
	else if(  xp < CR32.clip_rect.xx  &&  xp + w > CR32.clip_rect.x  ) {
		display_img_wc32( h, xp, yp, sp   CLIP_NUM_PAR);
		if(  dirty  ) {
			simgraph32_mark_rect_dirty_clip( xp, yp, xp + w - 1, yp + h - 1  CLIP_NUM_DEFAULT );
		}
	}
}


static PIXVAL          simgraph32_palette_lookup             (palette_index_t idx);
static palette_index_t simgraph32_palette_indexof            (PIXVAL color);
static rgb888_t        simgraph32_get_color_rgb              (palette_index_t idx);
static rgb888_t        simgraph32_get_pixval_rgb             (PIXVAL c);
static void            simgraph32_env_t_rgb_to_system_colors ();
static void            simgraph32_set_player_color_scheme    (const int player, const uint8 col1, const uint8 col2);
static void            simgraph32_set_light_color            (int light_idx, rgb888_t day_light, rgb888_t night_light);
static void            simgraph32_set_daynight_level         (int night);
static scr_coord_val   simgraph32_set_base_raster_width      (scr_coord_val new_raster);
static int             simgraph32_zoom_factor_up             ();
static int             simgraph32_zoom_factor_down           ();
static bool            simgraph32_init                       (scr_size window_size, sint16 full_screen);
static bool            simgraph32_is_display_init            ();
static void            simgraph32_exit                       ();
static void            simgraph32_on_window_resized          (scr_size new_window_size);
static bool            simgraph32_load_font                  (const char *fname, bool reload);
static image_id        simgraph32_get_image_count            ();
static image_id        simgraph32_register_image             (const image_t *image_in);
static void            simgraph32_free_all_images_above      (image_id above );
static scr_rect        simgraph32_get_base_image_offset      (image_id image);
static scr_rect        simgraph32_get_image_offset           (image_id image);
static void            simgraph32_mark_img_dirty             (image_id image, scr_coord_val xp, scr_coord_val yp);
static void            simgraph32_mark_rect_dirty_wc         (scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2);
static void            simgraph32_mark_rect_dirty_clip       (scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2  CLIP_NUM_DEF);
static void            simgraph32_mark_screen_dirty          ();
static scr_size        simgraph32_get_screen_size            ();
static void            simgraph32_set_screen_actual_width    (scr_coord_val w);
static void            simgraph32_set_screen_height          (scr_coord_val const h);
static scr_size        simgraph32_get_best_matching_size     (const image_id n, sint16 zoom_percent);
static void            simgraph32_fit_img_to_width           (const image_id n, sint16 new_w);
static void            simgraph32_move_scroll_band           (scr_coord_val start_y, scr_coord_val x_offset, scr_coord_val h);
static void            simgraph32_set_image_procs            (bool is_global);
static void            simgraph32_draw_img_aligned           (const image_id n, scr_rect area, int align, const bool dirty);
static void            simgraph32_draw_img_aux               (const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF);
static void            simgraph32_draw_rezoomed_img_blend    (const image_id, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF);
static void            simgraph32_draw_rezoomed_img_alpha    (const image_id, const image_id, const unsigned, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF);
static void            simgraph32_draw_color_img             (const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF);
static void            simgraph32_draw_base_img              (const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF);
static void            simgraph32_draw_base_img_blend        (const image_id, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF);
static void            simgraph32_draw_base_img_alpha        (const image_id, const image_id, const unsigned, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, bool  CLIP_NUM_DEF);
static void            simgraph32_draw_stretch_map           (const stretch_map_t &imag, scr_rect area);
static void            simgraph32_draw_stretch_map_blend     (const stretch_map_t &imag, scr_rect area, FLAGGED_PIXVAL color);
static PIXVAL          simgraph32_blend_colors               (PIXVAL, PIXVAL, int);
static void            simgraph32_tint_rect                  (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, int);
static void            simgraph32_draw_rect                  (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool);
static void            simgraph32_draw_rect_clipped          (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool  CLIP_NUM_DEF);
static void            simgraph32_draw_rect_colors_clipped   (scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL* color, scr_coord_val num_colors, bool horizontal, bool dirty   CLIP_NUM_DEF);
static void            simgraph32_draw_rounded_rect_clipped  (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool);
static void            simgraph32_draw_vline_clipped         (scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool  CLIP_NUM_DEF);
static void            simgraph32_flush_framebuffer          ();
static void            simgraph32_set_cursor_visible         (bool);
static void            simgraph32_set_default_cursor         (int);
static void            simgraph32_set_show_load_cursor       (bool);
static void            simgraph32_draw_array                 (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL *);
static scr_coord_val   simgraph32_calc_text_width_n          (const char *, size_t);
static scr_size        simgraph32_calc_multiline_text_size   (const char *text);
static size_t          simgraph32_calc_text_index_for_width  (const char *, scr_coord_val);
static bool            simgraph32_font_has_character         (utf16 char_code);
static utf32           simgraph32_get_next_char_with_metrics (const char* &text, unsigned char &byte_length, unsigned char &pixel_width);
static utf32           simgraph32_get_prev_char_with_metrics (const char* &text, const char *const text_start, unsigned char &byte_length, unsigned char &pixel_width);
static scr_coord_val   simgraph32_get_char_width             (utf32 c);
static scr_coord_val   simgraph32_get_number_width           ();
static scr_coord_val   simgraph32_draw_text_clipped_n        (scr_coord_val, scr_coord_val, const char*, control_alignment_t , const PIXVAL, bool, sint32  CLIP_NUM_DEF);
static scr_coord_val   simgraph32_draw_multiline_text        (scr_coord_val, scr_coord_val, const char *, PIXVAL);
static void            simgraph32_draw_text_ellipsis_shadowed(scr_rect, const char *, int, PIXVAL, bool, bool, PIXVAL);
static void            simgraph32_draw_text_outlined         (scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, const char *, int);
static void            simgraph32_draw_text_shadowed         (scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, const char *, int);
static void            simgraph32_draw_box3d                 (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, bool);
static void            simgraph32_draw_box3d_clipped         (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, PIXVAL);
static void            simgraph32_draw_textbox3d_clipped     (scr_coord_val, scr_coord_val, FLAGGED_PIXVAL, FLAGGED_PIXVAL, const char *, int  CLIP_NUM_DEF);
static void            simgraph32_draw_line                  (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL);
static void            simgraph32_draw_line_dotted           (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL);
static void            simgraph32_draw_empty_circle          (scr_coord_val, scr_coord_val, int, const PIXVAL);
static void            simgraph32_draw_filled_circle         (scr_coord_val, scr_coord_val, int, const PIXVAL);
static void            simgraph32_draw_bezier                (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL, scr_coord_val, scr_coord_val);
static void            simgraph32_draw_right_triangle        (scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL, const bool);
static bool            simgraph32_take_screenshot            (const scr_rect &area, const char *filename);
static void            simgraph32_draw_signal_direction      (scr_coord_val, scr_coord_val, uint8, uint8, PIXVAL, PIXVAL, bool, uint8);
static void            simgraph32_set_clip_rect              (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF, bool fit);
static clip_dimension  simgraph32_get_clip_rect              (CLIP_NUM_DEF0);
static void            simgraph32_push_clip_rect             (scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF);
static void            simgraph32_swap_clip_rect             (CLIP_NUM_DEF0);
static void            simgraph32_pop_clip_rect              (CLIP_NUM_DEF0);

#ifdef MULTI_THREAD
static void            simgraph32_add_poly_clip              (int, int, int, int, int  CLIP_NUM_DEF);
static void            simgraph32_clear_all_poly_clip        (const sint8);
static void            simgraph32_activate_ribi_clip         (int  CLIP_NUM_DEF);
#else
static void            simgraph32_add_poly_clip              (int, int, int, int, int);
static void            simgraph32_clear_all_poly_clip        ();
static void            simgraph32_activate_ribi_clip         (int);
#endif



simgraph_t g_simgraph32 = {
	/*.type =*/ SIMGRAPH_TYPE_SOFTWARE32,

	/*.tile_raster_width         =*/ 16, // zoomed
	/*.base_tile_raster_width    =*/ 16, // original
	/*.current_tile_raster_width =*/ 0,
	/*.draw_normal               =*/ NULL,
	/*.draw_color                =*/ NULL,
	/*.draw_blend                =*/ NULL,
	/*.draw_alpha                =*/ NULL,

	/*.zoom_num =*/ { 2, 3, 4, 1, 3, 5, 1, 3, 1, 1 },
	/*.zoom_den =*/ { 1, 2, 3, 1, 4, 8, 2, 8, 4, 8 },

	/*.palette_lookup              =*/ simgraph32_palette_lookup,
	/*.palette_indexof             =*/ simgraph32_palette_indexof,
	/*.get_color_rgb               =*/ simgraph32_get_color_rgb,
	/*.get_pixval_rgb              =*/ simgraph32_get_pixval_rgb,
	/*.env_t_rgb_to_system_colors  =*/ simgraph32_env_t_rgb_to_system_colors,
	/*.set_player_color_scheme     =*/ simgraph32_set_player_color_scheme,
	/*.set_light_color             =*/ simgraph32_set_light_color,
	/*.set_daynight_level          =*/ simgraph32_set_daynight_level,
	/*.set_base_raster_width       =*/ simgraph32_set_base_raster_width,
	/*.zoom_factor_up              =*/ simgraph32_zoom_factor_up,
	/*.zoom_factor_down            =*/ simgraph32_zoom_factor_down,
	/*.init                        =*/ simgraph32_init,
	/*.is_display_init             =*/ simgraph32_is_display_init,
	/*.exit                        =*/ simgraph32_exit,
	/*.on_window_resized           =*/ simgraph32_on_window_resized,
	/*.load_font                   =*/ simgraph32_load_font,
	/*.get_image_count             =*/ simgraph32_get_image_count,
	/*.register_image              =*/ simgraph32_register_image,
	/*.free_all_images_above       =*/ simgraph32_free_all_images_above,
	/*.get_base_image_offset       =*/ simgraph32_get_base_image_offset,
	/*.get_image_offset            =*/ simgraph32_get_image_offset,
	/*.mark_img_dirty              =*/ simgraph32_mark_img_dirty,
	/*.mark_rect_dirty_wc          =*/ simgraph32_mark_rect_dirty_wc,
	/*.mark_rect_dirty_clip        =*/ simgraph32_mark_rect_dirty_clip,
	/*.mark_screen_dirty           =*/ simgraph32_mark_screen_dirty,
	/*.get_screen_size             =*/ simgraph32_get_screen_size,
	/*.set_screen_height           =*/ simgraph32_set_screen_height,
	/*.set_screen_actual_width     =*/ simgraph32_set_screen_actual_width,
	/*.get_best_matching_size      =*/ simgraph32_get_best_matching_size,
	/*.fit_img_to_width            =*/ simgraph32_fit_img_to_width,
	/*.move_scroll_band            =*/ simgraph32_move_scroll_band,
	/*.set_image_procs             =*/ simgraph32_set_image_procs,
	/*.draw_img_aligned            =*/ simgraph32_draw_img_aligned,
	/*.draw_img_aux                =*/ simgraph32_draw_img_aux,
	/*.draw_rezoomed_img_blend     =*/ simgraph32_draw_rezoomed_img_blend,
	/*.draw_rezoomed_img_alpha     =*/ simgraph32_draw_rezoomed_img_alpha,
	/*.draw_color_img              =*/ simgraph32_draw_color_img,
	/*.draw_base_img               =*/ simgraph32_draw_base_img,
	/*.draw_base_img_blend         =*/ simgraph32_draw_base_img_blend,
	/*.draw_base_img_alpha         =*/ simgraph32_draw_base_img_alpha,
	/*.draw_stretch_map            =*/ simgraph32_draw_stretch_map,
	/*.draw_stretch_map_blend      =*/ simgraph32_draw_stretch_map_blend,
	/*.blend_colors                =*/ simgraph32_blend_colors,
	/*.tint_rect                   =*/ simgraph32_tint_rect,
	/*.draw_rect                   =*/ simgraph32_draw_rect,
	/*.draw_rect_clipped           =*/ simgraph32_draw_rect_clipped,
	/*.draw_rect_colors_clipped    =*/ simgraph32_draw_rect_colors_clipped,
	/*.draw_rounded_rect_clipped   =*/ simgraph32_draw_rounded_rect_clipped,
	/*.draw_vline_clipped          =*/ simgraph32_draw_vline_clipped,
	/*.flush_framebuffer           =*/ simgraph32_flush_framebuffer,
	/*.set_cursor_visible          =*/ simgraph32_set_cursor_visible,
	/*.set_default_cursor          =*/ simgraph32_set_default_cursor,
	/*.set_show_load_cursor        =*/ simgraph32_set_show_load_cursor,
	/*.draw_array                  =*/ simgraph32_draw_array,
	/*.font_has_character          =*/ simgraph32_font_has_character,
	/*.get_char_width              =*/ simgraph32_get_char_width,
	/*.get_number_width            =*/ simgraph32_get_number_width,
	/*.get_next_char_with_metrics  =*/ simgraph32_get_next_char_with_metrics,
	/*.get_prev_char_with_metrics  =*/ simgraph32_get_prev_char_with_metrics,
	/*.calc_text_width_n           =*/ simgraph32_calc_text_width_n,
	/*.calc_multiline_text_size    =*/ simgraph32_calc_multiline_text_size,
	/*.calc_text_index_for_width   =*/ simgraph32_calc_text_index_for_width,
	/*.draw_text_clipped_n         =*/ simgraph32_draw_text_clipped_n,
	/*.draw_multiline_text         =*/ simgraph32_draw_multiline_text,
	/*.draw_text_ellipsis_shadowed =*/ simgraph32_draw_text_ellipsis_shadowed,
	/*.draw_text_outlined          =*/ simgraph32_draw_text_outlined,
	/*.draw_text_shadowed          =*/ simgraph32_draw_text_shadowed,
	/*.draw_box3d                  =*/ simgraph32_draw_box3d,
	/*.draw_box3d_clipped          =*/ simgraph32_draw_box3d_clipped,
	/*.draw_textbox3d_clipped      =*/ simgraph32_draw_textbox3d_clipped,
	/*.draw_line                   =*/ simgraph32_draw_line,
	/*.draw_line_dotted            =*/ simgraph32_draw_line_dotted,
	/*.draw_empty_circle           =*/ simgraph32_draw_empty_circle,
	/*.draw_filled_circle          =*/ simgraph32_draw_filled_circle,
	/*.draw_bezier                 =*/ simgraph32_draw_bezier,
	/*.draw_right_triangle         =*/ simgraph32_draw_right_triangle,
	/*.draw_signal_direction       =*/ simgraph32_draw_signal_direction,
	/*.take_screenshot             =*/ simgraph32_take_screenshot,
	/*.set_clip_rect               =*/ simgraph32_set_clip_rect,
	/*.get_clip_rect               =*/ simgraph32_get_clip_rect,
	/*.push_clip_rect              =*/ simgraph32_push_clip_rect,
	/*.swap_clip_rect              =*/ simgraph32_swap_clip_rect,
	/*.pop_clip_rect               =*/ simgraph32_pop_clip_rect,
	/*.add_poly_clip               =*/ simgraph32_add_poly_clip,
	/*.clear_all_poly_clip         =*/ simgraph32_clear_all_poly_clip,
	/*.activate_ribi_clip          =*/ simgraph32_activate_ribi_clip,
};


static PIXVAL simgraph32_palette_lookup(palette_index_t idx)
{
	// same contract as simgraph16: the day table is the palette
	return specialcolormap_all_day[idx];
}

static palette_index_t simgraph32_palette_indexof(PIXVAL color)
{
	for(  int i = 0;  i <= 0xFF;  i++  ) {
		if(  specialcolormap_all_day[i] == color  ) {
			return (palette_index_t)i;
		}
	}
	return 0;
}


static rgb888_t simgraph32_get_color_rgb(palette_index_t idx)
{
	// special_pal has 224 rgb colors
	if (idx < SPECIAL_COLOR_COUNT) {
		return special_pal[idx];
	}

	// if it uses one of the special light colours it's under display_day_lights
	if (idx < SPECIAL_COLOR_COUNT + LIGHT_COUNT) {
		return display_day_lights[idx - SPECIAL_COLOR_COUNT];
	}

	// Return black for anything else
	return rgb888_t{0,0,0};
}

static rgb888_t simgraph32_get_pixval_rgb(PIXVAL c)
{
	// ARGB8888 -> rgb888, no quantisation anywhere on the way
	return { (uint8)((c >> 16) & 0xFF), (uint8)((c >> 8) & 0xFF), (uint8)(c & 0xFF) };
}

static void simgraph32_env_t_rgb_to_system_colors()
{
	// get system colours for the default colours or settings.xml
	env_t::default_window_title_color = get_system_color(env_t::default_window_title_color_rgb);
	env_t::front_window_text_color    = get_system_color(env_t::front_window_text_color_rgb);
	env_t::bottom_window_text_color   = get_system_color(env_t::bottom_window_text_color_rgb);
	env_t::tooltip_color              = get_system_color(env_t::tooltip_color_rgb);
	env_t::tooltip_textcolor          = get_system_color(env_t::tooltip_textcolor_rgb);
	env_t::cursor_overlay_color       = get_system_color(env_t::cursor_overlay_color_rgb);
	env_t::background_color           = get_system_color(env_t::background_color_rgb);
}

static scr_coord_val simgraph32_set_base_raster_width(scr_coord_val new_raster)
{
	const scr_coord_val old = g_simgraph32.base_tile_raster_width;
	g_simgraph32.base_tile_raster_width = new_raster;
	g_simgraph32.tile_raster_width =
		(new_raster * g_simgraph32.zoom_num[zoom_factor32]) / g_simgraph32.zoom_den[zoom_factor32];
	rezoom32();
	return old;
}

void set_zoom_factor(int z)
{
	// do not zoom beyond 4 pixels
	if(  (g_simgraph32.base_tile_raster_width * g_simgraph32.zoom_num[z]) / g_simgraph32.zoom_den[z] > 4  ) {
		zoom_factor32 = z;
		g_simgraph32.tile_raster_width =
			(g_simgraph32.base_tile_raster_width * g_simgraph32.zoom_num[zoom_factor32]) / g_simgraph32.zoom_den[zoom_factor32];
		rezoom32();
	}
}

static int simgraph32_zoom_factor_up()
{
	if(  zoom_factor32 > 0  ) {
		set_zoom_factor( (int)zoom_factor32 - 1 );
		return true;
	}
	return false;
}

static int simgraph32_zoom_factor_down()
{
	if(  zoom_factor32 < MAX_ZOOM_FACTOR  ) {
		set_zoom_factor( (int)zoom_factor32 + 1 );
		return true;
	}
	return false;
}






static scr_size simgraph32_get_screen_size()
{
	return scr_size{ disp_actual_width, disp_height };
}

static void simgraph32_set_screen_height(scr_coord_val h)
{
	disp_height = h;
}

static void simgraph32_set_screen_actual_width(scr_coord_val w)
{
	disp_actual_width = w;
}

static void simgraph32_set_daynight_level(int night)
{
	if(  night != night_shift  ) {
		night_shift = night;
		calc_base_pal_from_night_shift( night );
		// rgbmap_all_day is a snapshot of the night==0 tables and is
		// deliberately NOT refreshed here - simgraph16_set_daynight_level()
		// does not touch it either. Refreshing it would make the daytime
		// family identical to the colored one.
	}
}

static void simgraph32_set_player_color_scheme(const int player, const uint8 col1, const uint8 col2)
{
	if(  player_offsets[player][0] != col1  ||  player_offsets[player][1] != col2  ) {
		player_offsets[player][0] = col1;
		player_offsets[player][1] = col2;

		if(  player == player_day  ||  player == player_night  ) {
			// recalculate the day tables and save them, then restore the
			// current night shift - exactly simgraph16_set_player_color_scheme()
			calc_base_pal_from_night_shift( 0 );
			memcpy( rgbmap_all_day, rgbmap_day_night, RGBMAPSIZE * sizeof(PIXVAL) );
			if(  night_shift != 0  ) {
				calc_base_pal_from_night_shift( night_shift );
			}
			// calc_base_pal_from_night_shift resets player_night to 0
			player_day = player_night;
		}
	}
}


static image_id simgraph32_register_image(const image_t *image)
{
	if(  anz_images == alloc_images  ) {
		const image_id newsize = alloc_images == 0 ? 128 : alloc_images * 2;
		imd32 *const grown = MALLOCN( imd32, newsize );
		MEMZERON( grown, newsize );
		if(  images != NULL  ) {
			memcpy( grown, images, anz_images * sizeof(imd32) );
			free( images );
		}
		images = grown;
		alloc_images = newsize;
	}

	const image_id n = anz_images++;
	imd32 &d = images[n];

	d.base_x = image->x;
	d.base_y = image->y;
	d.base_w = image->w;
	d.base_h = image->h;
	d.x      = image->x;
	d.y      = image->y;
	d.w      = image->w;
	d.h      = image->h;
	d.len    = (uint32)image->len;
	d.recode_flags = FLAG_REZOOM | (image->zoomable ? FLAG_ZOOMABLE : 0);

	// find out if there are really player colors, exactly as simgraph16 does at
	// registration. draw_img_aux needs it to choose between the player cache and
	// the plain one; without it a non-player image asked for with a player number
	// would find a NULL cache and draw nothing.
	{
		const STORED_PIXVAL *src = image->data;
		for(  sint16 y = 0;  y < image->h;  ++y  ) {
			uint16 runlen = *src++;
			do {
				runlen = *src++;
				if(  runlen & TRANSPARENT_RUN  ) {
					d.recode_flags |= FLAG_HAS_TRANSPARENT_COLOR;
					runlen &= ~TRANSPARENT_RUN;
				}
				while(  runlen--  ) {
					const STORED_PIXVAL s = *src++;
					if(  s >= 0x8000  &&  s < 0x8010  ) {
						d.recode_flags |= FLAG_HAS_PLAYER_COLOR;
					}
				}
				runlen = *src++;
			} while(  runlen != 0  );
		}
	}
	d.zoom_data    = NULL;
	d.player_flags = 0xFFFF; // every player needs a recode

	// base_data keeps the pak's own 16 bit storage words, verbatim
	d.base_data = MALLOCN( STORED_PIXVAL, d.len );
	memcpy( d.base_data, image->data, d.len * sizeof(STORED_PIXVAL) );

	for(  int p = 0;  p < MAX_PLAYER_COUNT;  p++  ) {
		d.data[p] = NULL;
	}

	return n;
}


/**
 * Produces (or refreshes) the ARGB8888 screen space cache of image @p n for
 * player @p player_nr. The cache is PIXVAL wide; base_data is not touched.
 */
static void recode_img(const image_id n, const sint8 player_nr)
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &recode_img_mutex32 );
	if(  (images[n].player_flags & (1<<player_nr)) == 0  ) {
		// other thread did already the re-code...
		pthread_mutex_unlock( &recode_img_mutex32 );
		return;
	}
#endif
	imd32 &d = images[n];

	if(  d.data[player_nr] == NULL  ) {
		d.data[player_nr] = MALLOCN( PIXVAL, d.len );
	}

	// pick the player colours into the table, exactly as simgraph16 does
	for(  int i = 0;  i < 8;  i++  ) {
		rgbmap_day_night[0x8000 + i] = specialcolormap_day_night[player_offsets[player_nr][0] + i];
		rgbmap_day_night[0x8008 + i] = specialcolormap_day_night[player_offsets[player_nr][1] + i];
	}
	player_night = player_nr;

	recode_img_src_target( d.h, current_stored( n ), d.data[player_nr] );
	d.player_flags &= ~(1 << player_nr);
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &recode_img_mutex32 );
#endif
}


static bool simgraph32_take_screenshot(const scr_rect &area, const char *filename)
{
	// now save the screenshot
	scr_rect clipped_area = area;
	clipped_area.clip(scr_rect(0, 0, disp_actual_width, disp_height));

	raw_image_t img(clipped_area.w, clipped_area.h, raw_image_t::FMT_RGB888);

	for (scr_coord_val y = 0; y < clipped_area.h; ++y) {
		uint8 *dst = img.access_pixel(0, y);
		const PIXVAL *row = textur + (clipped_area.x + 0) + (clipped_area.y + y) * disp_width;

		for (scr_coord_val x = 0; x < clipped_area.w; ++x) {
			// the framebuffer is ARGB8888; the file is RGB888. This is the
			// only place the two representations meet, and nothing is
			// quantised on the way.
			const rgb888_t pixel = simgraph32_get_pixval_rgb(*row++);
			*dst++ = pixel.r;
			*dst++ = pixel.g;
			*dst++ = pixel.b;
		}
	}

	return img.write_png(filename);
}


static scr_rect simgraph32_get_image_offset(image_id)
{
	return scr_rect{ 0, 0, 0, 0 };
}


static scr_rect simgraph32_get_base_image_offset(image_id)
{
	return scr_rect{ 0, 0, 0, 0 };
}


static clip_dimension simgraph32_get_clip_rect(CLIP_NUM_DEF0)
{
	return CR32.clip_rect;
}

static void simgraph32_set_clip_rect(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h  CLIP_NUM_DEF, bool fit)
{
	if (!fit) {
		clip_wh32( &x, &w, 0, disp_width);
		clip_wh32( &y, &h, 0, disp_height);
	}
	else {
		clip_wh32( &x, &w, CR32.clip_rect.x, CR32.clip_rect.xx);
		clip_wh32( &y, &h, CR32.clip_rect.y, CR32.clip_rect.yy);
	}

	CR32.clip_rect.x  = x;
	CR32.clip_rect.y  = y;
	CR32.clip_rect.w  = w;
	CR32.clip_rect.h  = h;
	CR32.clip_rect.xx = x + w;
	CR32.clip_rect.yy = y + h;
}





/**
 * The plain image entry point of the interface.
 *
 * A THIN ADAPTER, not a reimplementation: it does what simgraph16_draw_img_aux
 * does before drawing - pick the player cache when the image really has player
 * colours, otherwise bring the plain cache up to date - and then hands over to
 * display_img_aux32, which is the certified geometry and dispatch.
 */
static void simgraph32_draw_img_aux(const image_id n, scr_coord_val xp, scr_coord_val yp, const sint8 player_nr_raw, const bool, const bool dirty  CLIP_NUM_DEF)
{
	if(  n >= anz_images  ) {
		return;
	}
	// only use player images if needed
	const sint8 use_player = (sint8)(((images[n].recode_flags & FLAG_HAS_PLAYER_COLOR) != 0) * player_nr_raw);

	if(  use_player > 0  &&  use_player < MAX_PLAYER_COUNT  ) {
		// player colour images are rezoomed and recoloured in draw_color_img
		if(  images[n].data[use_player] == NULL  ) {
			return;
		}
		display_img_aux32( n, xp, yp, use_player, dirty   CLIP_NUM_PAR);
		return;
	}

	if(  images[n].recode_flags & FLAG_REZOOM  ) {
		rezoom_img32( n );
		recode_img( n, 0 );
	}
	else if(  images[n].player_flags & 1  ) {
		recode_img( n, 0 );
	}
	if(  images[n].data[0] == NULL  ) {
		return;
	}
	display_img_aux32( n, xp, yp, 0, dirty   CLIP_NUM_PAR);
}

static void simgraph32_draw_color_img(const image_id n, scr_coord_val xp, scr_coord_val yp, const sint8 player_nr_raw, const bool daynight, const bool dirty  CLIP_NUM_DEF)
{
	if(  n >= anz_images  ) {
		return;
	}
	const sint8 player_nr = (player_nr_raw >= 0 && player_nr_raw < MAX_PLAYER_COUNT) ? player_nr_raw : 0;

	// first: size check
	if(  images[n].recode_flags & FLAG_REZOOM  ) {
		rezoom_img32( n );
	}

	if(  daynight  ||  night_shift == 0  ) {
		// the recoded screen cache can serve this, exactly as simgraph16 prefers
		if(  images[n].player_flags & (1 << player_nr)  ) {
			recode_img( n, player_nr );
		}
		display_img_aux32( n, xp, yp, player_nr, dirty   CLIP_NUM_PAR);
		return;
	}

	// player colour substitution WITHOUT day/night. The cache cannot serve it,
	// so the original stored data is read - this is the colored/daytime family.
	const scr_coord_val x = images[n].x + xp;
	      scr_coord_val y = images[n].y + yp;
	const scr_coord_val w = images[n].w;
	      scr_coord_val h = images[n].h;

	if(  h <= 0  ||  x >= CR32.clip_rect.xx  ||  y >= CR32.clip_rect.yy  ||
	     x + w <= CR32.clip_rect.x  ||  y + h <= CR32.clip_rect.y  ) {
		return;
	}
	if(  dirty  ) {
		simgraph32_mark_rect_dirty_wc( x, y, x + w - 1, y + h - 1 );
	}

	activate_player_color32( player_nr, daynight );

	// color replacement needs the original data => sp points to non-cached data
	const STORED_PIXVAL *sp = current_stored( n );

	// clip top/bottom
	scr_coord_val yoff = clip_wh32( &y, &h, CR32.clip_rect.y, CR32.clip_rect.yy );
	if(  h > 0  ) {
		while(  yoff  ) {
			yoff--;
			do {
				// clear run + colored run + next clear run
				++sp;
				sp += (*sp) & (~TRANSPARENT_RUN);
				sp++;
			} while(  *sp  );
			sp++;
		}

		if(  CR32.number_of_clips > 0  ) {
			daynight ? display_color_img_pc32<PixelOps32Colored>( h, x, y, sp   CLIP_NUM_PAR)
			         : display_color_img_pc32<PixelOps32Daytime>( h, x, y, sp   CLIP_NUM_PAR);
		}
		else {
			daynight ? display_color_img_wc32<PixelOps32Colored>( sp, x, y, h   CLIP_NUM_PAR)
			         : display_color_img_wc32<PixelOps32DaytimeWC>( sp, x, y, h   CLIP_NUM_PAR);
		}
	}
}

static scr_size simgraph32_get_best_matching_size(const image_id, sint16)
{
	return scr_size(32, 32); // default size
}


static void simgraph32_draw_base_img(const image_id n, scr_coord_val xp, scr_coord_val yp, const sint8 player_nr, const bool daynight, const bool dirty  CLIP_NUM_DEF)
{
	if(  g_simgraph32.tile_raster_width == g_simgraph32.base_tile_raster_width  ) {
		// same size => use standard routine
		simgraph32_draw_color_img( n, xp, yp, player_nr, daynight, dirty  CLIP_NUM_DEFAULT );
		return;
	}
	if(  n >= anz_images  ) {
		return;
	}

	// the zoomed path: the BASE image is drawn unzoomed, from stored data, so
	// this is where the colored family becomes production code
	const scr_coord_val x = images[n].base_x + xp;
	      scr_coord_val y = images[n].base_y + yp;
	const scr_coord_val w = images[n].base_w;
	      scr_coord_val h = images[n].base_h;

	if(  h <= 0  ||  x >= CR32.clip_rect.xx  ||  y >= CR32.clip_rect.yy  ||
	     x + w <= CR32.clip_rect.x  ||  y + h <= CR32.clip_rect.y  ) {
		return;
	}
	if(  dirty  ) {
		simgraph32_mark_rect_dirty_wc( x, y, x + w - 1, y + h - 1 );
	}

	activate_player_color32( (player_nr >= 0 && player_nr < MAX_PLAYER_COUNT) ? player_nr : 0, daynight );

	const STORED_PIXVAL *sp = images[n].base_data;

	scr_coord_val yoff = clip_wh32( &y, &h, CR32.clip_rect.y, CR32.clip_rect.yy );
	if(  h > 0  ) {
		while(  yoff  ) {
			yoff--;
			do {
				sp++;
				sp += (*sp) & (~TRANSPARENT_RUN);
				sp++;
			} while(  *sp  );
			sp++;
		}
		if(  CR32.number_of_clips > 0  ) {
			daynight ? display_color_img_pc32<PixelOps32Colored>( h, x, y, sp   CLIP_NUM_PAR)
			         : display_color_img_pc32<PixelOps32Daytime>( h, x, y, sp   CLIP_NUM_PAR);
		}
		else {
			daynight ? display_color_img_wc32<PixelOps32Colored>( sp, x, y, h   CLIP_NUM_PAR)
			         : display_color_img_wc32<PixelOps32DaytimeWC>( sp, x, y, h   CLIP_NUM_PAR);
		}
	}
}

/**
 * Pick the largest zoom step whose width still fits, and rezoom this one image
 * to it. A transposition of simgraph16_fit_img_to_width(): the zoom table, the
 * flags and the resampler are the ones U7A put in place.
 */
static void simgraph32_fit_img_to_width( const image_id n, sint16 new_w)
{
	if(  n < anz_images  &&  images[n].base_h > 0  &&  images[n].w != new_w  ) {
		const uint32 old_zoom_factor = zoom_factor32;
		for(  int i = 0;  i <= MAX_ZOOM_FACTOR;  i++  ) {
			const int zoom_w = (images[n].base_w * g_simgraph32.zoom_num[i]) / g_simgraph32.zoom_den[i];
			if(  zoom_w <= new_w  ) {
				const uint8 old_zoom_flag = images[n].recode_flags & FLAG_ZOOMABLE;
				images[n].recode_flags |= FLAG_REZOOM | FLAG_ZOOMABLE;
				zoom_factor32 = (uint32)i;
				rezoom_img32( n );
				images[n].recode_flags &= ~FLAG_ZOOMABLE;
				images[n].recode_flags |= old_zoom_flag;
				zoom_factor32 = old_zoom_factor;
				return;
			}
		}
	}
}











static PIXVAL simgraph32_blend_colors(PIXVAL background, PIXVAL foreground, int percent_blend)
{
	return blend_colors_alpha32_argb( background, foreground, (percent_blend * 32) / 100 );
}


static void simgraph32_tint_rect(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL colval, int percent_blend)
{
	if(  textur == NULL  ) {
		return;
	}
	if(  clip_lr32( &xp, &w, CR32_0.clip_rect.x, CR32_0.clip_rect.xx )  &&
	     clip_lr32( &yp, &h, CR32_0.clip_rect.y, CR32_0.clip_rect.yy )  ) {
		const int alpha = (percent_blend * 64) / 100;

		switch(  alpha  ) {
			case 0: // nothing to do
				break;

			case 16:
			case 32:
			case 48:
			{
				// family B, the same three quarters the outline uses
				for(  scr_coord_val y = 0;  y < h;  y++  ) {
					PIXVAL *dest = textur + xp + (yp + y) * disp_width;
					for(  scr_coord_val x = 0;  x < w;  x++  ) {
						switch(  alpha  ) {
							case 16: dest[x] = blend_b25_argb::blend( dest[x], colval ); break;
							case 32: dest[x] = blend_b50_argb::blend( dest[x], colval ); break;
							default: dest[x] = blend_b75_argb::blend( dest[x], colval ); break;
						}
					}
				}
			}
			break;

			case 64: // opaque
				simgraph32_draw_rect( xp, yp, w, h, colval, false );
				break;

			default:
				// family L: any percentage, interpolated in 64 steps
				for(  ;  h > 0;  yp++, h--  ) {
					PIXVAL *dest = textur + yp * disp_width + xp;
					for(  scr_coord_val x = 0;  x < w;  x++  ) {
						dest[x] = tint_lerp_argb( dest[x], colval, alpha );
					}
				}
				break;
		}
	}
}


static void simgraph32_draw_rect(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, bool)
{
	if(  textur == NULL  ) {
		return;
	}
	// clip against the framebuffer; there is no clip rect machinery yet
	if(  xp < 0  ) { w += xp; xp = 0; }
	if(  yp < 0  ) { h += yp; yp = 0; }
	if(  xp + w > disp_actual_width  ) { w = disp_actual_width - xp; }
	if(  yp + h > disp_height       ) { h = disp_height - yp;       }

	for(  scr_coord_val y = 0;  y < h;  y++  ) {
		PIXVAL *p = textur + (size_t)(yp + y) * disp_width + xp;
		for(  scr_coord_val x = 0;  x < w;  x++  ) {
			*p++ = color;
		}
	}
}










static void simgraph32_draw_array(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, const PIXVAL *arr)
{
	if(  textur == NULL  ||  arr == NULL  ) {
		return;
	}
	simgraph32_mark_rect_dirty_wc( xp, yp, xp + w - 1, yp + h - 1 );

	for(  scr_coord_val y = 0;  y < h;  y++  ) {
		if(  yp + y < 0  ||  yp + y >= disp_height  ) {
			continue;
		}
		PIXVAL *p = textur + (size_t)(yp + y) * disp_width + xp;
		for(  scr_coord_val x = 0;  x < w;  x++  ) {
			if(  xp + x >= 0  &&  xp + x < disp_actual_width  ) {
				p[x] = arr[(size_t)y * w + x];
			}
		}
	}
}




















static void simgraph32_flush_framebuffer()
{
	if(  textur != NULL  ) {
		dr_textur( 0, 0, disp_actual_width, disp_height );
	}
}




static bool simgraph32_init(scr_size window_size, sint16 full_screen)
{
	disp_actual_width = window_size.w;
	disp_height       = window_size.h;

	// let the backend open the window and tell us the real pitch
	disp_width = dr_os_open(window_size, full_screen);
	if(  disp_width <= 0  ) {
		dr_fatal_notify( "Cannot open window!" );
		return false;
	}

	textur = dr_textur_init();
	if(  textur == NULL  ) {
		dr_fatal_notify( "Cannot allocate 32 bit framebuffer!" );
		return false;
	}

	init_colour_tables();
	dirty_state_alloc();

	return true;
}

static bool simgraph32_is_display_init()
{
	return textur != NULL;
}

static void simgraph32_free_all_images_above(image_id above)
{
	while(  anz_images > above  ) {
		anz_images--;
		imd32 &d = images[anz_images];
		for(  int p = 0;  p < MAX_PLAYER_COUNT;  p++  ) {
			free( d.data[p] );
			d.data[p] = NULL;
		}
		free( d.base_data );
		d.base_data = NULL;
		d.len = 0;
	}
}

static void simgraph32_exit()
{
	dr_os_close();
	dirty_state_free();
	textur = NULL;
}

static void simgraph32_on_window_resized(scr_size new_window_size)
{
	disp_actual_width = max( 16, new_window_size.w );
	if(  new_window_size.h<=0  ) {
		new_window_size.h = 64;
	}
	// only resize, if internal values are different
	if (disp_width != new_window_size.w || disp_height != new_window_size.h) {
		scr_coord_val new_pitch = dr_textur_resize(&textur, new_window_size.w, new_window_size.h);
		if(  new_pitch!=disp_width  ||  disp_height != new_window_size.h) {
			disp_width = new_pitch;
			disp_height = new_window_size.h;

			// the dirty map for the new extent
			dirty_state_alloc();

			// the clip rectangle must follow the framebuffer: the writers
			// trust it, and SDL hands out an exact-size buffer
			simgraph32_set_clip_rect(0, 0, disp_actual_width, disp_height  CLIP_NUM_DEFAULT, false);
		}

		simgraph32_mark_screen_dirty();
		dirty_old_clear();
	}
}

void display_snapshot()
{
}









/**
 * Alignment arithmetic over the image's current geometry, then the ordinary
 * colour draw. Pure delegation: simgraph16 reaches its own draw_color_img
 * through the vtable here, and this is that renderer's own.
 */
static void simgraph32_draw_img_aligned(const image_id n, scr_rect area, int align, bool dirty)
{
	if(  n >= anz_images  ) {
		return;
	}
	scr_coord_val x, y;

	// align the image horizontally
	x = area.x;
	if(  (align & ALIGN_RIGHT) == ALIGN_CENTER_H  ) {
		x -= images[n].x;
		x += (area.w - images[n].w) / 2;
	}
	else if(  (align & ALIGN_RIGHT) == ALIGN_RIGHT  ) {
		x = area.get_right() - images[n].x - images[n].w;
	}

	// align the image vertically
	y = area.y;
	if(  (align & ALIGN_BOTTOM) == ALIGN_CENTER_V  ) {
		y -= images[n].y;
		y += (area.h - images[n].h) / 2;
	}
	else if(  (align & ALIGN_BOTTOM) == ALIGN_BOTTOM  ) {
		y = area.get_bottom() - images[n].y - images[n].h;
	}

	simgraph32_draw_color_img( n, x, y, 0, false, dirty  CLIP_NUM_DEFAULT );
}


static image_id simgraph32_get_image_count()
{
	return anz_images;
}

#ifdef MULTI_THREAD
/* One clipping context, whatever CLIP_NUM says: see the threading note
 * above the clipping state. The parameter is accepted and ignored so the
 * vtable signature matches, and that limitation is reported, not hidden. */
static void simgraph32_add_poly_clip(int x0, int y0, int x1, int y1, int ribi  CLIP_NUM_DEF)
{
	if(  CR32.number_of_clips < MAX_POLY_CLIPS32  ) {
		CR32.poly_clips[CR32.number_of_clips].clip_from_to( x0, y0, x1, y1, ribi & 16 );
		CR32.clip_ribi[CR32.number_of_clips] = ribi & 15;
		CR32.number_of_clips++;
	}
}

static void simgraph32_clear_all_poly_clip(const sint8 clip_num)
{
	CR32.number_of_clips = 0;
	CR32.active_ribi     = 15;
}

static void simgraph32_activate_ribi_clip(int ribi  CLIP_NUM_DEF)
{
	CR32.active_ribi = ribi;
}
#else
static void simgraph32_add_poly_clip(int x0, int y0, int x1, int y1, int ribi)
{
	if(  CR32.number_of_clips < MAX_POLY_CLIPS32  ) {
		CR32.poly_clips[CR32.number_of_clips].clip_from_to( x0, y0, x1, y1, ribi & 16 );
		CR32.clip_ribi[CR32.number_of_clips] = ribi & 15;
		CR32.number_of_clips++;
	}
}

static void simgraph32_clear_all_poly_clip()
{
	CR32.number_of_clips = 0;
	CR32.active_ribi     = 15;
}

static void simgraph32_activate_ribi_clip(int ribi)
{
	CR32.active_ribi = ribi;
}
#endif




/* ---------------- drawing primitives: the base three -----------------------
 *
 * simgraph16 has its own versions of these, but they are 16 bit specific: a
 * `rep stos` assembler path and a fill that packs two pixels into a uint32,
 * neither of which means anything once a screen pixel is four bytes. So the
 * three base writers are written here for ARGB8888, and everything built on
 * top of them is the historical geometry, moved across unchanged.
 *
 * Colour: every one of these takes a plain PIXVAL and writes it unmodified.
 * No blend family is involved and none is introduced - the caller has already
 * resolved the colour.
 *
 * Dirty marking: simgraph16 marks tiles here. This renderer has no dirty-tile
 * map yet (mark_rect_dirty_* are still stubs), so the flag is accepted and
 * ignored, exactly as the rest of simgraph32 already does.
 */

/// one pixel, clipped against the current clip rectangle
static void display_pixel32(scr_coord_val x, scr_coord_val y, PIXVAL colour)
{
	if(  x >= CR32_0.clip_rect.x  &&  x < CR32_0.clip_rect.xx  &&
	     y >= CR32_0.clip_rect.y  &&  y < CR32_0.clip_rect.yy  ) {
		textur[x + y * disp_width] = colour;
		mark_tile_dirty32( x >> DIRTY_TILE_SHIFT, y >> DIRTY_TILE_SHIFT );
	}
}


/// a filled rectangle, clipped against an explicit rectangle
static void display_fb_internal32(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL colval, bool dirty, scr_coord_val cL, scr_coord_val cR, scr_coord_val cT, scr_coord_val cB)
{
	if(  clip_lr32( &xp, &w, cL, cR )  &&  clip_lr32( &yp, &h, cT, cB )  ) {
		PIXVAL *p = textur + xp + yp * disp_width;
		const int dx = disp_width - w;

		if(  dirty  ) {
			mark_rect_dirty_nc( xp, yp, xp + w - 1, yp + h - 1 );
		}
		do {
			PIXVAL *const fillend = p + w;
			std::fill( p, fillend, colval );
			p = fillend + dx;
		} while(  --h  );
	}
}


/// a vertical line, clipped against an explicit rectangle
static void display_vl_internal32(const scr_coord_val xp, scr_coord_val yp, scr_coord_val h, const PIXVAL colval, int dirty, scr_coord_val cL, scr_coord_val cR, scr_coord_val cT, scr_coord_val cB)
{
	if(  xp >= cL  &&  xp < cR  &&  clip_lr32( &yp, &h, cT, cB )  ) {
		PIXVAL *p = textur + xp + yp * disp_width;

		if(  dirty  ) {
			mark_rect_dirty_nc( xp, yp, xp, yp + h - 1 );
		}
		do {
			*p = colval;
			p += disp_width;
		} while(  --h != 0  );
	}
}


/// the unclipped vertical line, as simgraph16 has it
static void simgraph32_draw_vline(const scr_coord_val xp, scr_coord_val yp, scr_coord_val h, const PIXVAL colour, bool dirty)
{
	display_vl_internal32( xp, yp, h, colour, dirty, 0, disp_width, 0, disp_height );
}

static void simgraph32_draw_rect_clipped(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, bool dirty  CLIP_NUM_DEF)
{
	display_fb_internal32( xp, yp, w, h, color, dirty, CR32.clip_rect.x, CR32.clip_rect.xx, CR32.clip_rect.y, CR32.clip_rect.yy );
}

static void simgraph32_draw_vline_clipped(const scr_coord_val xp, scr_coord_val yp, scr_coord_val h, const PIXVAL color, bool dirty  CLIP_NUM_DEF)
{
	display_vl_internal32( xp, yp, h, color, dirty, CR32.clip_rect.x, CR32.clip_rect.xx, CR32.clip_rect.y, CR32.clip_rect.yy );
}

static void simgraph32_draw_rect_colors_clipped(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL *colors, scr_coord_val num_colors, bool horizontal, bool dirty  CLIP_NUM_DEF)
{
	if (horizontal) {
		scr_coord_val last_w = 0;
		for (scr_coord_val i = 0; i < num_colors; i++) {
			scr_coord_val next_w = (w * (1 + i)) / num_colors;
			display_fb_internal32(xp+last_w, yp, next_w-last_w, h, colors[i], dirty, CR32.clip_rect.x, CR32.clip_rect.xx, CR32.clip_rect.y, CR32.clip_rect.yy);
			last_w = next_w;
		}
	}
	else {
		scr_coord_val last_h = 0;
		for (scr_coord_val i = 0; i < num_colors; i++) {
			scr_coord_val next_h = (h * (1 + i)) / num_colors;
			display_fb_internal32(xp, yp+last_h, w, next_h-last_h, colors[i], dirty, CR32.clip_rect.x, CR32.clip_rect.xx, CR32.clip_rect.y, CR32.clip_rect.yy);
			last_h = next_h;
		}
	}
}

static void simgraph32_draw_line(const scr_coord_val x, const scr_coord_val y, const scr_coord_val xx, const scr_coord_val yy, const PIXVAL colval)
{
	int i, steps;
	sint64 xp, yp;
	sint64 xs, ys;

	const int dx = xx - x;
	const int dy = yy - y;

	steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
	if (steps == 0) {
		steps = 1;
	}

	xs = ((sint64)dx << 16) / steps;
	ys = ((sint64)dy << 16) / steps;

	xp = (sint64)x << 16;
	yp = (sint64)y << 16;

	for (i = 0; i <= steps; i++) {
		display_pixel32(xp >> 16, yp >> 16, colval);
		xp += xs;
		yp += ys;
	}
}

static void simgraph32_draw_line_dotted(const scr_coord_val x, const scr_coord_val y, const scr_coord_val xx, const scr_coord_val yy, const scr_coord_val draw, const scr_coord_val dontDraw, const PIXVAL colval)
{
	int i, steps;
	sint64 xp, yp;
	sint64 xs, ys;
	int counter=0;
	bool mustDraw=true;

	const int dx = xx - x;
	const int dy = yy - y;

	steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
	if (steps == 0) {
		steps = 1;
	}

	xs = ((sint64)dx << 16) / steps;
	ys = ((sint64)dy << 16) / steps;

	xp = (sint64)x << 16;
	yp = (sint64)y << 16;

	for(  i = 0;  i <= steps;  i++  ) {
		counter ++;
		if(  mustDraw  ) {
			if(  counter == draw  ) {
				mustDraw = !mustDraw;
				counter = 0;
			}
		}
		if(  !mustDraw  ) {
			if(  counter == dontDraw  ) {
				mustDraw=!mustDraw;
				counter=0;
			}
		}

		if(  mustDraw  ) {
			display_pixel32( xp >> 16, yp >> 16, colval );
		}
		xp += xs;
		yp += ys;
	}
}

static void simgraph32_draw_empty_circle( scr_coord_val x0, scr_coord_val  y0, int radius, const PIXVAL colval )
{
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	display_pixel32( x0, y0 + radius, colval );
	display_pixel32( x0, y0 - radius, colval );
	display_pixel32( x0 + radius, y0, colval );
	display_pixel32( x0 - radius, y0, colval );

	while(x < y) {
		// ddF_x == 2 * x + 1;
		// ddF_y == -2 * y;
		// f == x*x + y*y - radius*radius + 2*x - y + 1;
		if(f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}

		x++;
		ddF_x += 2;
		f += ddF_x;

		display_pixel32( x0 + x, y0 + y, colval );
		display_pixel32( x0 - x, y0 + y, colval );
		display_pixel32( x0 + x, y0 - y, colval );
		display_pixel32( x0 - x, y0 - y, colval );
		display_pixel32( x0 + y, y0 + x, colval );
		display_pixel32( x0 - y, y0 + x, colval );
		display_pixel32( x0 + y, y0 - x, colval );
		display_pixel32( x0 - y, y0 - x, colval );
	}
}

static void simgraph32_draw_filled_circle(scr_coord_val x0, scr_coord_val y0, int radius, const PIXVAL colval)
{
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	display_fb_internal32( x0-radius, y0, radius+radius+1, 1, colval, false, CR32_0.clip_rect.x, CR32_0.clip_rect.xx, CR32_0.clip_rect.y, CR32_0.clip_rect.yy );
	display_pixel32( x0, y0 + radius, colval );
	display_pixel32( x0, y0 - radius, colval );
	display_pixel32( x0 + radius, y0, colval );
	display_pixel32( x0 - radius, y0, colval );

	while(x < y) {
		// ddF_x == 2 * x + 1;
		// ddF_y == -2 * y;
		// f == x*x + y*y - radius*radius + 2*x - y + 1;
		if(f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}

		x++;
		ddF_x += 2;
		f += ddF_x;
		display_fb_internal32( x0-x, y0+y, x+x, 1, colval, false, CR32_0.clip_rect.x, CR32_0.clip_rect.xx, CR32_0.clip_rect.y, CR32_0.clip_rect.yy );
		display_fb_internal32( x0-x, y0-y, x+x, 1, colval, false, CR32_0.clip_rect.x, CR32_0.clip_rect.xx, CR32_0.clip_rect.y, CR32_0.clip_rect.yy );

		display_fb_internal32( x0-y, y0+x, y+y, 1, colval, false, CR32_0.clip_rect.x, CR32_0.clip_rect.xx, CR32_0.clip_rect.y, CR32_0.clip_rect.yy );
		display_fb_internal32( x0-y, y0-x, y+y, 1, colval, false, CR32_0.clip_rect.x, CR32_0.clip_rect.xx, CR32_0.clip_rect.y, CR32_0.clip_rect.yy );
	}
//	mark_rect_dirty_wc( x0-radius, y0-radius, x0+radius+1, y0+radius+1 );
}

static void simgraph32_draw_rounded_rect_clipped(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, PIXVAL color, bool dirty)
{
	simgraph32_draw_rect_clipped(xp+2,   yp,   w-4, h,   color, dirty CLIP_NUM_DEFAULT);
	simgraph32_draw_rect_clipped(xp,     yp+2, 1,   h-4, color, dirty CLIP_NUM_DEFAULT);
	simgraph32_draw_rect_clipped(xp+1,   yp+1, 1,   h-2, color, dirty CLIP_NUM_DEFAULT);
	simgraph32_draw_rect_clipped(xp+w-1, yp+2, 1,   h-4, color, dirty CLIP_NUM_DEFAULT);
	simgraph32_draw_rect_clipped(xp+w-2, yp+1, 1,   h-2, color, dirty CLIP_NUM_DEFAULT);
}

static void simgraph32_draw_box3d(scr_coord_val x1, scr_coord_val y1, scr_coord_val w, scr_coord_val h, PIXVAL tl_color, PIXVAL rd_color, bool dirty)
{
	simgraph32_draw_rect(x1, y1,         w, 1, tl_color, dirty);
	simgraph32_draw_rect(x1, y1 + h - 1, w, 1, rd_color, dirty);

	h -= 2;

	simgraph32_draw_vline(x1,         y1 + 1, h, tl_color, dirty);
	simgraph32_draw_vline(x1 + w - 1, y1 + 1, h, rd_color, dirty);
}

static void simgraph32_draw_box3d_clipped(scr_coord_val x1, scr_coord_val y1, scr_coord_val w, scr_coord_val h, PIXVAL tl_color, PIXVAL rd_color)
{
	simgraph32_draw_rect_clipped(x1, y1,         w, 1, tl_color, true CLIP_NUM_DEFAULT);
	simgraph32_draw_rect_clipped(x1, y1 + h - 1, w, 1, rd_color, true CLIP_NUM_DEFAULT);

	h -= 2;

	simgraph32_draw_vline_clipped(x1,         y1 + 1, h, tl_color, true CLIP_NUM_DEFAULT);
	simgraph32_draw_vline_clipped(x1 + w - 1, y1 + 1, h, rd_color, true CLIP_NUM_DEFAULT);
}

static void simgraph32_draw_right_triangle(scr_coord_val x, scr_coord_val y, scr_coord_val height, const PIXVAL colval, const bool dirty)
{
	y += (height / 2);
	while(  height > 0  ) {
		simgraph32_draw_vline( x, y-(height/2), height, colval, dirty );
		x++;
		height -= 2;
	}
}

static void simgraph32_draw_signal_direction(scr_coord_val x, scr_coord_val y, uint8 way_dir, uint8 sig_dir, PIXVAL col1, PIXVAL col1_dark, bool is_diagonal, uint8 slope)
{
	uint8 width        = is_diagonal ? g_simgraph32.current_tile_raster_width/6*0.353 : g_simgraph32.current_tile_raster_width/6;
	const uint8 height = is_diagonal ? g_simgraph32.current_tile_raster_width/6*0.353 : g_simgraph32.current_tile_raster_width/12;
	const uint8 thickness = max( g_simgraph32.current_tile_raster_width/36, 2);

	x += g_simgraph32.current_tile_raster_width/2;
	y += (g_simgraph32.current_tile_raster_width*9)/16;

	if (is_diagonal) {

		if (way_dir == ribi_t::northeast || way_dir == ribi_t::southwest) {
			// vertical
			x += (way_dir==ribi_t::northeast) ? g_simgraph32.current_tile_raster_width/4 : (-g_simgraph32.current_tile_raster_width/4);
			y += g_simgraph32.current_tile_raster_width/16;
			width = width<<2; // 4x

			// upper
			for (uint8 xoff = 0; xoff < width/2; xoff++) {
				const uint8 yoff = (uint8)((xoff+1)/2);
				// up
				if (sig_dir & ribi_t::east || sig_dir & ribi_t::south) {
					simgraph32_draw_vline_clipped(x + xoff, y+yoff, width/4 - yoff, col1, true CLIP_NUM_DEFAULT);
					simgraph32_draw_vline_clipped(x-xoff-1, y+yoff, width/4 - yoff, col1, true CLIP_NUM_DEFAULT);
				}
				// down
				if (sig_dir & ribi_t::west || sig_dir & ribi_t::north) {
					simgraph32_draw_vline_clipped(x+xoff,   y+g_simgraph32.current_tile_raster_width/6,              width/4-yoff, col1,      true CLIP_NUM_DEFAULT);
					simgraph32_draw_vline_clipped(x+xoff,   y+g_simgraph32.current_tile_raster_width/6+width/4-yoff, thickness,    col1_dark, true CLIP_NUM_DEFAULT);
					simgraph32_draw_vline_clipped(x-xoff-1, y+g_simgraph32.current_tile_raster_width/6,              width/4-yoff, col1,      true CLIP_NUM_DEFAULT);
					simgraph32_draw_vline_clipped(x-xoff-1, y+g_simgraph32.current_tile_raster_width/6+width/4-yoff, thickness,    col1_dark, true CLIP_NUM_DEFAULT);
				}
			}
			// up
			if (sig_dir & ribi_t::east || sig_dir & ribi_t::south) {
				simgraph32_draw_rect_clipped(x - width/2, y + width/4, width, thickness, col1_dark, true CLIP_NUM_DEFAULT);
			}
		}
		else {
			// horizontal
			y -= g_simgraph32.current_tile_raster_width/12;
			if (way_dir == ribi_t::southeast) {
				y += g_simgraph32.current_tile_raster_width/4;
			}

			for (uint8 xoff = 0; xoff < width*2; xoff++) {
				const uint8 h = width*2 - (scr_coord_val)(xoff + 1);
				// left
				if (sig_dir & ribi_t::north || sig_dir & ribi_t::east) {
					simgraph32_draw_vline_clipped(x - xoff - width*2, y + (scr_coord_val)((xoff+1)/2),   h,         col1,      true CLIP_NUM_DEFAULT);
					simgraph32_draw_vline_clipped(x - xoff - width*2, y + (scr_coord_val)((xoff+1)/2)+h, thickness, col1_dark, true CLIP_NUM_DEFAULT);
				}
				// right
				if (sig_dir & ribi_t::south || sig_dir & ribi_t::west) {
					simgraph32_draw_vline_clipped(x + xoff + width*2, y + (scr_coord_val)((xoff+1)/2),   h,         col1,      true CLIP_NUM_DEFAULT);
					simgraph32_draw_vline_clipped(x + xoff + width*2, y + (scr_coord_val)((xoff+1)/2)+h, thickness, col1_dark, true CLIP_NUM_DEFAULT);
				}
			}
		}
	}
	else {
		if (sig_dir & ribi_t::south) {
			// upper right
			scr_coord_val slope_offset_y = corner_se( slope )*TILE_HEIGHT_STEP;
			for (uint8 xoff = 0; xoff < width; xoff++) {
				simgraph32_draw_vline_clipped( x + xoff, y - slope_offset_y, (scr_coord_val)(xoff/2) + 1, col1, true CLIP_NUM_DEFAULT);
				simgraph32_draw_vline_clipped( x + xoff, y - slope_offset_y + (scr_coord_val)(xoff/2) + 1, thickness, col1_dark, true CLIP_NUM_DEFAULT);
			}
		}
		if (sig_dir & ribi_t::east) {
			scr_coord_val slope_offset_y = corner_se( slope )*TILE_HEIGHT_STEP;
			for (uint8 xoff = 0; xoff < width; xoff++) {
				simgraph32_draw_vline_clipped(x - xoff - 1, y - slope_offset_y, (scr_coord_val)(xoff/2) + 1, col1, true CLIP_NUM_DEFAULT);
				simgraph32_draw_vline_clipped(x - xoff - 1, y - slope_offset_y + (scr_coord_val)(xoff/2) + 1, thickness, col1_dark, true CLIP_NUM_DEFAULT);
			}
		}
		if (sig_dir & ribi_t::west) {
			scr_coord_val slope_offset_y = corner_nw( slope )*TILE_HEIGHT_STEP;
			for (uint8 xoff = 0; xoff < width; xoff++) {
				simgraph32_draw_vline_clipped(x + xoff, y - slope_offset_y + height*2 - (scr_coord_val)(xoff/2) + 1, (scr_coord_val)(xoff/2) + 1, col1, true CLIP_NUM_DEFAULT);
				simgraph32_draw_vline_clipped(x + xoff, y - slope_offset_y + height*2 + 1, thickness, col1_dark, true CLIP_NUM_DEFAULT);
			}
		}
		if (sig_dir & ribi_t::north) {
			scr_coord_val slope_offset_y = corner_nw( slope )*TILE_HEIGHT_STEP;
			for (uint8 xoff = 0; xoff < width; xoff++) {
				simgraph32_draw_vline_clipped(x - xoff - 1, y - slope_offset_y + height*2 - (scr_coord_val)(xoff/2) + 1, (scr_coord_val)(xoff/2) + 1, col1, true CLIP_NUM_DEFAULT);
				simgraph32_draw_vline_clipped(x - xoff - 1, y - slope_offset_y + height*2 + 1, thickness, col1_dark, true CLIP_NUM_DEFAULT);
			}
		}
	}
}

static void simgraph32_draw_bezier(scr_coord_val Ax, scr_coord_val Ay, scr_coord_val Bx, scr_coord_val By, scr_coord_val ADx, scr_coord_val ADy, scr_coord_val BDx, scr_coord_val BDy, const PIXVAL colore, scr_coord_val draw, scr_coord_val dontDraw)
{
	scr_coord_val Cx,Cy,Dx,Dy;
	Cx = Ax + ADx;
	Cy = Ay + ADy;
	Dx = Bx + BDx;
	Dy = By + BDy;

	/* float a,b,rx,ry,oldx,oldy;
	for (float t=0.0;t<=1;t+=0.05)
	{
		a = t;
		b = 1.0 - t;
		if (t>0.0)
		{
			oldx=rx;
			oldy=ry;
		}
		rx = Ax*b*b*b + 3*Cx*b*b*a + 3*Dx*b*a*a + Bx*a*a*a;
		ry = Ay*b*b*b + 3*Cy*b*b*a + 3*Dy*b*a*a + By*a*a*a;
		if (t>0.0)
			if (!draw && !dontDraw)
				simgraph32_draw_line(rx,ry,oldx,oldy,colore);
			else
				display_direct_line_dotted_rgb(rx,ry,oldx,oldy,draw,dontDraw,colore);
	  }
*/

	sint32 rx = Ax*32*32*32; // init with a=0, b=32
	sint32 ry = Ay*32*32*32; // init with a=0, b=32

	// fixed point: we cycle between 0 and 32, rather than 0 and 1
	for(  sint32 a=1;  a<=32;  a++  ) {
		const sint32 b = 32 - a;
		const sint32 oldx = rx;
		const sint32 oldy = ry;
		rx = Ax*b*b*b + 3*Cx*b*b*a + 3*Dx*b*a*a + Bx*a*a*a;
		ry = Ay*b*b*b + 3*Cy*b*b*a + 3*Dy*b*a*a + By*a*a*a;

		// fixed point: due to cycling between 0 and 32 (1<<5), we divide by 32^3 == 1<<15 because of cubic interpolation
		if(  !draw  &&  !dontDraw  ) {
			simgraph32_draw_line( rx>>15, ry>>15, oldx>>15, oldy>>15, colore );
		}
		else {
			simgraph32_draw_line_dotted( rx>>15, ry>>15, oldx>>15, oldy>>15, draw, dontDraw, colore );
		}
	}
}



/* ---------------- text and fonts ------------------------------------------
 *
 * The font engine is font_t, in display/font.h, and it is renderer
 * independent - it loads through FreeType and hands out 8 bit alpha glyph
 * bitmaps with their own advance, top and left. Nothing about it changes
 * because a screen pixel became four bytes wide, and nothing here widens it:
 * the glyph mask is the SOURCE, ARGB8888 is the DESTINATION, and the two are
 * separate contracts.
 *
 * What this renderer was missing is the state simgraph16 keeps beside the
 * engine: one loaded font and its derived metrics. default_font_ascent and
 * default_font_linespace are the globals simgraph.h declares and the GUI
 * reads, so they are defined here exactly as the legacy renderer defines them.
 */

static font_t default_font;
static int    default_font_numberwidth = 0;

/* default_font_ascent and default_font_linespace are NOT defined here: they
 * live in display/simgraph.cc, the renderer-neutral shim, which is why
 * simgraph16.cc only assigns them too. Defining them again would be a
 * duplicate symbol - and it was, until the linker said so. */

static bool simgraph32_load_font(const char *fname, bool reload)
{
	font_t loaded_fnt;

	if(  fname == NULL  ) {
		dbg->error( "display_load_font", "NULL filename" );
		return false;
	}

	// skip reloading if already in memory, if bdf font
	if(  !reload  &&  default_font.is_loaded()  &&  strcmp( default_font.get_fname(), fname ) == 0  ) {
		return true;
	}

	if(  loaded_fnt.load_from_file(fname)  ) {
		default_font = loaded_fnt;
		default_font_ascent    = default_font.get_ascent();
		default_font_linespace = default_font.get_linespace();

		// find default number width
		const char* digits = "0123456789";
		default_font_numberwidth = 0;
		while (*digits) {
			int pixel = default_font.get_glyph_advance(*digits++);
			if (pixel > default_font_numberwidth) {
				default_font_numberwidth = pixel;
			}
		}

		env_t::fontname = fname;

		return default_font.is_loaded();
	}

	return false;
}

static scr_coord_val simgraph32_get_char_width(utf32 c)
{
	return default_font.get_glyph_advance(c);
}

static scr_coord_val simgraph32_get_number_width()
{
	return default_font_numberwidth;
}

static bool simgraph32_font_has_character(utf16 char_code)
{
	return default_font.is_valid_glyph(char_code);
}

static utf32 simgraph32_get_next_char_with_metrics(const char* &text, unsigned char &byte_length, unsigned char &pixel_width)
{
	size_t len = 0;
	utf32 const char_code = utf8_decoder_t::decode((utf8 const *)text, len);

	if(  char_code==UNICODE_NUL  ||  char_code == '\n') {
		// case : end of text reached -> do not advance text pointer
		// also stop at linebreaks
		byte_length = 0;
		pixel_width = 0;
		return 0;
	}
	else {
		text += len;
		byte_length = (uint8)len;
		pixel_width = default_font.get_glyph_advance(char_code);
	}
	return char_code;
}

static utf32 simgraph32_get_prev_char_with_metrics(const char* &text, const char *const text_start, unsigned char &byte_length, unsigned char &pixel_width)
{
	if(  text<=text_start  ) {
		// case : start of text reached or passed -> do not move the pointer backwards
		byte_length = 0;
		pixel_width = 0;
		return 0;
	}

	utf32 char_code;
	// determine the start of the previous logical character
	do {
		--text;
	} while (  text>text_start  &&  (*text & 0xC0)==0x80  );

	size_t len = 0;
	char_code = utf8_decoder_t::decode((utf8 const *)text, len);
	byte_length = (uint8)len;
	pixel_width = default_font.get_glyph_advance(char_code);

	return char_code;
}

static scr_coord_val simgraph32_calc_text_width_n(const char *text, size_t len)
{
	uint8 byte_length = 0;
	uint8 pixel_width = 0;
	size_t idx = 0;
	scr_coord_val width = 0;

	while (simgraph32_get_next_char_with_metrics(text, byte_length, pixel_width)  &&  idx < len) {
		width += pixel_width;
		idx += byte_length;
	}
	return width;
}


static scr_size simgraph32_calc_multiline_text_size(const char *text)
{
	const font_t* const fnt = &default_font;
	int width = 0;
	bool last_cr = false;

	scr_size size{0,0};

	const utf8 *p = reinterpret_cast<const utf8 *>(text);
	while (const utf32 iUnicode = utf8_decoder_t::decode(p)) {

		if(  iUnicode == '\n'  ) {
			// new line: record max width
			size.w = max( size.w, width );
			size.h += LINESPACE;
			width = 0;
			last_cr = true;
			continue;
		}
		last_cr = false;
		width += fnt->get_glyph_advance(iUnicode);
	}

	size.w = max( size.w, width );
	if (!last_cr) {
		// extra CR of the last was not already a CR
		size.h += LINESPACE;
	}

	return size;
}

static size_t simgraph32_calc_text_index_for_width(const char *text, scr_coord_val max_width)
{
	size_t max_idx = 0;

	uint8 byte_length = 0;
	uint8 pixel_width = 0;
	scr_coord_val current_offset = 0;

	const char *tmp_text = text;
	while(  simgraph32_get_next_char_with_metrics(tmp_text, byte_length, pixel_width)  &&  max_width > (current_offset+pixel_width)  ) {
		current_offset += pixel_width;
		max_idx += byte_length;
	}
	return max_idx;
}

static scr_coord_val simgraph32_draw_text_clipped_n(scr_coord_val x, scr_coord_val y, const char* txt, control_alignment_t flags, const PIXVAL color, bool dirty, sint32 len  CLIP_NUM_DEF)
{
	scr_coord_val cL, cR, cT, cB;

	// TAKE CARE: Clipping area may be larger than actual screen size
	if(  (flags & DT_CLIP)  ) {
		cL = CR32.clip_rect.x;
		cR = CR32.clip_rect.xx;
		cT = CR32.clip_rect.y;
		cB = CR32.clip_rect.yy;
	}
	else {
		cL = 0;
		cR = disp_width;
		cT = 0;
		cB = disp_height;
	}

	if (len < 0) {
		// don't know len yet
		len = 0x7FFF;
	}

	// adapt x-coordinate for alignment
	switch (flags & ( ALIGN_LEFT | ALIGN_CENTER_H | ALIGN_RIGHT) ) {
		case ALIGN_LEFT:
			// nothing to do
			break;

		case ALIGN_CENTER_H:
			x -= simgraph32_calc_text_width_n(txt, len) / 2;
			break;

		case ALIGN_RIGHT:
			x -= simgraph32_calc_text_width_n(txt, len);
			break;
	}

	// still something to display?
	const font_t *const fnt = &default_font;

	if (x >= cR || y >= cB || y + fnt->get_linespace() <= cT) {
		// nothing to display
		return 0;
	}

	// store the initial x (for dirty marking)
	const scr_coord_val x0 = x;

	// big loop, draw char by char
	utf8_decoder_t decoder((utf8 const*)txt);
	size_t iTextPos = 0; // pointer on text position

	while (iTextPos < (size_t)len  &&  decoder.has_next()) {
		// decode char
		utf32 c = decoder.next();
		iTextPos = decoder.get_position() - (utf8 const*)txt;

		if(  c == '\n'  ) {
			// stop at linebreak
			break;
		}
		// print unknown character?
		else if(  !fnt->is_valid_glyph(c)  ) {
			c = 0;
		}

		// get the data from the font
		const font_t::glyph_t& glyph = fnt->get_glyph(c);
		const uint8 *p = glyph.bitmap;

		int screen_pos = (y + glyph.top) * disp_width + x + glyph.left;

		// glyph x clipping
		int g_left  = max(cL - x - glyph.left, 0);
		int g_right = min(cR - x - glyph.left, glyph.width);

		// all visible rows
		for (int h = 0; h < glyph.height; h++) {
			const int line = y + glyph.top + h;
			if(line >= cT && line < cB) {

				PIXVAL* dst = textur + screen_pos + g_left;

				// all columns
				for(int gx=g_left; gx<g_right; gx++) {
					int alpha = p[h*glyph.width + gx];

					if(alpha > 31) {
						// opaque
						*dst++ = color;
					} else {
						// partially transparent -> blend it
						PIXVAL old_color = *dst;
						*dst++ = blend_alpha32_argb(old_color, color, alpha);
					}
				}
			}
			screen_pos += disp_width;
		}

		x += fnt->get_glyph_advance(c);
	}

	if(  dirty  ) {
		// here, because only now we know the length also for ALIGN_LEFT text
		simgraph32_mark_rect_dirty_clip( x0, y, x - 1, y + LINESPACE - 1  CLIP_NUM_PAR);
	}

	// warning: actual len might be longer, due to clipping!
	return x - x0;
}

static scr_coord_val simgraph32_draw_multiline_text(scr_coord_val x, scr_coord_val y, const char *buf, PIXVAL color)
{
	scr_coord_val max_px_len = 0;
	if (buf != NULL && *buf != '\0') {
		const char *next;

		do {
			next = strchr(buf, '\n');
			const scr_coord_val px_len = simgraph32_draw_text_clipped_n(
				x, y, buf,
				ALIGN_LEFT | DT_CLIP, color, true,
				next != NULL ? (int)(size_t)(next - buf) : -1
				CLIP_NUM_DEFAULT
			);
			if(  px_len>max_px_len  ) {
				max_px_len = px_len;
			}
			y += LINESPACE;
		} while ((void)(buf = (next ? next+1 : NULL)), buf != NULL);
	}
	return max_px_len;
}

static void simgraph32_draw_text_outlined(scr_coord_val xpos, scr_coord_val ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty)
{
	const int flags = ALIGN_LEFT | DT_CLIP;
	simgraph32_draw_text_clipped_n(xpos - 1, ypos    , text, flags, shadow_color, dirty, -1  CLIP_NUM_DEFAULT);
	simgraph32_draw_text_clipped_n(xpos + 1, ypos + 2, text, flags, shadow_color, dirty, -1  CLIP_NUM_DEFAULT);
	simgraph32_draw_text_clipped_n(xpos,     ypos + 1, text, flags, text_color,   dirty, -1  CLIP_NUM_DEFAULT);
}

static void simgraph32_draw_text_shadowed(scr_coord_val xpos, scr_coord_val ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty)
{
	const int flags = ALIGN_LEFT | DT_CLIP;
	simgraph32_draw_text_clipped_n(xpos + 1, ypos + 1 + (12 - LINESPACE) / 2, text, flags, shadow_color, dirty, -1  CLIP_NUM_DEFAULT);
	simgraph32_draw_text_clipped_n(xpos,     ypos +     (12 - LINESPACE) / 2, text, flags, text_color,   dirty, -1  CLIP_NUM_DEFAULT);
}

static void simgraph32_draw_text_ellipsis_shadowed(scr_rect r, const char *text, int align, const PIXVAL color, const bool dirty, bool shadowed, PIXVAL shadow_color)
{
	const scr_coord_val ellipsis_width = translator::get_lang()->ellipsis_width;
	const scr_coord_val max_screen_width = r.w;
	size_t max_idx = 0;

	uint8 byte_length = 0;
	uint8 pixel_width = 0;
	scr_coord_val current_offset = 0;

	if(  align & ALIGN_CENTER_V  ) {
		r.y += (r.h - LINESPACE)/2;
		align &= ~ALIGN_CENTER_V;
	}

	const char *tmp_text = text;
	while(  simgraph32_get_next_char_with_metrics(tmp_text, byte_length, pixel_width)  &&  max_screen_width >= (current_offset+ellipsis_width+pixel_width)  ) {
		current_offset += pixel_width;
		max_idx += byte_length;
	}
	size_t max_idx_before_ellipsis = max_idx;
	scr_coord_val max_offset_before_ellipsis = current_offset;

	// now check if the text would fit completely
	if(  ellipsis_width  &&  pixel_width > 0  ) {
		// only when while above failed because of exceeding length
		current_offset += pixel_width;
		max_idx += byte_length;
		// check the rest ...
		while(  simgraph32_get_next_char_with_metrics(tmp_text, byte_length, pixel_width)  &&  max_screen_width >= (current_offset+pixel_width)  ) {
			current_offset += pixel_width;
			max_idx += byte_length;
		}
		// if it does not fit
		if(  max_screen_width < (current_offset+pixel_width)  ) {
			scr_coord_val w = 0;
			// since we know the length already, we try to center the text with the remaining pixels of the last character
			if(  align & ALIGN_CENTER_H  ) {
				w = (max_screen_width-max_offset_before_ellipsis-ellipsis_width)/2;
			}
			if (shadowed) {
				simgraph32_draw_text_clipped_n( r.x+w+1, r.y+1, text, ALIGN_LEFT | DT_CLIP, shadow_color, dirty, max_idx_before_ellipsis  CLIP_NUM_DEFAULT);
			}
			w += simgraph32_draw_text_clipped_n( r.x+w, r.y, text, ALIGN_LEFT | DT_CLIP, color, dirty, max_idx_before_ellipsis  CLIP_NUM_DEFAULT);

			if (shadowed) {
				simgraph32_draw_text_clipped_n( r.x+w+1, r.y+1, translator::translate("..."), ALIGN_LEFT | DT_CLIP, shadow_color, dirty, -1  CLIP_NUM_DEFAULT);
			}

			simgraph32_draw_text_clipped_n( r.x+w, r.y, translator::translate("..."), ALIGN_LEFT | DT_CLIP, color, dirty, -1  CLIP_NUM_DEFAULT);
			return;
		}
		else {
			// if this fits, end of string
			max_idx += byte_length;
			current_offset += pixel_width;
		}
	}
	switch (align & ALIGN_RIGHT) {
		case ALIGN_CENTER_H:
			r.x += (max_screen_width - current_offset)/2;
			break;
		case ALIGN_RIGHT:
			r.x += max_screen_width - current_offset;
		default: ;
	}
	if (shadowed) {
		simgraph32_draw_text_clipped_n( r.x+1, r.y+1, text, ALIGN_LEFT | DT_CLIP, shadow_color, dirty, -1  CLIP_NUM_DEFAULT);
	}
	simgraph32_draw_text_clipped_n( r.x, r.y, text, ALIGN_LEFT | DT_CLIP, color, dirty, -1  CLIP_NUM_DEFAULT);
}

static void simgraph32_draw_textbox3d_clipped(scr_coord_val xpos, scr_coord_val ypos, FLAGGED_PIXVAL ddd_color, FLAGGED_PIXVAL text_color, const char *text, int dirty  CLIP_NUM_DEF)
{
	const int vpadding = LINESPACE / 7;
	const int hpadding = LINESPACE / 4;

	scr_coord_val width = simgraph32_calc_text_width_n(text, 0x7FFFu);

	PIXVAL lighter = blend_colors_alpha32_argb(ddd_color, simgraph32_palette_lookup(COL_WHITE), 8 /* 25% */);
	PIXVAL darker  = blend_colors_alpha32_argb(ddd_color, simgraph32_palette_lookup(COL_BLACK), 8 /* 25% */);

	simgraph32_draw_rect_clipped( xpos+1, ypos - vpadding + 1, width+2*hpadding-2, LINESPACE+2*vpadding-1, ddd_color, dirty CLIP_NUM_PAR);

	simgraph32_draw_rect_clipped( xpos, ypos - vpadding,             width + 2*hpadding - 2, 1, lighter, dirty CLIP_NUM_DEFAULT);
	simgraph32_draw_rect_clipped( xpos, ypos + LINESPACE + vpadding, width + 2*hpadding - 2, 1, darker,  dirty CLIP_NUM_DEFAULT);

	simgraph32_draw_vline_clipped( xpos,                          ypos - vpadding, LINESPACE + vpadding * 2, lighter, dirty CLIP_NUM_DEFAULT);
	simgraph32_draw_vline_clipped( xpos + width + 2*hpadding - 2, ypos - vpadding, LINESPACE + vpadding * 2, darker,  dirty CLIP_NUM_DEFAULT);

	simgraph32_draw_text_clipped_n( xpos+hpadding, ypos+1, text, ALIGN_LEFT | DT_CLIP, text_color, dirty, -1 CLIP_NUM_DEFAULT);
}



/* ---------------- GUI renderer state ---------------------------------------
 *
 * None of this draws anything. It is the state the remaining GUI API expects a
 * renderer to own, and simgraph32 has never had any of it: a dirty-tile map, a
 * clip swap slot, the cursor selection and the scroll band.
 *
 * The dirty map is simgraph16's, transposed - same granularity, same bit
 * layout, same atomic OR in mark_dirty_word, which is the r12237 fix that is
 * in trunk. The state is this renderer's own: nothing is shared with
 * simgraph16, which keeps its own map in its own translation unit.
 */

static uint32 *tile_dirty     = NULL;
static uint32 *tile_dirty_old = NULL;
static int     tiles_per_line       = 0;
static int     tile_buffer_per_line = 0;
static int     tile_lines           = 0;
static int     tile_buffer_length   = 0;

/// cursor selection, exactly the two statics simgraph16 keeps
static int standard_pointer = -1;


/// Set one bit of the dirty map. Atomic when threads can draw concurrently -
/// this is the r12237 contract and it is transposed, not reinvented.
static inline void mark_dirty_word(uint32 *word, uint32 bits)
{
#ifdef MULTI_THREAD
#	if defined _MSC_VER
	static_assert( sizeof(long) == sizeof(uint32), "_InterlockedOr would not cover the whole word" );
	_InterlockedOr( (volatile long *)word, (long)bits );
#	else
	__atomic_fetch_or( word, bits, __ATOMIC_RELAXED );
#	endif
#else
	*word |= bits;
#endif
}


/// One tile, for the single-pixel writer.
static void mark_tile_dirty32(int x, int y)
{
	if(  tile_dirty != NULL  ) {
		const int bit = x + y * tile_buffer_per_line;
		mark_dirty_word( &tile_dirty[bit >> 5], 1 << (bit & 31) );
	}
}


/// Mark tiles dirty with NO clipping. The caller has already clamped.
static void mark_rect_dirty_nc(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2)
{
	if(  tile_dirty == NULL  ) {
		return;
	}
	// floor to tile size
	x1 >>= DIRTY_TILE_SHIFT;
	y1 >>= DIRTY_TILE_SHIFT;
	x2 >>= DIRTY_TILE_SHIFT;
	y2 >>= DIRTY_TILE_SHIFT;

	for(  ;  y1 <= y2;  y1++  ) {
		int bit = y1 * tile_buffer_per_line + x1;
		const int end = bit + x2 - x1;
		do {
			mark_dirty_word( &tile_dirty[bit >> 5], 1 << (bit & 31) );
		} while(  ++bit <= end  );
	}
}


/// (re)allocate the dirty map for the current display size
static void dirty_state_alloc()
{
	free( tile_dirty_old );
	free( tile_dirty );

	tiles_per_line       = (disp_width + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
	tile_buffer_per_line = (tiles_per_line + 31) & ~31;
	tile_lines           = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
	tile_buffer_length   = (tile_lines * tile_buffer_per_line / 32);

	tile_dirty     = MALLOCN( uint32, tile_buffer_length );
	tile_dirty_old = MALLOCN( uint32, tile_buffer_length );
	MEMZERON( tile_dirty_old, tile_buffer_length );
	memset( tile_dirty, 0xFFFFFFFF, sizeof(uint32) * tile_buffer_length );
}


static void dirty_state_free()
{
	free( tile_dirty_old );
	free( tile_dirty );
	tile_dirty = tile_dirty_old = NULL;
	tile_buffer_length = tiles_per_line = tile_lines = tile_buffer_per_line = 0;
}


/// how many tiles are currently marked - for gates, not for drawing
static int dirty_state_count()
{
	int n = 0;
	for(  int b = 0;  b < tile_buffer_length * 32;  b++  ) {
		if(  tile_dirty[b >> 5] & (1u << (b & 31))  ) {
			n++;
		}
	}
	return n;
}


static void dirty_state_clear()
{
	if(  tile_dirty  ) {
		MEMZERON( tile_dirty, tile_buffer_length );
	}
}


/// the resize epilogue forgets the previous frame, as simgraph16 does
static void dirty_old_clear()
{
	if(  tile_dirty_old  ) {
		MEMZERON( tile_dirty_old, tile_buffer_length );
	}
}

static void simgraph32_mark_screen_dirty()
{
	memset( tile_dirty, 0xFFFFFFFF, sizeof(uint32) * tile_buffer_length );
}

static void simgraph32_mark_rect_dirty_wc(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2)
{
	// inside display?
	if(  x2 >= 0  &&  y2 >= 0  &&  x1 < disp_width  &&  y1 < disp_height  ) {
		if(  x1 < 0  ) {
			x1 = 0;
		}
		if(  y1 < 0  ) {
			y1 = 0;
		}
		if(  x2 >= disp_width  ) {
			x2 = disp_width - 1;
		}
		if(  y2 >= disp_height  ) {
			y2 = disp_height - 1;
		}
		mark_rect_dirty_nc( x1, y1, x2, y2 );
	}
}

static void simgraph32_mark_rect_dirty_clip(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2  CLIP_NUM_DEF)
{
	// inside clip_rect?
	if(  x2 >= CR32.clip_rect.x  &&  y2 >= CR32.clip_rect.y  &&  x1 < CR32.clip_rect.xx  &&  y1 < CR32.clip_rect.yy  ) {
		if(  x1 < CR32.clip_rect.x  ) {
			x1 = CR32.clip_rect.x;
		}
		if(  y1 < CR32.clip_rect.y  ) {
			y1 = CR32.clip_rect.y;
		}
		if(  x2 >= CR32.clip_rect.xx  ) {
			x2 = CR32.clip_rect.xx-1;
		}
		if(  y2 >= CR32.clip_rect.yy  ) {
			y2 = CR32.clip_rect.yy-1;
		}
		mark_rect_dirty_nc( x1, y1, x2, y2 );
	}
}

static void simgraph32_mark_img_dirty(image_id image, scr_coord_val xp, scr_coord_val yp)
{
	if(  image < anz_images  ) {
		simgraph32_mark_rect_dirty_wc(
			xp + images[image].x,
			yp + images[image].y,
			xp + images[image].x + images[image].w - 1,
			yp + images[image].y + images[image].h - 1
		);
	}
}

static void simgraph32_push_clip_rect(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h  CLIP_NUM_DEF)
{
	assert(!CR32.swap_active);
	// save active clipping rectangle
	CR32.clip_rect_swap = CR32.clip_rect;
	// active rectangle provided by parameters
	simgraph32_set_clip_rect(x, y, w, h  CLIP_NUM_PAR, false);
	CR32.swap_active = true;
}

static void simgraph32_pop_clip_rect(CLIP_NUM_DEF0)
{
	if (CR32.swap_active) {
		// swap original clipping rectangle back
		CR32.clip_rect   = CR32.clip_rect_swap;
		CR32.swap_active = false;
	}
}

static void simgraph32_swap_clip_rect(CLIP_NUM_DEF0)
{
	if (CR32.swap_active) {
		// swap clipping rectangles
		clip_dimension save = CR32.clip_rect;
		CR32.clip_rect = CR32.clip_rect_swap;
		CR32.clip_rect_swap = save;
	}
}

static void simgraph32_move_scroll_band(scr_coord_val start_y, scr_coord_val x_offset, scr_coord_val h)
{
	start_y  = max(start_y,  0);
	x_offset = min(x_offset, disp_width);
	h        = min(h,        disp_height);

	const PIXVAL *const src = textur + start_y * disp_width + x_offset;
	PIXVAL *const dst = textur + start_y * disp_width;
	const size_t amount = sizeof(PIXVAL) * (h * disp_width - x_offset);

	memmove(dst, src, amount);
}

static void simgraph32_set_cursor_visible(bool show)
{
#ifdef USE_SOFTPOINTER
	softpointer = show;
#else
	show_pointer(show);
#endif
}

static void simgraph32_set_default_cursor(int cursor_id)
{
	standard_pointer = cursor_id;
}

static void simgraph32_set_show_load_cursor(bool show)
{
#ifdef USE_SOFTPOINTER
	softpointer = !show;
#else
	set_pointer(show);
#endif
}



/* ---------------- GUI drawing surface: the nine-patch --------------------- *
 *
 * stretch_map_t is image_id[3][3]. Corners keep their natural size, edges and
 * the centre are TILED, and the last partial tile is clipped rather than
 * scaled - that is the historical algorithm and it is not improved here.
 *
 * Everything this needs is already real: draw_color_img (U7C),
 * draw_rezoomed_img_blend (U7B, blend family B), the clip rectangle (U8C) and
 * the dirty marking the writers do themselves (U8C). No new decoder, no new
 * walker, no new clipping engine, no new dirty engine.
 */

// to pass the extra clipnum when not needed use this
#ifdef MULTI_THREAD
#define CLIPNUM_IGNORE , 0
#else
#define CLIPNUM_IGNORE
#endif

static void display_three_image_row( image_id i1, image_id i2, image_id i3, scr_rect row, FLAGGED_PIXVAL)
{
	if(  i1!=IMG_EMPTY  ) {
		scr_coord_val w = images[i1].w;
		simgraph32_draw_color_img( i1, row.x, row.y, 0, false, true  CLIP_NUM_DEFAULT);
		row.x += w;
		row.w -= w;
	}
	// right
	if(  i3!=IMG_EMPTY  ) {
		scr_coord_val w = images[i3].w;
		simgraph32_draw_color_img( i3, row.get_right()-w, row.y, 0, false, true  CLIP_NUM_DEFAULT);
		row.w -= w;
	}
	// middle
	if(  i2!=IMG_EMPTY  ) {
		scr_coord_val w = images[i2].w;
		// tile it wide
		while(  w <= row.w  ) {
			simgraph32_draw_color_img( i2, row.x, row.y, 0, false, true  CLIP_NUM_DEFAULT);
			row.x += w;
			row.w -= w;
		}
		// for the rest we have to clip the rectangle
		if(  row.w > 0  ) {
			clip_dimension const cl = simgraph32_get_clip_rect(CLIP_NUM_DEFAULT_VALUE);
			simgraph32_set_clip_rect( cl.x, cl.y, max(0,min(row.get_right(),cl.xx)-cl.x), cl.h CLIP_NUM_DEFAULT, false);
			simgraph32_draw_color_img( i2, row.x, row.y, 0, false, true  CLIP_NUM_DEFAULT);
			simgraph32_set_clip_rect(cl.x, cl.y, cl.w, cl.h CLIP_NUM_DEFAULT, false);
		}
	}
}

static scr_coord_val get_img_width(image_id img)
{
	return img != IMG_EMPTY ? images[ img ].w : 0;
}

static scr_coord_val get_img_height(image_id img)
{
	return img != IMG_EMPTY ? images[ img ].h : 0;
}

typedef void (*DISP_THREE_ROW_FUNC)(image_id, image_id, image_id, scr_rect, FLAGGED_PIXVAL);

static void display_img_stretch_intern( const stretch_map_t &imag, scr_rect area, DISP_THREE_ROW_FUNC display_three_image_rowf, FLAGGED_PIXVAL color)
{
	scr_coord_val h_top    = max(max( get_img_height(imag[0][0]), get_img_height(imag[1][0])), get_img_height(imag[2][0]));
	scr_coord_val h_middle = max(max( get_img_height(imag[0][1]), get_img_height(imag[1][1])), get_img_height(imag[2][1]));
	scr_coord_val h_bottom = max(max( get_img_height(imag[0][2]), get_img_height(imag[1][2])), get_img_height(imag[2][2]));

	// center vertically if images[*][1] are empty, display images[*][0]
	if(  imag[0][1] == IMG_EMPTY  &&  imag[1][1] == IMG_EMPTY  &&  imag[2][1] == IMG_EMPTY  ) {
		scr_coord_val h = max(h_top, get_img_height(imag[1][1]));
		// center vertically
		area.y += (area.h-h)/2;
	}

	// center horizontally if images[1][*] are empty, display images[0][*]
	if(  imag[1][0] == IMG_EMPTY  &&  imag[1][1] == IMG_EMPTY  &&  imag[1][2] == IMG_EMPTY  ) {
		scr_coord_val w_left = max(max( get_img_width(imag[0][0]), get_img_width(imag[0][1])), get_img_width(imag[0][2]));
		// center vertically
		area.x += (area.w-w_left)/2;
	}

	// top row
	display_three_image_rowf( imag[0][0], imag[1][0], imag[2][0], area, color);

	// bottom row
	if(  h_bottom > 0  ) {
		scr_rect row( area.x, area.y+area.h-h_bottom, area.w, h_bottom );
		display_three_image_rowf( imag[0][2], imag[1][2], imag[2][2], row, color);
	}

	// now stretch the middle
	if(  h_middle > 0  ) {
		scr_rect row( area.x, area.y+h_top, area.w, area.h-h_top-h_bottom);
		// tile it wide
		while(  h_middle <= row.h  ) {
			display_three_image_rowf( imag[0][1], imag[1][1], imag[2][1], row, color);
			row.y += h_middle;
			row.h -= h_middle;
		}
		// for the rest we have to clip the rectangle
		if(  row.h > 0  ) {
			clip_dimension const cl = simgraph32_get_clip_rect(CLIP_NUM_DEFAULT_VALUE);
			simgraph32_set_clip_rect( cl.x, cl.y, cl.w, max(0,min(row.get_bottom(),cl.yy)-cl.y) CLIP_NUM_DEFAULT, false);
			display_three_image_rowf( imag[0][1], imag[1][1], imag[2][1], row, color);
			simgraph32_set_clip_rect(cl.x, cl.y, cl.w, cl.h CLIP_NUM_DEFAULT, false);
		}
	}
}

static void simgraph32_draw_stretch_map(const stretch_map_t &imag, scr_rect area)
{
	display_img_stretch_intern(imag, area, display_three_image_row, 0);
}

static void display_three_blend_row( image_id i1, image_id i2, image_id i3, scr_rect row, FLAGGED_PIXVAL color )
{
	if(  i1!=IMG_EMPTY  ) {
		scr_coord_val w = images[i1].w;
		simgraph32_draw_rezoomed_img_blend( i1, row.x, row.y, 0, color, false, true CLIPNUM_IGNORE );
		row.x += w;
		row.w -= w;
	}
	// right
	if(  i3!=IMG_EMPTY  ) {
		scr_coord_val w = images[i3].w;
		simgraph32_draw_rezoomed_img_blend( i3, row.get_right()-w, row.y, 0, color, false, true CLIPNUM_IGNORE );
		row.w -= w;
	}
	// middle
	if(  i2!=IMG_EMPTY  ) {
		scr_coord_val w = images[i2].w;
		// tile it wide
		while(  w <= row.w  ) {
			simgraph32_draw_rezoomed_img_blend( i2, row.x, row.y, 0, color, false, true CLIPNUM_IGNORE );
			row.x += w;
			row.w -= w;
		}
		// for the rest we have to clip the rectangle
		if(  row.w > 0  ) {
			clip_dimension const cl = simgraph32_get_clip_rect(CLIP_NUM_DEFAULT_VALUE);
			simgraph32_set_clip_rect( cl.x, cl.y, max(0,min(row.get_right(),cl.xx)-cl.x), cl.h CLIP_NUM_DEFAULT, false);
			simgraph32_draw_rezoomed_img_blend( i2, row.x, row.y, 0, color, false, true CLIPNUM_IGNORE );
			simgraph32_set_clip_rect(cl.x, cl.y, cl.w, cl.h CLIP_NUM_DEFAULT, false);
		}
	}
}

static void simgraph32_draw_stretch_map_blend(const stretch_map_t &imag, scr_rect area, FLAGGED_PIXVAL color)
{
	display_img_stretch_intern(imag, area, display_three_blend_row, color);
}

static void simgraph32_set_image_procs(bool is_global)
{
	if(  is_global  ) {
		g_simgraph32.draw_normal = simgraph32_draw_img_aux;
		g_simgraph32.draw_color  = simgraph32_draw_color_img;
		g_simgraph32.draw_blend  = simgraph32_draw_rezoomed_img_blend;
		g_simgraph32.draw_alpha  = simgraph32_draw_rezoomed_img_alpha;
		g_simgraph32.current_tile_raster_width = g_simgraph32.tile_raster_width;
	}
	else {
		g_simgraph32.draw_normal = simgraph32_draw_base_img;
		g_simgraph32.draw_color  = simgraph32_draw_base_img;
		g_simgraph32.draw_blend  = simgraph32_draw_base_img_blend;
		g_simgraph32.draw_alpha  = simgraph32_draw_base_img_alpha;
		g_simgraph32.current_tile_raster_width = gfx->get_base_tile_raster_width();
	}
}


void simgraph32_set_light_color(int light_idx, rgb888_t day_colour, rgb888_t night_colour)
{
	display_day_lights[light_idx]   = day_colour;
	display_night_lights[light_idx] = night_colour;
}
