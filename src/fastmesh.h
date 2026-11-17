/* fastmesh: three cheap wins for a CPU-bound mesh draw on the SH-4 (docs/PERFORMANCE.md section 7b).
 *  1. Skip early (lazy transform): a model-space pre-pass marks triangles clearly facing away from the eye. Their vertices are
 *     never transformed unless a kept triangle needs them. Your triangle pass culls any triangle that touches an FM_SKIP vertex.
 *  2. Prefetch every vertex: SHZ_PREFETCH a few vertices ahead in the source stream, and allocate the 32 B output line
 *     (shz_dcache_alloc_line) instead of reading it from RAM first.
 *  3. Fewer branches (loop unswitch): the per-vertex "lazy or not" test is hoisted out of the loop; each case gets its own copy
 *     of the loop body (always_inline helper called with a constant flag).
 * Static meshes only for the pre-pass (a skinned pose moves the vertices after the test). Two-sided materials: pass keep = NULL.
 * SH4ZAM for all math. MIT, see LICENSE. */
#ifndef FASTMESH_H
#define FASTMESH_H
#include <stdint.h>
#include <sh4zam/shz_sh4zam.h>

#define FM_SKIP 0x80u   /* keep[] value: no kept triangle uses this vertex (not transformed) */
#define FM_REC  8       /* floats per output record = 32 B, one cache line: x y z written here, slots 3..7 are yours */

typedef struct {
    const uint8_t  *verts;   /* vertex stream: float x, y, z at offset 0 of each vertex */
    uint32_t        stride;  /* bytes per vertex */
    uint32_t        nverts;
    const uint16_t *idx;     /* triangle list, 3 indices a triangle */
    uint32_t        ntris;
} fm_mesh_t;

/* The eye in model space: place^-1 (eye - place.pos). *ok = 0 when the placement is singular (then do not use the pre-pass). */
shz_vec3_t fm_eye_in_model(const shz_mat3x4_t *place, shz_vec3_t eye_world, int *ok);

/* Pre-pass: keep[i] = 0 for a vertex some front (or near edge-on) triangle uses, FM_SKIP otherwise. keep has nverts bytes. */
void fm_face_mark(const fm_mesh_t *m, shz_vec3_t eye_model, uint8_t *keep);

/* Transform pass, model to view (model_view = view x place). out: FM_REC floats a vertex, 32-byte aligned.
 * keep = NULL transforms every vertex. Returns the vertices transformed. */
uint32_t fm_transform(const fm_mesh_t *m, const shz_mat3x4_t *model_view, const uint8_t *keep, float *out);

#endif
