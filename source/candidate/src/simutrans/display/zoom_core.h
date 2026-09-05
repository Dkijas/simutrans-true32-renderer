/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DISPLAY_ZOOM_CORE_H
#define DISPLAY_ZOOM_CORE_H


/*
 * Renderer-independent image resampling.
 *
 * Zooming a Simutrans image is a STORED16 -> STORED16 operation: unpack the run
 * stream to four bytes per pixel, resample, re-encode. It never touches a
 * screen pixel, a palette or a blend, so it lives here once and both renderers
 * use it. The algorithm is the historical one, moved unchanged.
 *
 * The one thing that is NOT unchanged is how the flat intermediate buffer says
 * "nothing here". The legacy renderer writes the COLOUR value 0x73FE, which is
 * the ordinary RGB555 colour (28,31,30) - so an opaque image pixel of exactly
 * that colour is indistinguishable from transparency and silently disappears
 * when the image is zoomed. That is measured, not suspected.
 *
 * The marker is therefore a policy:
 *
 *   zoom_sample_inband_t   the historical representation, value == 0x73FE.
 *                          The 16 bit renderer keeps it, byte for byte, so its
 *                          output does not move.
 *   zoom_sample_tagged_t   value plus an explicit clear bit beside it. No
 *                          colour can be mistaken for state. The 32 bit
 *                          renderer uses this one.
 *
 * This header knows STORED_PIXVAL, which is storage. It does not know PIXVAL,
 * palettes, alpha or framebuffers.
 */

#include <cstring>
#include <cstdlib>

#include "../simtypes.h"
#include "../simcolor.h"     // STORED_PIXVAL
#include "../simmem.h"


/// the historical in-band marker: a colour value doubles as "no pixel"
struct zoom_sample_inband_t {
	typedef STORED_PIXVAL value_t;

	static inline value_t       clear()                  { return (value_t)0x73FE; }
	static inline bool          is_clear(value_t v)      { return v == (value_t)0x73FE; }
	static inline value_t       make(STORED_PIXVAL c)    { return c; }
	static inline STORED_PIXVAL colour(value_t v)        { return v; }
};


/// value and state side by side: no colour is ever read as "no pixel"
struct zoom_sample_tagged_t {
	typedef uint32 value_t;

	static const uint32 CLEAR_BIT = 0x10000u;

	static inline value_t       clear()                  { return CLEAR_BIT; }
	static inline bool          is_clear(value_t v)      { return (v & CLEAR_BIT) != 0; }
	static inline value_t       make(STORED_PIXVAL c)    { return (value_t)c; }
	static inline STORED_PIXVAL colour(value_t v)        { return (STORED_PIXVAL)(v & 0xFFFFu); }
};


/// scratch buffers, owned by the caller so per-thread arrangements stay there
struct zoom_scratch_t {
	uint8  *unpack;    ///< four bytes per pixel, then the re-encoded output
	void   *resample;  ///< one Sample::value_t per pixel
	size_t  size;      ///< bytes allocated for `unpack`
};


/// result of one resample
struct zoom_image_t {
	sint16         x, y, w, h;
	uint32         len;
	STORED_PIXVAL *data;
};


#define SumSubpixel(p) \
	if(*(p)<255  &&  valid<255) { \
	if(*(p)==1) { valid = 255; r = g = b = 0; } else { valid ++; } /* mark special colors */\
		r += (p)[1]; \
		g += (p)[2]; \
		b += (p)[3]; \
	}


/// recode 4 bytes into one STORED_PIXVAL
static inline STORED_PIXVAL compress_pixel(uint8* p)
{
	return (((p[0]==1 ? 0x8000 : 0 ) | p[1]) + (((uint16)p[2])<<5) + (((uint16)p[3])<<10));
}

/// recode 4 bytes, respecting transparency
template<class Sample>
static inline typename Sample::value_t compress_pixel_transparent(uint8 *p)
{
	return p[0]==255 ? Sample::clear() : Sample::make(compress_pixel(p));
}

