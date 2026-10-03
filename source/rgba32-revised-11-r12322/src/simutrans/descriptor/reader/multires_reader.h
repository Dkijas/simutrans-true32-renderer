/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_MULTIRES_READER_H
#define DESCRIPTOR_READER_MULTIRES_READER_H


#include <string>

#include "obj_reader.h"
#include "../objversion.h"
#include "../../display/simimg.h"


/// The MRES layouts this build understands. v2 has v1's pairing layout; it only
/// allows XR32 companions after the IMG children (MRES-V2-SPEC.md).
#define MRES_VERSION_MIN (1)
#define MRES_VERSION_MAX (2)
/// How deep a base image may sit below its object. Four is what a building
/// needs (tile, 2d list, list, image); eight leaves room without unbounded
/// recursion on a damaged file.
#define MRES_MAX_DEPTH (8)


/**
 * One high-resolution pairing, as read and before it is resolved.
 *
 * The base image is named the only way a pak can name one: the owning object's
 * NAME, then the path of child indices down to the image node. Image ids are
 * deliberately not part of this. They are handed out during loading, they move
 * when an unrelated object is added, and identical images are collapsed onto
 * one id (image_reader_t::read_node), so two different logical images can share
 * one - none of which a stored identity may depend on.
 *
 * The guard is what makes a name safe to use. Duplicate object names resolve to
 * the last one loaded, so an add-on may keep a name and replace the art; the
 * guard records what the base looked like when the pairing was made, and a
 * pairing whose base no longer matches is dropped instead of applied.
 */
struct mres_pair_t {
	std::string base_object;          ///< owning object's name
	obj_type    object_type = obj_root; ///< and its node type
	uint16      path[MRES_MAX_DEPTH]; ///< child indices from the object down
	uint8       path_depth = 0;
	uint8       scale_log2 = 1;       ///< 1 = twice the size; only 1 is read
	uint32      guard_adler = 0;      ///< adler32 over the base's len*2 bytes
	sint16      guard_x = 0, guard_y = 0, guard_w = 0, guard_h = 0;
	uint32      guard_len = 0;        ///< base len, in PIXVALs
	image_id    hires = IMG_EMPTY;    ///< payload: child index, then image id
	bool        bound = false;        ///< has `hires` become an image id yet?
};


/**
 * Reads the optional MRES node: high-resolution counterparts for images that
 * have one.
 *
 * The node is a child of ROOT, so a build that does not know the type skips it
 * whole - body and children alike - and keeps loading. Its children are
 * ordinary IMG nodes holding the high-resolution art, which means no second
 * image format exists and an older build never even reads them.
 */
class multires_reader_t : public obj_reader_t
{
	OBJ_READER_DEF(multires_reader_t, obj_multires, "multires");

protected:
	/// @copydoc obj_reader_t::register_obj
	void register_obj(obj_desc_t *&desc) OVERRIDE;

public:
	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;
};


#endif
