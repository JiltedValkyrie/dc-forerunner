/* fastmesh (fastmesh.h). KOS path: none (pure CPU); output feeds your PVR submit (pvr_dr_target / store queue).
 * SH4ZAM: shz_mat3x3_inverse + shz_mat3x3_transform_vec3 (eye to model space), shz_vec3_sub / cross / dot (facing test),
 *   shz_xmtrx_load_3x4 + shz_xmtrx_transform_point3 (transform), SHZ_PREFETCH, shz_dcache_alloc_line.
 *   No SH4ZAM equivalent: the keep[] byte marks (memset) and the loop control. */
#include <string.h>
#include "fastmesh.h"

/* Skip a face only when it is CLEARLY back facing: cos(eye ray, normal) > 1e-2, and not a sliver: sin(corner) > 1e-2.
 * Near edge-on or near-collinear faces are left to the screen-space test, whose sign is float noise there (large world
 * coordinates); a tighter 1e-3 margin let a few faces differ from the screen test. Squared to avoid a sqrt. */
#define FM_COS2 1e-4f
/* Prefetch distance in vertices. 8 ahead suits a 24 B vertex (about 6 cache lines); one pref per 8 vertices left most lines
 * cold. Tune per vertex size: aim for roughly 150-200 B ahead. */
#define FM_PREF_AHEAD 8

/* finite test on the bits: -ffast-math lets the compiler drop x == x and isfinite() */
static inline int finite_bits(float f) {
    uint32_t u;
    memcpy(&u, &f, 4);
    return (u & 0x7f800000u) != 0x7f800000u;
}

shz_vec3_t fm_eye_in_model(const shz_mat3x4_t *place, shz_vec3_t eye_world, int *ok) {
    shz_mat3x3_t r3, ri;
    memcpy(r3.elem, place->elem, sizeof r3.elem);                 /* the 3x3 part, column-major like shz_mat3x3_t */
    shz_mat3x3_inverse(&r3, &ri);
    const shz_vec3_t e = shz_mat3x3_transform_vec3(&ri, shz_vec3_sub(eye_world, place->pos));
    *ok = finite_bits(e.x) && finite_bits(e.y) && finite_bits(e.z);
    return e;
}

static inline shz_vec3_t vpos(const fm_mesh_t *m, unsigned i) {
    const float *p = (const float *)(m->verts + i * m->stride);
    return shz_vec3_init(p[0], p[1], p[2]);
}

void fm_face_mark(const fm_mesh_t *m, shz_vec3_t em, uint8_t *keep) {
    memset(keep, FM_SKIP, m->nverts);
    for (uint32_t t = 0; t < m->ntris; t++) {
        const unsigned a = m->idx[3u * t], b = m->idx[3u * t + 1u], c = m->idx[3u * t + 2u];
        const shz_vec3_t a0 = vpos(m, a);
        const shz_vec3_t e  = shz_vec3_sub(a0, em);                   /* eye -> A */
        const shz_vec3_t ab = shz_vec3_sub(vpos(m, b), a0);
        const shz_vec3_t ac = shz_vec3_sub(vpos(m, c), a0);
        const shz_vec3_t n  = shz_vec3_cross(ab, ac);                 /* front = outward n, counter-clockwise winding */
        const float s = shz_vec3_dot(e, n), nn = shz_vec3_dot(n, n);
        if (s > 0.0f && s * s > FM_COS2 * shz_vec3_dot(e, e) * nn
            && nn > FM_COS2 * shz_vec3_dot(ab, ab) * shz_vec3_dot(ac, ac))
            continue;                                                 /* clearly back facing: its vertices stay FM_SKIP */
        keep[a] = 0; keep[b] = 0; keep[c] = 0;
    }
}

/* One loop body, two copies: lazy is a compile-time constant at each call site, so the all-vertices copy has no keep test. */
static inline __attribute__((always_inline)) uint32_t xf_loop(const fm_mesh_t *m, const uint8_t *keep, float *out,
                                                              const int lazy) {
    uint32_t done = 0;
    for (uint32_t i = 0; i < m->nverts; i++) {
        if (lazy && keep[i] == FM_SKIP) continue;
        const uint8_t *vb = m->verts + i * m->stride;
        SHZ_PREFETCH(vb + FM_PREF_AHEAD * m->stride);                 /* every vertex, not one in eight */
        float *r = out + i * FM_REC;
        shz_dcache_alloc_line(r);                                     /* write-only line: no RAM read before the store */
        const float *p = (const float *)vb;
        const shz_vec3_t q = shz_xmtrx_transform_point3(shz_vec3_init(p[0], p[1], p[2]));
        r[0] = q.x; r[1] = q.y; r[2] = q.z;
        done++;
    }
    return done;
}

uint32_t fm_transform(const fm_mesh_t *m, const shz_mat3x4_t *model_view, const uint8_t *keep, float *out) {
    shz_xmtrx_load_3x4(model_view);                                   /* once for the whole mesh */
    return keep ? xf_loop(m, keep, out, 1) : xf_loop(m, keep, out, 0);
}
