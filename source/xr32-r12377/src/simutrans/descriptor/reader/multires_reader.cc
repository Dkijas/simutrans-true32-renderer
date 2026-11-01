/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../../simdebug.h"
#include "../../dataobj/pakset_manager.h"
#include "../image.h"
#include "../obj_desc.h"
#include "../obj_node_info.h"
#include "multires_reader.h"


/*
 * MRES v1 and v2. Little-endian, like every other pak field. v2 has exactly
 * this pairing layout; its children may end with XR32 companions of the IMG
 * children, which the XR32 seam pairs by its own img_index rule.
 *
 *   header
 *     0   uint16  version           == 1
 *     2   uint16  count             number of pairings
 *
 *   pairing, repeated `count` times
 *     0   uint8   scale_log2        1 = twice the size
 *     1   uint8   path_depth        child indices that follow, <= 8
 *     2   uint16  hires_child       index into THIS node's own children
 *     4   uint32  object_type       the owning node's four-character type
 *     8   uint32  guard_adler       adler32 over the base image's len*2 bytes
 *    12   sint16  guard_x
 *    14   sint16  guard_y
 *    16   sint16  guard_w
 *    18   sint16  guard_h
 *    20   uint32  guard_len         base image len, in PIXVALs
 *    24   uint16  name_len
 *    26   char[]  name              not NUL terminated
 *   ...   uint16[path_depth]        child indices from the named object down
 *
 * A pairing is 26 + name_len + 2*path_depth bytes.
 *
 * Everything optional here fails soft: a version this build does not know, a
 * truncated node, an impossible path or a scale it cannot draw all end with the
 * pairing dropped and the authored low-resolution image used, never with a
 * refusal to load the pakset.
 */

#define MRES_PAIR_FIXED_BYTES (26)


obj_desc_t *multires_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	node_body_t p(fp, node.size, get_type_name());
	if (!p) return NULL;

	// A bare descriptor. MRES keeps nothing of its own once its pairings have
	// been handed over, but it has to exist so the generic reader has somewhere
	// to hang its children - the high-resolution images, read after this
	// returns. obj_reader_t::read_node<T>() cannot be used: it insists on an
	// empty body, and this node carries the pairings.
	obj_desc_t *desc = new obj_desc_t();

	if (node.size < 4) {
		dbg->warning("multires_reader_t::read_node", "MRES node is only %u bytes; ignoring it", node.size);
		return desc;
	}

	const uint16 version = decode_uint16(p);
	const uint16 count   = decode_uint16(p);

	if (version < MRES_VERSION_MIN || version > MRES_VERSION_MAX) {
		dbg->warning("multires_reader_t::read_node", "MRES version %u is not one this build reads (%u..%u); ignoring %u high-resolution pairing(s)", version, MRES_VERSION_MIN, MRES_VERSION_MAX, count);
		return desc;
	}

	uint32 read = 4;
	for (uint16 i = 0; i < count; i++) {
		if (read + MRES_PAIR_FIXED_BYTES > node.size) {
			dbg->warning("multires_reader_t::read_node", "MRES ends after %u of %u pairing(s); ignoring the rest", i, count);
			break;
		}
		mres_pair_t pair;
		pair.scale_log2  = decode_uint8(p);
		pair.path_depth  = decode_uint8(p);
		const uint16 hires_child = decode_uint16(p);
		pair.object_type = static_cast<obj_type>(decode_uint32(p));
		pair.guard_adler = decode_uint32(p);
		pair.guard_x     = (sint16)decode_uint16(p);
		pair.guard_y     = (sint16)decode_uint16(p);
		pair.guard_w     = (sint16)decode_uint16(p);
		pair.guard_h     = (sint16)decode_uint16(p);
		pair.guard_len   = decode_uint32(p);
		const uint16 name_len = decode_uint16(p);
		read += MRES_PAIR_FIXED_BYTES;

		if (read + name_len + 2u * pair.path_depth > node.size) {
			dbg->warning("multires_reader_t::read_node", "MRES pairing %u runs past the end of the node; ignoring it and the rest", i);
			break;
		}
		if (name_len > 0) {
			pair.base_object.assign(p.read_bytes(name_len), name_len);
		}
		read += name_len;

		if (pair.path_depth == 0 || pair.path_depth > MRES_MAX_DEPTH) {
			dbg->warning("multires_reader_t::read_node", "MRES pairing for '%s' has an image path %u deep; ignoring it", pair.base_object.c_str(), pair.path_depth);
			for (uint8 d = 0; d < pair.path_depth && d < MRES_MAX_DEPTH; d++) {
				decode_uint16(p); // keep the cursor aligned for the next pairing
			}
			read += 2u * pair.path_depth;
			continue;
		}
		for (uint8 d = 0; d < pair.path_depth; d++) {
			pair.path[d] = decode_uint16(p);
		}
		read += 2u * pair.path_depth;

		if (pair.scale_log2 != 1) {
			// A larger factor is a legitimate future pairing rather than
			// damage, so it is passed over quietly.
			dbg->warning("multires_reader_t::read_node", "MRES pairing for '%s' is %ux art, which this build cannot use; ignoring it", pair.base_object.c_str(), 1u << pair.scale_log2);
			continue;
		}
		if (hires_child >= node.nchildren) {
			dbg->warning("multires_reader_t::read_node", "MRES pairing for '%s' wants image %u of %u; ignoring it", pair.base_object.c_str(), hires_child, node.nchildren);
			continue;
		}

		// The image id does not exist until the children have been read, so
		// carry the child index and convert it in register_obj, which runs
		// after them.
		pair.hires = (image_id)hires_child;
		pakset_manager_t::multires_add(pair);
	}
	return desc;
}


void multires_reader_t::register_obj(obj_desc_t *&data)
{
	pakset_manager_t::multires_bind_images(data);
}
