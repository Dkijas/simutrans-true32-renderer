/*
 * STLAB-RGBA32-END-TO-END-MAKEOBJ-RUNTIME-01 -- FORMAT CANDIDATE v1 serializer.
 * NOT FOR COMMIT. Laboratory only.
 *
 * Layout, from RGBA32-NODE-V1-CANDIDATE.md, little-endian throughout:
 *
 *   0..3   magic "XR32"
 *   4      version                       = 1
 *   5      flags                         bit0 = base plane deflated
 *   6      class_registry                = 1
 *   7      reserved                      = 0
 *   8..9   width          uint16
 *   10..11 height         uint16
 *   12..13 x              sint16
 *   14..15 y              sint16
 *   16     zoomable       uint8
 *   17     reserved                      = 0
 *   18..19 img_index      uint16
 *   20..23 base_bytes     uint32         the plane as stored, compressed or not
 *   24..27 run_count      uint32
 *   28..31 run_bytes      uint32
 *
 * then the base plane, then the run block. One run is
 *
 *   varint(start - previous_end), varint(length), class, parameter, coverage
 *   [, under R, G, B  when coverage != 255 and the UNDER_IS_FRAMEBUFFER bit is clear]
 */
#include "stlab_xr32_writer.h"

#include <cstdlib>
#include <cstring>
#include <zlib.h>

#include "obj_node.h"
#include "../../simdebug.h"

stlab_xr32_writer_t stlab_xr32_writer_t::the_instance;
stlab_xnex_writer_t stlab_xnex_writer_t::the_instance;

static const uint8 XR32_VERSION        = 1;
static const uint8 XR32_CLASS_REGISTRY = 1;
static const uint8 XR32_HEADER_LEN     = 32;
static const uint8 XR32_FLAG_BASE_DEFLATED = 0x01;
static const uint8 XR32_FLAG_CRC32 = 0x02;
static const uint8 XR32_UNDER_IS_FRAMEBUFFER = 0x80;


bool stlab_xr32_deflate_enabled()
{
	static int cached = -1;
	if(  cached < 0  ) {
		const char *e = getenv("STLAB_XR32_DEFLATE");
		cached = (e != NULL  &&  e[0] == '0') ? 0 : 1;   // default: compress
	}
	return cached == 1;
}


std::vector<stlab_pending_node_t> &stlab_pending_nodes()
{
	static std::vector<stlab_pending_node_t> pending;
	return pending;
}


static void put16(std::vector<uint8> &b, size_t at, uint16 v)
{
	b[at] = (uint8)(v & 0xFF);
	b[at + 1] = (uint8)(v >> 8);
}


static void put32(std::vector<uint8> &b, size_t at, uint32 v)
{
	b[at] = (uint8)(v & 0xFF);
	b[at + 1] = (uint8)((v >> 8) & 0xFF);
	b[at + 2] = (uint8)((v >> 16) & 0xFF);
	b[at + 3] = (uint8)((v >> 24) & 0xFF);
}


static void put_varint(std::vector<uint8> &b, uint32 v)
{
	for(  ;;  ) {
		uint8 c = (uint8)(v & 0x7F);
		v >>= 7;
		b.push_back(v ? (uint8)(c | 0x80) : c);
		if(  !v  ) return;
	}
}