// zoom-in pixel taking color of above/below and transparency of diagonal neighbor into account
template<class Sample>
static inline typename Sample::value_t zoomin_pixel(uint8 *p, uint8* pab, uint8 *prl, uint8* pdia)
{
	if (p[0] == 255) {
		if ( (pab[0] | prl[0] | pdia[0])==255) {
			return Sample::clear(); // pixel and one neighbor transparent -> return transparent
		}
		// pixel transparent but all three neighbors not -> interpolate
		uint8 valid=0;
		uint8 r=0, g=0, b=0;
		SumSubpixel(pab);
		SumSubpixel(prl);
		if(valid==0) {
			return Sample::clear();
		}
		else if(valid==255) {
			return Sample::make((STORED_PIXVAL)((0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10)));
		}
		else {
			return Sample::make((STORED_PIXVAL)((r/valid) + (((uint16)(g/valid))<<5) + (((uint16)(b/valid))<<10)));
		}
	}
	else {
		if ( (pab[0] & prl[0] & pdia[0])!=255) {
			// pixel and one neighbor not transparent
			return Sample::make(compress_pixel(p));
		}
		return Sample::clear();
	}
}


/**
 * Convert base image data to actual image size
 * Uses averages of all sampled points to get the "real" value
 * Blurs a bit
 */


/**
 * Convert base image data to actual image size.
 *
 * Uses averages of all sampled points to get the "real" value. Blurs a bit.
 * The caller owns @p scratch and the buffer returned in @p out.data.
 */
