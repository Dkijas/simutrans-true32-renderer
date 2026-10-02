/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DISPLAY_BLIT_CLIP_CORE_H
#define DISPLAY_BLIT_CLIP_CORE_H


/*
 * Renderer-independent clipping geometry.
 *
 * Working out which horizontal span of a row survives a set of clipping lines
 * is integer geometry: it does not know what a screen pixel is, how wide one
 * is, or what is drawn into it. So it lives here once, and both renderers use
 * it.
 *
 * What this header deliberately does NOT do:
 *
 *   - own any clipping state. simgraph16 keeps a clipping_info_t per thread
 *     and selects it with CLIP_NUM; simgraph32 keeps a single context. Both
 *     keep theirs. The helpers below take the state they need as arguments,
 *     so neither renderer's arrangement constrains the other, and per-thread
 *     clipping stays a question each renderer answers for itself.
 *   - know about PIXVAL, STORED_PIXVAL, palettes, alpha or framebuffers. It
 *     includes simtypes.h for the integer types and nothing else.
 */

#include <cstdlib>   // abs

#include "../simtypes.h"
#include "scr_coord.h"  // scr_coord_val


/**
 * Interval of the current row associated to some clipline.
 */
struct xrange {
	sint64 sx, sy;
	scr_coord_val y;
	bool non_convex_active;
};


class clip_line_t {
private:
	// line from (x0,y0) to (x1 y1)
	// clip (do not draw) everything right from the ray (x0,y0)->(x1,y1)
	// pixels on the ray are not drawn
	// (not_convex) if y0>=y1 then clip along the path (x0,-inf)->(x0,y0)->(x1,y1)
	// (not_convex) if y0<y1  then clip along the path (x0,y0)->(x1,y1)->(x1,+inf)
	int x0, y0;
	int dx, dy;
	sint64 sdy, sdx;
	sint64 inc;
	bool non_convex;

public:
	void clip_from_to(int x0_, int y0_, int x1, int y1, bool non_convex_) {
		x0 = x0_;
		dx = x1 - x0;
		y0 = y0_;
		dy = y1 - y0;
		non_convex = non_convex_;
		int steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
		if(  steps == 0  ) {
			return;
		}
		sdx = ((sint64)dx << 16) / steps;
		sdy = ((sint64)dy << 16) / steps;
		// to stay right from the line
		// left border: xmin <= x
		// right border: x < xmax
		if(  dy > 0  ) {
			if(  dy > dx  ) {
				inc = 1 << 16;
			}
			else {
				inc = ((sint64)dx << 16) / dy -  (1 << 16);
			}
		}
		else if(  dy < 0  ) {
			if(  dy < dx  ) {
				inc = 0; // (+1)-1 << 16;
			}
			else {
				inc = 0;
			}
		}
	}

	// clip if
	// ( x-x0) . (  y1-y0 )
	// ( y-y0) . (-(x1-x0)) < 0
	// -- initialize the clipping
	//    has to be called before image will be drawn
	//    return interval for x coordinate
	inline void get_x_range(scr_coord_val y, xrange &r, bool use_non_convex) const {
		// do everything for the previous row
		y--;
		r.y = y;
		r.non_convex_active = false;
		if(  non_convex  &&  use_non_convex  &&  y < y0  &&  y < (y0 + dy)  ) {
			r.non_convex_active = true;
			y = min(y0, y0+dy) - 1;
		}
		if(  dy != 0  ) {
			// init Bresenham algorithm
			const sint64 t = (((sint64)y - y0) << 16) / sdy;
			// sx >> 16 = x
			// sy >> 16 = y
			r.sx = t * sdx + inc + ((sint64)x0 << 16);
			r.sy = t * sdy + ((sint64)y0 << 16);
		}
	}

	// -- step one line down, return interval for x coordinate
	inline void inc_y(xrange &r, int &xmin, int &xmax) const {
		r.y++;
		// switch between clip vertical and along ray
		if(  r.non_convex_active  ) {
			if(  r.y == min( y0, y0 + dy )  ) {
				r.non_convex_active = false;
			}
			else {
				if(  dy < 0  ) {
					const int r_xmax = x0 + dx;
					if(  xmax > r_xmax  ) {
						xmax = r_xmax;
					}
				}
				else {
					const int r_xmin = x0 + 1;
					if(  xmin < r_xmin  ) {
						xmin = r_xmin;
					}
				}
				return;
			}
		}
		// go along the ray, Bresenham
		if(  dy != 0  ) {
			if(  dy > 0  ) {
				do {
					r.sx += sdx;
					r.sy += sdy;
				} while(  (r.sy >> 16) < r.y  );
				const int r_xmin = r.sx >> 16;
				if(  xmin < r_xmin  ) {
					xmin = r_xmin;
				}
			}
			else {
				do {
					r.sx -= sdx;
					r.sy -= sdy;
				} while(  (r.sy >> 16) < r.y  );
				const int r_xmax = r.sx >> 16;
				if(  xmax > r_xmax  ) {
					xmax = r_xmax;
				}
			}
		}
		// horizontal clip
		else {
			const bool clip = dx * (r.y - y0) > 0;
			if(  clip  ) {
				// invisible row
				xmin = +1;
				xmax = -1;
			}
		}
	}
};


/*
 * The two span helpers. The caller passes its own clip lines, ribi flags and
 * xranges; this code reads and steps them but owns none of them.
 */

/**
 * Initialize the clipping region for an image starting at screen line @p y.
 */
static inline void clip_core_init_ranges(int y, int number_of_clips, uint8 active_ribi,
                                         const uint8 *clip_ribi,
                                         const clip_line_t *poly_clips,
                                         xrange *xranges)
{
	for(  int i = 0;  i < number_of_clips;  i++  ) {
		if(  (clip_ribi[i] & active_ribi)  ) {
			poly_clips[i].get_x_range( y, xranges[i], active_ribi & 16 );
		}
	}
}


/**
 * Narrow [@p xmin, @p xmax) by every active clipping line and step them one
 * row down. The caller has already seeded the interval from its clip
 * rectangle.
 */
static inline void clip_core_step_y(int &xmin, int &xmax, int number_of_clips, uint8 active_ribi,
                                    const uint8 *clip_ribi,
                                    const clip_line_t *poly_clips,
                                    xrange *xranges)
{
	for(  int i = 0;  i < number_of_clips;  i++  ) {
		if(  (clip_ribi[i] & active_ribi)  ) {
			poly_clips[i].inc_y( xranges[i], xmin, xmax );
		}
	}
}


#endif
