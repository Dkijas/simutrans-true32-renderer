/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WRITER_STLAB_MRES_WRITER_H
#define DESCRIPTOR_WRITER_STLAB_MRES_WRITER_H


/**
 * makeobj mres <out.pak> <base.pak> <hires.pak>
 *
 * MRES v2 (MRES-V2-SPEC.md). Copies base.pak unchanged and appends one MRES node
 * as the last child of ROOT. Every base image is paired with the image at the same
 * object name and the same child path in hires.pak - the same .dat compiled at
 * twice the tile size. A hires image's XR32 companion, if the hires pak has one,
 * travels into the MRES node with it.
 *
 * @return 0 on success, 1 on failure (a message has been printed)
 */
int stlab_mres_main(const char *out_name, const char *base_name, const char *hires_name);


#endif