void stlab_xr32_build_body(const uint8 *rgba, int w, int h, sint16 x, sint16 y,
                           uint8 zoomable, uint16 img_index,
                           const std::vector<stlab_semantic_record_t> &records,
                           std::vector<uint8> &body)
{
	const size_t plane_len = (size_t)w * h * 4;

	std::vector<uint8> plane;
	uint8 flags = 0;
	if(  stlab_xr32_deflate_enabled()  ) {
		uLongf cap = compressBound((uLong)plane_len);
		plane.resize(cap);
		if(  compress2(&plane[0], &cap, rgba, (uLong)plane_len, 9) != Z_OK  ) {
			plane.assign(rgba, rgba + plane_len);        // fall back to the raw plane
		}
		else {
			plane.resize(cap);
			flags |= XR32_FLAG_BASE_DEFLATED;
		}
	}
	else {
		plane.assign(rgba, rgba + plane_len);
	}

	// run/delta encoding: fold neighbouring records that carry identical semantics
	std::vector<uint8> runs;
	uint32 run_count = 0, prev_end = 0;
	size_t i = 0;
	while(  i < records.size()  ) {
		const stlab_semantic_record_t &r = records[i];
		size_t j = i + 1;
		while(  j < records.size()
		        &&  records[j].position == records[j - 1].position + 1
		        &&  records[j].semantic_class == r.semantic_class
		        &&  records[j].semantic_parameter == r.semantic_parameter
		        &&  records[j].coverage_alpha8 == r.coverage_alpha8
		        &&  records[j].has_under == r.has_under
		        &&  records[j].under_r == r.under_r
		        &&  records[j].under_g == r.under_g
		        &&  records[j].under_b == r.under_b  ) {
			j++;
		}
		const uint32 length = (uint32)(j - i);
		put_varint(runs, r.position - prev_end);
		put_varint(runs, length);
		runs.push_back(r.semantic_class);
		runs.push_back(r.semantic_parameter);
		runs.push_back(r.coverage_alpha8);
		if(  r.coverage_alpha8 != 255
		     &&  (r.semantic_class & XR32_UNDER_IS_FRAMEBUFFER) == 0  ) {
			runs.push_back(r.under_r);
			runs.push_back(r.under_g);
			runs.push_back(r.under_b);
		}
		prev_end = r.position + length;
		run_count++;
		i = j;
	}

	body.assign(XR32_HEADER_LEN, 0);
	body[0] = 'X'; body[1] = 'R'; body[2] = '3'; body[3] = '2';
	body[4] = XR32_VERSION;
	flags |= XR32_FLAG_CRC32;
	body[5] = flags;
	body[6] = XR32_CLASS_REGISTRY;
	body[7] = 0;
	put16(body, 8, (uint16)w);
	put16(body, 10, (uint16)h);
	put16(body, 12, (uint16)x);
	put16(body, 14, (uint16)y);
	body[16] = zoomable;
	body[17] = 0;
	put16(body, 18, img_index);
	put32(body, 20, (uint32)plane.size());
	put32(body, 24, run_count);
	put32(body, 28, (uint32)runs.size());

	body.insert(body.end(), plane.begin(), plane.end());
	body.insert(body.end(), runs.begin(), runs.end());

	// integrity trailer: CRC-32 of every byte above, so a reader can refuse a damaged
	// plane or run block before it allocates or inflates anything
	const uint32 crc = (uint32)crc32( crc32(0L, Z_NULL, 0), &body[0], (uInt)body.size() );
	const size_t at = body.size();
	body.resize(at + 4);
	put32(body, at, crc);

	stlab_semantic_counters.rgba_raw_bytes += plane_len;
	stlab_semantic_counters.rgba_compressed_bytes += plane.size();
	stlab_semantic_counters.semantic_bytes += runs.size();
}


void stlab_xr32_flush(FILE *fp, obj_node_t &parent)
{
	std::vector<stlab_pending_node_t> &pending = stlab_pending_nodes();
	for(  size_t i = 0;  i < pending.size();  i++  ) {
		// creating the node bumps parent->nchildren, which is the only bookkeeping needed;
		// the parent's own body still reports the IMG count, so get_image() is untouched
		obj_writer_t *w = pending[i].night ? (obj_writer_t *)stlab_xnex_writer_t::instance()
		                                   : (obj_writer_t *)stlab_xr32_writer_t::instance();
		obj_node_t node(w, (uint32)pending[i].body.size(), &parent);
		node.write_bytes(fp, (uint32)pending[i].body.size(), &pending[i].body[0]);
		node.check_and_write_header(fp);
		stlab_semantic_counters.xr32_nodes_written++;
	}
	pending.clear();
}


/* ==========================================================================
 * XR32-SEMANTIC-MASK-PIPELINE-01 -- FORMAT v2 (class registry 2). NOT FOR COMMIT.
 *
 * The header is the v1 header with version = 2 and class_registry = 2. The plane is the
 * UNDER plane: the colour of the non-semantic surface of every pixel, alpha = the pixel's.
 * The run block holds contributor runs:
 *
 *   varint(start - previous start), varint(length), id, coverage
 *
 * id is the semantic vocabulary of registry 2 (0..7 PC1 tones, 8..15 PC2 tones, 16..30
 * lights, == the makeobj special index); coverage is 1..255. Starts never decrease and runs
 * MAY overlap: a pixel inside k runs has k contributors (k <= 4, coverages summing to at most
 * 255, no id twice). A v1 reader sees version 2 and falls back to the legacy IMG.
 * ==========================================================================*/
#include <algorithm>

static const uint8 XR32_V2_VERSION = 2;
static const uint8 XR32_V2_CLASS_REGISTRY = 2;

namespace {
struct v2_run_t {
	uint32 start, length;
	uint8 id, coverage;
	bool operator<(const v2_run_t &o) const { return start != o.start ? start < o.start : id < o.id; }
};
}


