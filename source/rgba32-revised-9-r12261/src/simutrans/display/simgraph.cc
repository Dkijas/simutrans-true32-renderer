/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simgraph.h"

#include "simgraph0.h"
#include "simgraph16.h"
#include "simgraph32.h"


const simgraph_t *gfx;

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
