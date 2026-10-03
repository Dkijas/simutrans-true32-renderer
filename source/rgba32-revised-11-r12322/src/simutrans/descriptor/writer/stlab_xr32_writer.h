/*
 * STLAB-RGBA32-END-TO-END-MAKEOBJ-RUNTIME-01 -- FORMAT CANDIDATE v1 serializer for makeobj.
 *
 * NOT FOR COMMIT. Laboratory only. `XR32` is an EXPERIMENTAL tag, not a final ABI.
 *
 * The byte layout is taken from RGBA32-NODE-V1-CANDIDATE.md and is not restated or
 * "improved" here. This translation unit knows the layout; it knows nothing about how the
 * semantic records were obtained. That separation is deliberate: the extractor is
 * certified against the Python oracle on its own, and this file is certified against the
 * independent parser on its own.
 */
#ifndef DESCRIPTOR_WRITER_STLAB_XR32_WRITER_H
#define DESCRIPTOR_WRITER_STLAB_XR32_WRITER_H

#include <vector>
#include "obj_writer.h"
#include "stlab_semantic.h"

class obj_node_t;


/// a node waiting to be emitted after all the IMG children of its parent
struct stlab_pending_node_t {
	uint16 img_index;
	std::vector<uint8> body;
};


class stlab_xr32_writer_t : public obj_writer_t
{
private:
	static stlab_xr32_writer_t the_instance;
	stlab_xr32_writer_t() { register_writer(false); }

public:
	static stlab_xr32_writer_t *instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return (obj_type)C4ID('X', 'R', '3', '2'); }
	const char *get_type_name() const OVERRIDE { return "xr32"; }
};


/// should the base plane be deflated? (env STLAB_XR32_DEFLATE, default yes)
bool stlab_xr32_deflate_enabled();

/**
 * Build one candidate node body.
 *
 * @param rgba     the region, w*h*4 straight RGBA8888
 * @param records  the region's semantic records, positions local to the region and sorted
 */
void stlab_xr32_build_body(const uint8 *rgba, int w, int h, sint16 x, sint16 y,
                           uint8 zoomable, uint16 img_index,
                           const std::vector<stlab_semantic_record_t> &records,
                           std::vector<uint8> &body);

/// the nodes accumulated while the current parent's images were written
std::vector<stlab_pending_node_t> &stlab_pending_nodes();

/// emit every pending node as a child of @p parent, then clear the list
void stlab_xr32_flush(FILE *fp, obj_node_t &parent);

#endif
