/*
 * XR32-SEMANTIC-MASK-PIPELINE-01 -- explicit semantic masks for makeobj.
 *
 * NOT FOR COMMIT. Laboratory only.
 *
 * makeobj does NOT guess semantics from the final RGB. The artist (or the exporter) supplies,
 * beside an ordinary image sheet X.png, one 8-bit coverage mask per semantic id:
 *
 *     X.png                 the conventional base image, exactly as today
 *     X.sem.<name>.png      coverage of semantic <name>, 0..255, same size as X.png
 *
 * and opts in per object with the dat key   semantic_masks=1
 *
 * makeobj only VALIDATES and PACKAGES: it solves the under-colour algebraically from the
 * base pixel, the known coverages and the known authoring colours, checks that recomposing
 * gives the base pixel back, and hands the per-pixel contributors to the XR32 v2 serializer.
 * The legacy IMG is built from X.png exactly as before: masks never change it.
 *
 * Semantic meaning, once an object opts in, comes from the masks alone: an exact special
 * colour in X.png that no mask declares is ordinary paint in the XR32 node (and warned
 * about); only the legacy IMG, which cannot say otherwise, still treats it as special.
 */
#ifndef DESCRIPTOR_WRITER_STLAB_SEMANTIC_MASKS_H
#define DESCRIPTOR_WRITER_STLAB_SEMANTIC_MASKS_H

#include <string>
#include <vector>
#include "../../simtypes.h"

/// the stable semantic vocabulary of XR32 v2 (class registry 2): id == makeobj special index
#define STLAB_SEM_ID_COUNT (31)
/// at most this many semantics may share one pixel (the parser enforces the same number)
#define STLAB_SEM_MAX_CONTRIB (4)

struct stlab_sem_vocab_t {
	uint8 id;
	const char *name;      ///< the file-name token, lower case, stable
	                       ///< (the authoring colour is image_t::rgbtab[id], not repeated here)
};
extern const stlab_sem_vocab_t stlab_sem_vocab[STLAB_SEM_ID_COUNT];

/// one contributor of one pixel: the format's own record (stlab_xr32_writer.h)
#include "stlab_xr32_writer.h"

struct stlab_sem_counters_t {
	uint32 sheets_opted_in = 0;
	uint32 sheets_with_masks = 0;
	uint32 masks_loaded = 0;
	uint32 contributors = 0;
	uint32 multi_pixels = 0;          ///< pixels with two or more contributors
	uint32 undeclared_key_pixels = 0; ///< exact key colour without a mask: ordinary in XR32 (warned)
	uint32 dropped_on_holes = 0;      ///< mask coverage on a transparent pixel, ignored
	uint32 trimmed_pixels = 0;        ///< independently rounded masks summing 256..258, trimmed
	uint32 mismatch_pixels = 0;       ///< recomposition differs from the base by more than 2
	uint32 mismatch_max = 0;
	uint32 key_nudged = 0;            ///< an under-colour that came out as the transparency key
	uint32 unknown_mask_names = 0;    ///< X.sem.<name>.png whose name is not in the vocabulary
	uint32 v2_nodes = 0;
};
extern stlab_sem_counters_t stlab_sem_counters;

/// per-object opt-in, set by obj_writer_t::write() from the dat key semantic_masks
void stlab_semantic_masks_set_requested(bool on);
bool stlab_semantic_masks_requested();

/**
 * Load, validate and solve the masks of one sheet.
 *
 * @param sheet_png  the base sheet's path, ending in ".png"
 * @param rgba       the base sheet, w*h*4 straight RGBA (255 == opaque)
 * @param under      receives the under plane (w*h*4) when masks were found
 * @param contribs   receives every contributor, sorted by position then id
 * @return true if at least one mask exists for this sheet (the caller then writes v2)
 *
 * Structural errors (wrong size, more than STLAB_SEM_MAX_CONTRIB semantics or a coverage sum
 * above 255 on one pixel) are fatal: the object opted in, so producing a pak without the
 * semantics it asked for would hide the error.
 */
bool stlab_semantic_masks_solve(const std::string &sheet_png, const uint8 *rgba, int w, int h,
                                std::vector<uint8> &under, std::vector<stlab_sem_contrib_t> &contribs);

/// print the counters (makeobj verbose / STLAB_SEM_REPORT)
void stlab_semantic_masks_report();

#endif
