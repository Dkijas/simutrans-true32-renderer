/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simgraph.h"

#include "../simmem.h"

#include "simgraph0.h"
#include "simgraph16.h"
#include "simgraph32.h"


const simgraph_t *gfx;


/*
 * High-resolution counterparts.
 *
 * Kept here rather than in a backend so every build links, and so the table
 * does not depend on which renderer is compiled in. It is written once, by
 * pakset_manager_t::resolve_multires() after all paks and add-ons have been
 * read, and only read afterwards.
 */
static image_id *hires_of  = NULL;
static uint32    hires_len = 0;


void simgraph_set_hires(image_id base, image_id hires)
{
	if(  base == IMG_EMPTY  ) {
		return;
	}
	if(  base >= hires_len  ) {
		const uint32 want = base + 1024u;
		hires_of = REALLOC( hires_of, image_id, want );
		for(  uint32 i = hires_len;  i < want;  i++  ) {
			hires_of[i] = IMG_EMPTY;
		}
		hires_len = want;
	}
	hires_of[base] = hires;
}


image_id simgraph_get_hires(image_id base)
{
	return (base < hires_len) ? hires_of[base] : IMG_EMPTY;
}


scr_coord_val default_font_ascent = 0;

// a font height of zero could cause division by zero errors, even though it should not be used in a server
scr_coord_val default_font_linespace = 1;


const simgraph_t *simgraph_select(simgraph_type_t preferred_type)
{
	switch (preferred_type) {
#if COLOUR_DEPTH == 0
		case SIMGRAPH_TYPE_NULL:     return &g_simgraph0;
#else
#	if COLOUR_DEPTH == 32
		// only the 32 bit renderer is compiled in, so it answers the generic
		// request for a software renderer
		case SIMGRAPH_TYPE_SOFTWARE:   return &g_simgraph32;
		case SIMGRAPH_TYPE_SOFTWARE32: return &g_simgraph32;
#	else
		case SIMGRAPH_TYPE_SOFTWARE: return &g_simgraph16;
#	endif
#endif

		default: return NULL;
	}
}
