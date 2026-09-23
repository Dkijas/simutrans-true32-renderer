/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DISPLAY_BLIT_CORE_H
#define DISPLAY_BLIT_CORE_H


/*
 * Renderer-independent image traversal.
 *
 * An image row is the historical run stream
 *
 *      [clear][count][pixels...][clear]...[0]
 *
 * with TRANSPARENT_RUN marking a semi-transparent colour run. Decoding that
 * stream, stepping over clear runs and walking rows is the same work whatever
 * a screen pixel is, so it lives here once.
 *
 * Everything that knows what a screen pixel IS stays with the renderer, in a
 * pixel policy:
 *
 *      Ops::screen_t                     the destination pixel type
 *      Ops::copy (dest, src, end)        opaque run
 *      Ops::alpha(dest, src, end)        semi-transparent run
 *
 * The policy is defined inside the renderer's own translation unit, so it can
 * reach that renderer's palette and blend functions without any of them being
 * exported, and so the compiler can inline straight through it.
 *
 * This header owns no colour arithmetic, no palette, no alpha formula and no
 * clipping state.
 */

#include "../simtypes.h"
#include "scr_coord.h"

#ifndef TRANSPARENT_RUN
#	define TRANSPARENT_RUN (0x8000u)
#endif


/**
 * Draw one image without clipping.
 *
 * @param h          rows to draw
 * @param tp         first destination pixel of the first row
 * @param sp         run stream, already in the policy's screen space
 * @param disp_width destination pitch, in pixels
 */
template<class Ops>
static inline void blit_core_nc(scr_coord_val h,
                                typename Ops::screen_t *tp,
                                const typename Ops::screen_t *sp,
                                const int disp_width)
{
	typedef typename Ops::screen_t screen_t;

	if(  h > 0  ) {
		do { // line decoder
			uint16 runlen = (uint16)*sp++;
			screen_t *p = tp;

			do {
				// we start with a clear run
				p += (runlen & ~TRANSPARENT_RUN);

				// now get colored pixels
				runlen = (uint16)*sp++;
				if(  runlen & TRANSPARENT_RUN  ) {
					runlen &= ~TRANSPARENT_RUN;
					Ops::alpha( p, sp, sp + runlen );
				}
				else {
					Ops::copy( p, sp, sp + runlen );
				}
				p  += runlen;
				sp += runlen;
			} while(  (runlen = (uint16)*sp++)  );

			tp += disp_width;
		} while(  --h  );
	}
}


/**
 * Interval of a row when the same interval applies to every row.
 *
 * This is the horizontally clipped case: the caller has already worked out
 * the clip rectangle, and it does not change while the image is drawn.
 */
struct span_fixed_t {
	int xmin, xmax;

	inline void next(int &lo, int &hi) const { lo = xmin; hi = xmax; }
};


/**
 * Draw one image against a clipping interval that may change per row.
 *
 * The interval comes from @p span, an object the RENDERER supplies:
 * span_fixed_t for the clip rectangle, or a renderer-local object that reads
 * that renderer's own clipping state for the polygon case. This header
 * therefore never sees a clipping_info_t, a CLIP_NUM or a clip rectangle, and
 * per-thread clipping stays a question each renderer answers for itself.
 *
 * @param h          rows to draw
 * @param tp         first pixel of the first row of the destination
 * @param sp         run stream, in the policy's source space
 * @param xp         x of the image, in destination pixels
 * @param disp_width destination pitch, in pixels
 * @param span       supplies [xmin, xmax) for each row, in order
 * @param ops        the pixel policy. Passed as an OBJECT, not used
 *                   purely statically, so that a policy may carry the
 *                   little state some of them need - a blend colour, an
 *                   alpha map. Stateless policies are empty classes and
 *                   the default argument costs nothing.
 */
template<class Ops, class Span>
static inline void blit_core_clipped(scr_coord_val h,
                                     typename Ops::screen_t *tp,
                                     const typename Ops::source_t *sp,
                                     const int xp,
                                     const int disp_width,
                                     Span span,
                                     const Ops &ops = Ops())
{
	if(  h > 0  ) {
		do { // line decoder
			int xpos = xp;

			int runlen = (int)(uint16)*sp++;

			// get left/right boundary, step
			int xmin, xmax;
			span.next( xmin, xmax );
			do {
				// we start with a clear run (which may be 0 pixels)
				xpos += (runlen & ~TRANSPARENT_RUN);

				// now get colored pixels
				runlen = (int)(uint16)*sp++;
				const int has_alpha = runlen & TRANSPARENT_RUN;
				runlen &= ~TRANSPARENT_RUN;

				// something to display?
				if(  xmin < xmax  &&  xpos + runlen > xmin  &&  xpos < xmax  ) {
					const int left = (xpos >= xmin ? 0 : xmin - xpos);
					const int len  = (xmax - xpos >= runlen ? runlen : xmax - xpos);
					if(  !has_alpha  ) {
						ops.copy( tp + xpos + left, sp + left, sp + len );
					}
					else {
						ops.alpha( tp + xpos + left, sp + left, sp + len );
					}
				}

				sp   += runlen;
				xpos += runlen;
			} while(  (runlen = (int)(uint16)*sp++)  );

			tp += disp_width;
		} while(  --h  );
	}
}


#endif