void stlab_xr32_build_body_v2(const uint8 *under, int w, int h, sint16 x, sint16 y,
                              uint8 zoomable, uint16 img_index,
                              const std::vector<stlab_sem_contrib_t> &contribs,
                              std::vector<uint8> &body)
{
	const size_t plane_len = (size_t)w * h * 4;

	std::vector<uint8> plane;
	uint8 flags = 0;
	{
		uLongf cap = compressBound((uLong)plane_len);
		plane.resize(cap);
		if(  compress2(&plane[0], &cap, under, (uLong)plane_len, 9) != Z_OK  ) {
			plane.assign(under, under + plane_len);
		}
		else {
			plane.resize(cap);
			flags |= XR32_FLAG_BASE_DEFLATED;
		}
	}

	// fold each id's consecutive pixels with equal coverage into runs
	std::vector<v2_run_t> runs_v;
	for(  int id = 0;  id < 256;  id++  ) {
		v2_run_t cur;
		bool open = false;
		for(  size_t i = 0;  i < contribs.size();  i++  ) {
			const stlab_sem_contrib_t &c = contribs[i];
			if(  c.id != id  ) continue;
			if(  open  &&  c.position == cur.start + cur.length  &&  c.coverage == cur.coverage  ) {
				cur.length++;
				continue;
			}
			if(  open  ) runs_v.push_back(cur);
			cur.start = c.position; cur.length = 1; cur.id = c.id; cur.coverage = c.coverage;
			open = true;
		}
		if(  open  ) runs_v.push_back(cur);
	}
	std::sort(runs_v.begin(), runs_v.end());

	std::vector<uint8> runs;
	uint32 prev_start = 0;
	for(  size_t i = 0;  i < runs_v.size();  i++  ) {
		put_varint(runs, runs_v[i].start - prev_start);
		put_varint(runs, runs_v[i].length);
		runs.push_back(runs_v[i].id);
		runs.push_back(runs_v[i].coverage);
		prev_start = runs_v[i].start;
	}

	body.assign(XR32_HEADER_LEN, 0);
	body[0] = 'X'; body[1] = 'R'; body[2] = '3'; body[3] = '2';
	body[4] = XR32_V2_VERSION;
	flags |= XR32_FLAG_CRC32;
	body[5] = flags;
	body[6] = XR32_V2_CLASS_REGISTRY;
	put16(body, 8, (uint16)w);
	put16(body, 10, (uint16)h);
	put16(body, 12, (uint16)x);
	put16(body, 14, (uint16)y);
	body[16] = zoomable;
	put16(body, 18, img_index);
	put32(body, 20, (uint32)plane.size());
	put32(body, 24, (uint32)runs_v.size());
	put32(body, 28, (uint32)runs.size());
	body.insert(body.end(), plane.begin(), plane.end());
	body.insert(body.end(), runs.begin(), runs.end());

	const uint32 crc = (uint32)crc32( crc32(0L, Z_NULL, 0), &body[0], (uInt)body.size() );
	const size_t at = body.size();
	body.resize(at + 4);
	put32(body, at, crc);

	stlab_semantic_counters.rgba_raw_bytes += plane_len;
	stlab_semantic_counters.rgba_compressed_bytes += plane.size();
	stlab_semantic_counters.semantic_bytes += runs.size();
}


/* ==========================================================================
 * XR32-PRISSI-NIGHT-IMAGE-PROTOTYPE-01 -- EXPERIMENTAL night node XNEX (NOT A FORMAT).
 * Layout in display/stlab_xr32.h; the reader is written from that comment, not from this code.
 * ==========================================================================*/
void stlab_xnex_build_body(const uint8 *night_under, int w, int h, sint16 x, sint16 y, uint16 img_index,
                           std::vector<uint8> &body)
{
	const size_t plane_len = (size_t)w * h * 4;
	uLongf cap = compressBound((uLong)plane_len);
	std::vector<uint8> plane(cap);
	if(  compress2(&plane[0], &cap, night_under, (uLong)plane_len, 9) != Z_OK  ) {
		dbg->fatal("stlab_xnex", "cannot compress a night plane");
	}
	plane.resize(cap);
	body.assign(24, 0);
	body[0] = 'X'; body[1] = 'N'; body[2] = 'E'; body[3] = 'X';
	body[4] = 1;          // version
	body[5] = 0x03;       // deflated + CRC-32
	body[6] = 0;          // transition model 'abc'
	put16(body, 8, (uint16)w);
	put16(body, 10, (uint16)h);
	put16(body, 12, (uint16)x);
	put16(body, 14, (uint16)y);
	put16(body, 16, img_index);
	put32(body, 20, (uint32)plane.size());
	body.insert(body.end(), plane.begin(), plane.end());
	const uint32 crc = (uint32)crc32( crc32(0L, Z_NULL, 0), &body[0], (uInt)body.size() );
	const size_t at = body.size();
	body.resize(at + 4);
	put32(body, at, crc);
}