template<class Sample>
static void zoom_core_rezoom(const STORED_PIXVAL *base_data,
                             const sint16 base_x, const sint16 base_y,
                             const sint16 base_w, const sint16 base_h,
                             const sint32 zoom_num, const sint32 zoom_den,
                             zoom_scratch_t &scratch,
                             zoom_image_t &out)
{
	typedef typename Sample::value_t sample_t;
	// NOT taken once at entry: the scratch buffers may be reallocated below
	sample_t *resample = (sample_t *)scratch.resample;

// now we want to downsize the image
// just divide the sizes
out.x = (base_x * zoom_num) / zoom_den;
out.y = (base_y * zoom_num) / zoom_den;
out.w = (base_w * zoom_num) / zoom_den;
out.h = (base_h * zoom_num) / zoom_den;

if(  out.h > 0  &&  out.w > 0  ) {
	// just recalculate the image in the new size
	const STORED_PIXVAL *src = base_data;
	sample_t *dest = NULL;
	// embed the baseimage in an image with margin ~ remainder
	const sint16 x_rem = (base_x * zoom_num) % zoom_den;
	const sint16 y_rem = (base_y * zoom_num) % zoom_den;
	const sint16 xl_margin = max( x_rem, 0);
	const sint16 xr_margin = max(-x_rem, 0);
	const sint16 yl_margin = max( y_rem, 0);
	const sint16 yr_margin = max(-y_rem, 0);
	// baseimage top-left  corner is at (xl_margin, yl_margin)
	// ...       low-right corner is at (xr_margin, yr_margin)

	sint32 orgzoomwidth = ((base_w + zoom_den - 1 ) / zoom_den) * zoom_den;
	sint32 newzoomwidth = (orgzoomwidth*zoom_num)/zoom_den;
	sint32 orgzoomheight = ((base_h + zoom_den - 1 ) / zoom_den) * zoom_den;
	sint32 newzoomheight = (orgzoomheight * zoom_num) / zoom_den;

	// we will unpack, re-sample, pack it

	// thus the unpack buffer must at least fit the window => find out maximum size
	// Note: This value is certainly way bigger than the average size we'll get,
	// but it's the worst scenario possible, a succession of solid - transparent - solid - transparent
	// pattern.
	// This would encode EACH LINE as:
	// 0x0000 (0 transparent) 0x0001 PIXWORD 0x0001 (every 2 pixels, 3 words) 0x0000 (EOL)
	// The extra +1 is to make sure we cover divisions with module != 0
	// We end with an over sized buffer for the normal usage, but since it's re-used for all re-zooms,
	// it's not performance critical and we are safe from all possible inputs.

	size_t new_size = ( ( (newzoomwidth * 3) / 2 ) + 1 + 2) * newzoomheight * sizeof(STORED_PIXVAL);
	size_t unpack_size = (xl_margin + orgzoomwidth + xr_margin) * (yl_margin + orgzoomheight + yr_margin) * 4;
	if(  unpack_size > new_size  ) {
		new_size = unpack_size;
	}
	new_size = ((new_size * 128) + 127) / 128; // enlarge slightly to try and keep buffers on their own cacheline for multithreaded access. A portable aligned_alloc would be better.
	if(  scratch.size < new_size  ) {
		free( scratch.resample );
		free( scratch.unpack );
		scratch.size = new_size;
		scratch.unpack   = MALLOCN( uint8, new_size );
		scratch.resample = MALLOCN( uint8, new_size * (sizeof(sample_t) / sizeof(STORED_PIXVAL)) );
	}
	resample = (sample_t *)scratch.resample;
	memset( scratch.unpack, 255, new_size ); // fill with invalid data to mark transparent regions

	// index of top-left corner
	uint32 baseoff = 4 * (yl_margin * (xl_margin + orgzoomwidth + xr_margin) + xl_margin);
	sint32 basewidth = xl_margin + orgzoomwidth + xr_margin;

	// now: unpack the image
	for(  sint32 y = 0;  y < base_h;  ++y  ) {
		uint16 runlen;
		uint8 *p = scratch.unpack + baseoff + y * (basewidth * 4);

		// decode line
		runlen = *src++;
		do {
			// clear run
			p += (runlen & ~TRANSPARENT_RUN) * 4;
			// color pixel
			runlen = (*src++) & ~TRANSPARENT_RUN;
			while(  runlen--  ) {
				// get rgb components
				STORED_PIXVAL s = *src++;
				*p++ = (s>>15);
				*p++ = (s & 31);
				s >>= 5;
				*p++ = (s & 31);
				s >>= 5;
				*p++ = (s & 31);
			}
			runlen = *src++;
		} while(  runlen != 0  );
	}

	// now we have the image, we do a repack then
	dest = resample;
	switch(  zoom_den  ) {
		case 1: {
			assert(zoom_num==2);

			// first half row - just copy values, do not fiddle with neighbor colors
			uint8 *p1 = scratch.unpack + baseoff;
			for(  sint16 x = 0;  x < orgzoomwidth;  x++  ) {
				sample_t c1 = compress_pixel_transparent<Sample>( p1 + (x * 4) );
				// now set the pixel ...
				dest[x * 2] = c1;
				dest[x * 2 + 1] = c1;
			}
			// skip one line
			dest += newzoomwidth;

			for(  sint16 y = 0;  y < orgzoomheight - 1;  y++  ) {
				uint8 *p1 = scratch.unpack + baseoff + y * (basewidth * 4);
				// copy leftmost pixels
				dest[0] = compress_pixel_transparent<Sample>( p1 );
				dest[newzoomwidth] = compress_pixel_transparent<Sample>( p1 + basewidth * 4 );
				for(  sint16 x = 0;  x < orgzoomwidth - 1;  x++  ) {
					uint8 *px1 = p1 + (x * 4);
					// pixel at 2,2 in 2x2 superpixel
					dest[x * 2 + 1] = zoomin_pixel<Sample>( px1, px1 + 4, px1 + basewidth * 4, px1 + basewidth * 4 + 4 );

					// 2x2 superpixel is transparent but original pixel was not
					// preserve one pixel
					if(  Sample::is_clear(dest[x * 2 + 1])  &&  px1[0] != 255  &&  Sample::is_clear(dest[x * 2])  &&  Sample::is_clear(dest[x * 2 - newzoomwidth])  &&  Sample::is_clear(dest[x * 2 - newzoomwidth - 1])  ) {
						// preserve one pixel
						dest[x * 2 + 1] = Sample::make(compress_pixel( px1 ));
					}

					// pixel at 2,1 in next 2x2 superpixel
					dest[x * 2 + 2] = zoomin_pixel<Sample>( px1 + 4, px1, px1 + basewidth * 4 + 4, px1 + basewidth * 4 );

					// pixel at 1,2 in next row 2x2 superpixel
					dest[x * 2 + newzoomwidth + 1] = zoomin_pixel<Sample>( px1 + basewidth * 4, px1 + basewidth * 4 + 4, px1, px1 + 4 );

					// pixel at 1,1 in next row next 2x2 superpixel
					dest[x * 2 + newzoomwidth + 2] = zoomin_pixel<Sample>( px1 + basewidth * 4 + 4, px1 + basewidth * 4, px1 + 4, px1 );
				}
				// copy rightmost pixels
				dest[2 * orgzoomwidth - 1] = compress_pixel_transparent<Sample>( p1 + 4 * (orgzoomwidth - 1) );
				dest[2 * orgzoomwidth + newzoomwidth - 1] = compress_pixel_transparent<Sample>( p1 + 4 * (orgzoomwidth - 1) + basewidth * 4 );
				// skip two lines
				dest += 2 * newzoomwidth;
			}
			// last half row - just copy values, do not fiddle with neighbor colors
			p1 = scratch.unpack + baseoff + (orgzoomheight - 1) * (basewidth * 4);
			for(  sint16 x = 0;  x < orgzoomwidth;  x++  ) {
				sample_t c1 = compress_pixel_transparent<Sample>( p1 + (x * 4) );
				// now set the pixel ...
				dest[x * 2]   = c1;
				dest[x * 2 + 1] = c1;
			}
			break;
		}
		case 2:
			for(  sint16 y = 0;  y < newzoomheight;  y++  ) {
				uint8 *p1 = scratch.unpack + baseoff + ((y * zoom_den + 0 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p2 = scratch.unpack + baseoff + ((y * zoom_den + 1 - y_rem) / zoom_num) * (basewidth * 4);
				for(  sint16 x = 0;  x < newzoomwidth;  x++  ) {
					uint8 valid = 0;
					uint8 r = 0, g = 0, b = 0;
					sint16 xreal1 = ((x * zoom_den + 0 - x_rem) / zoom_num) * 4;
					sint16 xreal2 = ((x * zoom_den + 1 - x_rem) / zoom_num) * 4;
					SumSubpixel( p1 + xreal1 );
					SumSubpixel( p1 + xreal2 );
					SumSubpixel( p2 + xreal1 );
					SumSubpixel( p2 + xreal2 );
					if(  valid == 0  ) {
						*dest++ = Sample::clear();
					}
					else if(  valid == 255  ) {
						*dest++ = (0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10);
					}
					else {
						*dest++ = (r/valid) + (((uint16)(g/valid))<<5) + (((uint16)(b/valid))<<10);
					}
				}
			}
			break;
		case 3:
			for(  sint16 y = 0;  y < newzoomheight;  y++  ) {
				uint8 *p1 = scratch.unpack + baseoff + ((y * zoom_den + 0 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p2 = scratch.unpack + baseoff + ((y * zoom_den + 1 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p3 = scratch.unpack + baseoff + ((y * zoom_den + 2 - y_rem) / zoom_num) * (basewidth * 4);
				for(  sint16 x = 0;  x < newzoomwidth;  x++  ) {
					uint8 valid = 0;
					uint16 r = 0, g = 0, b = 0;
					sint16 xreal1 = ((x * zoom_den + 0 - x_rem) / zoom_num) * 4;
					sint16 xreal2 = ((x * zoom_den + 1 - x_rem) / zoom_num) * 4;
					sint16 xreal3 = ((x * zoom_den + 2 - x_rem) / zoom_num) * 4;
					SumSubpixel( p1 + xreal1 );
					SumSubpixel( p1 + xreal2 );
					SumSubpixel( p1 + xreal3 );
					SumSubpixel( p2 + xreal1 );
					SumSubpixel( p2 + xreal2 );
					SumSubpixel( p2 + xreal3 );
					SumSubpixel( p3 + xreal1 );
					SumSubpixel( p3 + xreal2 );
					SumSubpixel( p3 + xreal3 );
					if(  valid == 0  ) {
						*dest++ = Sample::clear();
					}
					else if(  valid == 255  ) {
						*dest++ = (0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10);
					}
					else {
						*dest++ = (r/valid) | (((uint16)(g/valid))<<5) | (((uint16)(b/valid))<<10);
					}
				}
			}
			break;
		case 4:
			for(  sint16 y = 0;  y < newzoomheight;  y++  ) {
				uint8 *p1 = scratch.unpack + baseoff + ((y * zoom_den + 0 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p2 = scratch.unpack + baseoff + ((y * zoom_den + 1 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p3 = scratch.unpack + baseoff + ((y * zoom_den + 2 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p4 = scratch.unpack + baseoff + ((y * zoom_den + 3 - y_rem) / zoom_num) * (basewidth * 4);
				for(  sint16 x = 0;  x < newzoomwidth;  x++  ) {
					uint8 valid = 0;
					uint16 r = 0, g = 0, b = 0;
					sint16 xreal1 = ((x * zoom_den + 0 - x_rem) / zoom_num) * 4;
					sint16 xreal2 = ((x * zoom_den + 1 - x_rem) / zoom_num) * 4;
					sint16 xreal3 = ((x * zoom_den + 2 - x_rem) / zoom_num) * 4;
					sint16 xreal4 = ((x * zoom_den + 3 - x_rem) / zoom_num) * 4;
					SumSubpixel( p1 + xreal1 );
					SumSubpixel( p1 + xreal2 );
					SumSubpixel( p1 + xreal3 );
					SumSubpixel( p1 + xreal4 );
					SumSubpixel( p2 + xreal1 );
					SumSubpixel( p2 + xreal2 );
					SumSubpixel( p2 + xreal3 );
					SumSubpixel( p2 + xreal4 );
					SumSubpixel( p3 + xreal1 );
					SumSubpixel( p3 + xreal2 );
					SumSubpixel( p3 + xreal3 );
					SumSubpixel( p3 + xreal4 );
					SumSubpixel( p4 + xreal1 );
					SumSubpixel( p4 + xreal2 );
					SumSubpixel( p4 + xreal3 );
					SumSubpixel( p4 + xreal4 );
					if(  valid == 0  ) {
						*dest++ = Sample::clear();
					}
					else if(  valid == 255  ) {
						*dest++ = (0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10);
					}
					else {
						*dest++ = (r/valid) | (((uint16)(g/valid))<<5) | (((uint16)(b/valid))<<10);
					}
				}
			}
			break;
		case 8:
			for(  sint16 y = 0;  y < newzoomheight;  y++  ) {
				uint8 *p1 = scratch.unpack + baseoff + ((y * zoom_den + 0 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p2 = scratch.unpack + baseoff + ((y * zoom_den + 1 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p3 = scratch.unpack + baseoff + ((y * zoom_den + 2 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p4 = scratch.unpack + baseoff + ((y * zoom_den + 3 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p5 = scratch.unpack + baseoff + ((y * zoom_den + 4 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p6 = scratch.unpack + baseoff + ((y * zoom_den + 5 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p7 = scratch.unpack + baseoff + ((y * zoom_den + 6 - y_rem) / zoom_num) * (basewidth * 4);
				uint8 *p8 = scratch.unpack + baseoff + ((y * zoom_den + 7 - y_rem) / zoom_num) * (basewidth * 4);
				for(  sint16 x = 0;  x < newzoomwidth;  x++  ) {
					uint8 valid = 0;
					uint16 r = 0, g = 0, b = 0;
					sint16 xreal1 = ((x * zoom_den + 0 - x_rem) / zoom_num) * 4;
					sint16 xreal2 = ((x * zoom_den + 1 - x_rem) / zoom_num) * 4;
					sint16 xreal3 = ((x * zoom_den + 2 - x_rem) / zoom_num) * 4;
					sint16 xreal4 = ((x * zoom_den + 3 - x_rem) / zoom_num) * 4;
					sint16 xreal5 = ((x * zoom_den + 4 - x_rem) / zoom_num) * 4;
					sint16 xreal6 = ((x * zoom_den + 5 - x_rem) / zoom_num) * 4;
					sint16 xreal7 = ((x * zoom_den + 6 - x_rem) / zoom_num) * 4;
					sint16 xreal8 = ((x * zoom_den + 7 - x_rem) / zoom_num) * 4;
					SumSubpixel( p1 + xreal1 );
					SumSubpixel( p1 + xreal2 );
					SumSubpixel( p1 + xreal3 );
					SumSubpixel( p1 + xreal4 );
					SumSubpixel( p1 + xreal5 );
					SumSubpixel( p1 + xreal6 );
					SumSubpixel( p1 + xreal7 );
					SumSubpixel( p1 + xreal8 );
					SumSubpixel( p2 + xreal1 );
					SumSubpixel( p2 + xreal2 );
					SumSubpixel( p2 + xreal3 );
					SumSubpixel( p2 + xreal4 );
					SumSubpixel( p2 + xreal5 );
					SumSubpixel( p2 + xreal6 );
					SumSubpixel( p2 + xreal7 );
					SumSubpixel( p2 + xreal8 );
					SumSubpixel( p3 + xreal1 );
					SumSubpixel( p3 + xreal2 );
					SumSubpixel( p3 + xreal3 );
					SumSubpixel( p3 + xreal4 );
					SumSubpixel( p3 + xreal5 );
					SumSubpixel( p3 + xreal6 );
					SumSubpixel( p3 + xreal7 );
					SumSubpixel( p3 + xreal8 );
					SumSubpixel( p4 + xreal1 );
					SumSubpixel( p4 + xreal2 );
					SumSubpixel( p4 + xreal3 );
					SumSubpixel( p4 + xreal4 );
					SumSubpixel( p4 + xreal5 );
					SumSubpixel( p4 + xreal6 );
					SumSubpixel( p4 + xreal7 );
					SumSubpixel( p4 + xreal8 );
					SumSubpixel( p5 + xreal1 );
					SumSubpixel( p5 + xreal2 );
					SumSubpixel( p5 + xreal3 );
					SumSubpixel( p5 + xreal4 );
					SumSubpixel( p5 + xreal5 );
					SumSubpixel( p5 + xreal6 );
					SumSubpixel( p5 + xreal7 );
					SumSubpixel( p5 + xreal8 );
					SumSubpixel( p6 + xreal1 );
					SumSubpixel( p6 + xreal2 );
					SumSubpixel( p6 + xreal3 );
					SumSubpixel( p6 + xreal4 );
					SumSubpixel( p6 + xreal5 );
					SumSubpixel( p6 + xreal6 );
					SumSubpixel( p6 + xreal7 );
					SumSubpixel( p6 + xreal8 );
					SumSubpixel( p7 + xreal1 );
					SumSubpixel( p7 + xreal2 );
					SumSubpixel( p7 + xreal3 );
					SumSubpixel( p7 + xreal4 );
					SumSubpixel( p7 + xreal5 );
					SumSubpixel( p7 + xreal6 );
					SumSubpixel( p7 + xreal7 );
					SumSubpixel( p7 + xreal8 );
					SumSubpixel( p8 + xreal1 );
					SumSubpixel( p8 + xreal2 );
					SumSubpixel( p8 + xreal3 );
					SumSubpixel( p8 + xreal4 );
					SumSubpixel( p8 + xreal5 );
					SumSubpixel( p8 + xreal6 );
					SumSubpixel( p8 + xreal7 );
					SumSubpixel( p8 + xreal8 );
					if(  valid == 0  ) {
						*dest++ = Sample::clear();
					}
					else if(  valid == 255  ) {
						*dest++ = (0x8000 | r) + (((uint16)g)<<5) + (((uint16)b)<<10);
					}
					else {
						*dest++ = (r/valid) | (((uint16)(g/valid))<<5) | (((uint16)(b/valid))<<10);
					}
				}
			}
			break;
		default: assert(0);
	}

	// now encode the image again
	STORED_PIXVAL *enc = reinterpret_cast<STORED_PIXVAL *>(scratch.unpack);
	for(  sint16 y = 0;  y < newzoomheight;  y++  ) {
		const sample_t *line = resample + (y * newzoomwidth);
		uint16 count;
		sint16 x = 0;
		uint16 clear_colored_run_pair_count = 0;

		do {
			// check length of transparent pixels
			for(  count = 0;  x < newzoomwidth  &&  Sample::is_clear(line[x]);  count++, x++  )
				{}
			// first runlength: transparent pixels
			*enc++ = count;
			uint16 has_alpha = 0;
			// copy for non-transparent
			count = 0;
			while(  x < newzoomwidth  &&  !Sample::is_clear(line[x])  ) {
				const STORED_PIXVAL pixval = Sample::colour(line[x++]);
				if(  pixval >= 0x8020  &&  !has_alpha  ) {
					if(  count  ) {
						*enc++ = count;
						enc += count;
						count = 0;
						*enc++ = TRANSPARENT_RUN;
					}
					has_alpha = TRANSPARENT_RUN;
				}
				else if(  pixval < 0x8020  &&  has_alpha  ) {
					if(  count  ) {
						*enc++ = count+TRANSPARENT_RUN;
						enc += count;
						count = 0;
						*enc++ = TRANSPARENT_RUN;
					}
					has_alpha = 0;
				}
				count++;
				enc[count] = pixval;
			}

			/*
			 * If it is not the first clear-colored-run pair and its colored run is empty
			 * --> it is superfluous and can be removed by rolling back the pointer
			 */
			if(  clear_colored_run_pair_count > 0  &&  count == 0  ) {
				enc--;
				// this only happens at the end of a line, so no need to increment clear_colored_run_pair_count
			}
			else {
				*enc++ = count+has_alpha; // number of colored pixels
				enc += count; // skip them
				clear_colored_run_pair_count++;
			}
		} while(  x < newzoomwidth  );
		*enc++ = 0; // mark line end
	}

	// something left?
	out.w = newzoomwidth;
	out.h = newzoomheight;
	if(  newzoomheight > 0  ) {
		const size_t zoom_len = (size_t)(((uint8 *)enc) - ((uint8 *)scratch.unpack));
		out.len = (uint32)(zoom_len / sizeof(STORED_PIXVAL));
		out.data = MALLOCN(STORED_PIXVAL, out.len);
		assert( out.data );
		memcpy( out.data, scratch.unpack, zoom_len );
	}
}
else {
//			if (out.w <= 0) {
//				// h=0 will be ignored, with w=0 there was an error!
//				printf("WARNING: image%d w=0!\n", n);
//			}
	out.h = 0;
}
}


/**
 * Length in STORED_PIXVAL words of an un-zoomed run stream.
 */
static inline uint32 zoom_core_stored_len(const STORED_PIXVAL *sp, sint16 h)
{
	const STORED_PIXVAL *const start = sp;
	while(  h-- > 0  ) {
		do {
			// clear run + colored run + next clear run
			sp++;
			sp += (*sp)&(~TRANSPARENT_RUN); // MSVC crashes on (*sp)&(~TRANSPARENT_RUN) + 1 !!!
			sp ++;
		} while(  *sp  );
		sp++;
	}
	return (uint32)(size_t)(sp - start);
}


#endif
