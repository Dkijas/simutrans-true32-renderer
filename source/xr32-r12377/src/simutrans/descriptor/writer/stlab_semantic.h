/*
 * STLAB-RGBA32-END-TO-END-MAKEOBJ-RUNTIME-01 -- semantic extractor for makeobj.
 *
 * NOT FOR COMMIT. Laboratory only.
 *
 * This is a MECHANICAL PORT of the certified solver of
 * STLAB-RGBA32-SEMANTIC-ALPHA-PROBE-01 (tools/probe.py, reconstruct()), plus the record
 * assembly of STLAB-RGBA32-NODE-DESIGN-01 (tools/records.py). Nothing here is a new
 * algorithm, a new threshold or an improvement: every constant and every tie-break is the
 * one the Python oracle uses, and the port is gated on reproducing it exactly.
 *
 * Layering, deliberately separate (the extractor knows nothing about the binary node):
 *
 *   RGBA source pixels -> stlab_extract_semantics() -> semantic records
 *   semantic records   -> the XR32 serializer (a different translation unit)
 *
 * The in-memory record below is NOT the on-disk layout. Never serialise it with sizeof.
 */
#ifndef DESCRIPTOR_WRITER_STLAB_SEMANTIC_H
#define DESCRIPTOR_WRITER_STLAB_SEMANTIC_H

#include <vector>
#include "../../simtypes.h"

/// semantic classes of FORMAT CANDIDATE v1
#define STLAB_CLASS_PLAYER_1 (1)
#define STLAB_CLASS_PLAYER_2 (2)
#define STLAB_CLASS_LIGHT    (3)

/// the solver's own classes, kept verbatim so the port can be compared class by class
enum stlab_solver_class_t {
	STLAB_CLEANLY_RECONSTRUCTABLE = 0,
	STLAB_RECONSTRUCTABLE_WITH_ERROR,
	STLAB_COMPLEX_INTERNAL_BLEND,
	STLAB_NOT_AN_EDGE,
	STLAB_NOT_RECONSTRUCTABLE,
	STLAB_NO_UNDER_COLOUR,          ///< probe.py calls this class "AMBIGUOUS"
	STLAB_SOLVER_CLASS_COUNT
};

/// the three-way reporting taxonomy the mission asks for
enum stlab_taxonomy_t {
	STLAB_CLEAN = 0,
	STLAB_AMBIGUOUS,
	STLAB_UNRECOVERABLE,
	STLAB_TAXONOMY_COUNT
};

stlab_taxonomy_t stlab_taxonomy_of(stlab_solver_class_t c);
const char *stlab_solver_class_name(stlab_solver_class_t c);
const char *stlab_taxonomy_name(stlab_taxonomy_t t);


/// one semantic record, in memory. Position is a linear index into the view it came from.
struct stlab_semantic_record_t {
	uint32 position;
	uint8  semantic_class;
	uint8  semantic_parameter;
	uint8  coverage_alpha8;
	uint8  under_r, under_g, under_b;
	bool   has_under;
};


/// one solved edge candidate, kept so the port can be diagnosed against the oracle
struct stlab_edge_candidate_t {
	uint32 y, x;
	uint8  er, eg, eb;                 ///< the observed pixel
	stlab_solver_class_t cls;
	int    marker_index;
	double residual;
	double alpha_raw;                  ///< before clamping, the value the class test uses
	double alpha_clamped;
	int    coverage;                   ///< round(alpha_clamped * 255), banker's rounding
	bool   has_under;
	uint8  under_r, under_g, under_b;
	bool   has_rival;
	uint8  rival_r, rival_g, rival_b;
	bool   touches_hole;
};


struct stlab_semantic_counters_t {
	uint32 rgba32_images_seen = 0;
	uint32 exact_marker_records = 0;
	uint32 clean_semantic_edges = 0;
	uint32 ambiguous_edges = 0;
	uint32 unrecoverable_edges = 0;
	uint32 player1_records = 0;
	uint32 player2_records = 0;
	uint32 light_marker_records = 0;
	uint32 semantic_records_total = 0;
	uint64 rgba_raw_bytes = 0;
	uint64 rgba_compressed_bytes = 0;
	uint64 semantic_bytes = 0;
	uint32 xr32_nodes_written = 0;
	uint64 extract_micros = 0;
	uint64 compress_micros = 0;
	uint64 serialise_micros = 0;
};

extern stlab_semantic_counters_t stlab_semantic_counters;


/**
 * Extract the semantic records of one RGBA view.
 *
 * @param rgba   w*h*4 bytes, straight RGBA8888, row major, NOT premultiplied and with the
 *               source alpha convention (255 = opaque), not makeobj's inverted one
 * @param out    receives the records, sorted by position, exactly as records.py orders them
 * @param edges  optional: every solved edge candidate, for the equivalence harness
 */
void stlab_extract_semantics(const uint8 *rgba, int w, int h,
                             std::vector<stlab_semantic_record_t> &out,
                             std::vector<stlab_edge_candidate_t> *edges);

/// diagnostic dump command, defined in stlab_semanticdump.cc
int stlab_semanticdump_main(const char *png, const char *recs_out, const char *cand_out,
                            int tile_size);

/// is the laboratory writer enabled at all? (env STLAB_XR32_WRITE=1)
bool stlab_writer_enabled();

#endif
