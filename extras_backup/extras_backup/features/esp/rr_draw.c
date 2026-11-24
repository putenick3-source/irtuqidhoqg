/* rr_draw.c - Render ROB RUN snapshots with the gpu.cpp drawing interface.
 *
 * The frame includes player labels, boxes, radar, world markers, vehicle hulls
 * and a skinned reference body. Full-rig poses are filtered in world space
 * before projection. When a rig is unavailable, estimated body points keep
 * basic markers visible. rr_game.c supplies snapshots; this module draws them.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "ui/gpu.h"
#include "ui/menu_bridge.h"
#include "game/rr_game.h"
#include "rr_draw.h"
#include "features/aim/rr_aim.h"
#include "game/rig/rr_cham_model.h"
#include "game/rig/rr_cham_hull.h"
#include "game/rig/rr_rig_map.h"
#include "game/rig/rr_body_joints.h"
#include "game/rig/rr_vehicle_cham_model.h"
#ifdef RR_INTERNAL
#include "features/visibility/rr_los.h"
#endif


static RGBA slot_color(int i) {
    BR_Config* b = br_get();
    if (i < 0 || i >= BR_COLOR_COUNT) return RRGB(255, 255, 255);
    const unsigned char* c = b->color[i].rgb;
    return RARGB(255, c[0], c[1], c[2]);
}

/* Held-item icons. Description names carry an item prefix: weapon families
 * ("dp9_m1", "kl545_m3"), and held gear ("afak_1", "frag_grenade_2", ...).
 * The art is baked from the game's own sprite bundles by
 * tools/make_weapon_icons.py into one RGBA atlas. Unknown items draw nothing
 * rather than a wrong gun. */
#include "internal/rr_weapon_icons.h"
#define WI_H RR_WI_H

static int held_icon_kind(const char* name) {
    if (!name || !name[0]) return -1;
    /* Generated table is ordered longest prefix first. */
    for (size_t i = 0; i < RR_WI_PREFIX_COUNT; ++i) {
        const char* prefix = rr_weapon_icon_prefix[i].prefix;
        if (_strnicmp(name, prefix, strlen(prefix)) == 0) return rr_weapon_icon_prefix[i].icon;
    }
    return -1;
}

static float draw_weapon_icon(int icon, float cx, float top, RGBA c) {
    return r_weapon_icon(icon, cx, top, c);
}

static RGBA fade_color(RGBA c, float opacity) {
    if (!(opacity > 0.0f)) return 0;
    if (opacity > 1.0f) opacity = 1.0f;
    return (c & 0x00FFFFFFu) | ((uint32_t)(opacity * 255.0f + 0.5f) << 24);
}

/* Filled aim-assist ring. Gradient mode runs the two picker colors across the
 * diameter; spin rotates that axis a full 360 degrees at the slider's deg/s. */
static void draw_fov_fill(const BR_Config* b, int vw, int vh) {
    if (!b->aa_fov_fill && !b->aa_fov_gradient) return;
    const float opacity = (float)b->aa_fov_opacity / 100.0f;
    if (!(opacity > 0.0f)) return;
    int r = b->aa_fov_px;
    if (r < 10) r = 10;
    if (r > 1200) r = 1200;
    const float cx = (float)vw * 0.5f, cy = (float)vh * 0.5f;
    const RGBA inner = fade_color(slot_color(BR_COLOR_FOV_FILL), opacity);
    if (!b->aa_fov_gradient) {
        r_fill_ellipse(cx, cy, (float)r, (float)r, inner);
        return;
    }
    const RGBA outer = fade_color(slot_color(BR_COLOR_FOV_GRADIENT), opacity);
    float angle = 0.0f;
    if (b->aa_fov_spin) {
        const double degrees = (double)GetTickCount64() * 0.001 * (double)b->aa_fov_spin_speed;
        angle = (float)fmod(degrees * 3.14159265358979323846 / 180.0, 6.28318530717958647692);
    }
    r_fill_ellipse_gradient(cx, cy, (float)r, (float)r, inner, outer, angle);
}

/* Map replicated role names to marker colors for cops, robbers and civilians. */
static RGBA role_color(const char* role) {
    if (strstr(role, "cop")) return RRGB(90, 170, 255);
    if (strstr(role, "robber")) return RRGB(255, 86, 86);
    if (strstr(role, "hubber") || strstr(role, "civilian")) return RRGB(220, 220, 220);
    if (strstr(role, "citizen")) return RRGB(200, 230, 200);
    return 0;
}

static RGBA player_color(const RR_PLAYER* p) {
    if (p->relationships & RR_REL_CLAN) return RRGB(218, 153, 255);
    if (p->relationships & RR_REL_POLICE) return RRGB(86, 255, 246);
    if (p->dead) return slot_color(5);
    RGBA color = role_color(p->role);
    return color ? color : slot_color(0);
}

static void box_stroke(float x, float y, float w, float h, float stroke, RGBA color) {
    r_fill(x, y, w, stroke, color);
    r_fill(x, y + h - stroke, w, stroke, color);
    r_fill(x, y + stroke, stroke, h - 2.0f * stroke, color);
    r_fill(x + w - stroke, y + stroke, stroke, h - 2.0f * stroke, color);
}

static const char* team_caption(const char* role, int* color) {
    if (strstr(role, "robber") || strstr(role, "criminal")) {
        *color = BR_COLOR_CRIMINAL;
        return "CRIMINAL";
    }
    if (strstr(role, "cop") || strstr(role, "police")) {
        *color = BR_COLOR_POLICE;
        return "POLICE";
    }
    if (strstr(role, "hubber") || strstr(role, "civilian") || strstr(role, "citizen")) {
        *color = BR_COLOR_CIVILIAN;
        return "CIVILIAN";
    }
    return NULL;
}

typedef struct {
    uint64_t object, rig;
    int32_t id;
    uint32_t sampled_at;
    uint32_t last_used;
    int crouch, dead, inveh, full_rig;
    RR_V3 root;
    RR_V3 diag_delta, diag_bone;
    int diag_have_delta, diag_have_bone;
    RR_V3 diag_shape[3], diag_shape_older[3];
    int diag_shape_count;
    RR_V3 anchors[RR_MAX_BONES + 2]; /* feet, head, then skeleton points */
    float skin[RR_CHAM_BONES][12];
} CHAM_VISUAL_POSE;
static int g_cham_diag_enabled;
static RR_CHAM_DIAG g_cham_diag;
void cham_diag_enable(void) { memset(&g_cham_diag,0,sizeof(g_cham_diag)); g_cham_diag_enabled=1; }
RR_CHAM_DIAG cham_diag_results(void) { return g_cham_diag; }

/* The two pose tables below are ~2.9 MB together. A manually mapped image pays
 * for its whole SizeOfImage in the target: the loader allocates it and the
 * kernel side writes it before one instruction runs, so big zeroed tables
 * belong on the heap, not in .bss. */
static void* draw_table(size_t bytes) {
    return VirtualAlloc(NULL, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

static const CHAM_VISUAL_POSE* cham_visual_pose(const RR_PLAYER* p) {
    static CHAM_VISUAL_POSE* poses;
    static CHAM_VISUAL_POSE empty;
    static uint32_t serial;
    if (!poses) {
        poses = (CHAM_VISUAL_POSE*)draw_table(sizeof(CHAM_VISUAL_POSE) * RR_MAX_PLAYERS);
        if (!poses) return &empty;
    }
    ++serial;
    CHAM_VISUAL_POSE* previous = &poses[0];
    for (int i = 0; i < RR_MAX_PLAYERS; ++i) {
        CHAM_VISUAL_POSE* candidate = &poses[i];
        if (candidate->object == p->addr && candidate->id == p->id && candidate->rig == p->cham_rig) {
            previous = candidate;
            break;
        }
        if (serial - candidate->last_used > serial - previous->last_used) previous = candidate;
    }
    previous->last_used = serial;
    int same = previous->object == p->addr && previous->id == p->id && previous->rig == p->cham_rig &&
               previous->crouch == p->crouch && previous->dead == p->dead && previous->inveh == p->inveh &&
               previous->full_rig == p->full_rig;
    uint32_t elapsed = p->cham_sample_at - previous->sampled_at;
    if (same && elapsed == 0) {
        if (g_cham_diag_enabled) ++g_cham_diag.repeated;
        return previous;
    }

    const float* first_bone = p->cham_skin[0];
    /* A skin matrix includes the inverse bind transform. Its translation
       moves as the torso rotates, even when the character root is stationary. */
    RR_V3 root = p->mesh_bones ? p->pos :
        (RR_V3){first_bone[3], first_bone[7], first_bone[11]};
    float dx = root.x - previous->root.x;
    float dy = root.y - previous->root.y;
    float dz = root.z - previous->root.z;
    int reset = !same || elapsed >= 100 || dx*dx + dy*dy + dz*dz > 0.64f;
    if (g_cham_diag_enabled) {
        ++g_cham_diag.fresh;
        int identity = previous->object == p->addr && previous->id == p->id && previous->rig == p->cham_rig;
        if (!identity) ++g_cham_diag.initial;
        else {
            if (!same) ++g_cham_diag.state;
            if (elapsed >= 100) ++g_cham_diag.gap;
            if (dx*dx+dy*dy+dz*dz > 0.64f) ++g_cham_diag.jump;
            if (elapsed > g_cham_diag.max_gap_ms) g_cham_diag.max_gap_ms=elapsed;
            float jump=sqrtf(dx*dx+dy*dy+dz*dz);
            if (jump > g_cham_diag.max_jump_m) g_cham_diag.max_jump_m=jump;
            RR_V3 bone={p->cham_skin[RR_RIG_M_HEAD_1_JNT][3]-root.x,p->cham_skin[RR_RIG_M_HEAD_1_JNT][7]-root.y,p->cham_skin[RR_RIG_M_HEAD_1_JNT][11]-root.z};
            if (same && elapsed>0 && elapsed<100) {
                if (jump>.01f) {
                    ++g_cham_diag.moving_samples;
                    float prev_len=sqrtf(previous->diag_delta.x*previous->diag_delta.x + previous->diag_delta.y*previous->diag_delta.y + previous->diag_delta.z*previous->diag_delta.z);
                    float dot=dx*previous->diag_delta.x+dy*previous->diag_delta.y+dz*previous->diag_delta.z;
                    if (previous->diag_have_delta && prev_len>.01f && dot<-.5f*jump*prev_len) ++g_cham_diag.reversals;
                }
                if (previous->diag_have_bone) {
                    float bx=bone.x-previous->diag_bone.x, by=bone.y-previous->diag_bone.y, bz=bone.z-previous->diag_bone.z;
                    float delta=sqrtf(bx*bx+by*by+bz*bz);
                    if (delta>.15f) ++g_cham_diag.bone_jumps;
                    if (delta>g_cham_diag.max_bone_delta_m) g_cham_diag.max_bone_delta_m=delta;
                }
                const int indices[3]={RR_RIG_M_HEAD_1_JNT,RR_RIG_L_ARM_2_JNT,RR_RIG_R_ARM_2_JNT};
                for (int q=0;q<3;++q) {
                    const float* mat=p->cham_skin[indices[q]];
                    RR_V3 now_shape={mat[3]-root.x,mat[7]-root.y,mat[11]-root.z};
                    RR_V3 last=previous->diag_shape[q], older=previous->diag_shape_older[q];
                    float dx1=now_shape.x-last.x,dy1=now_shape.y-last.y,dz1=now_shape.z-last.z;
                    float dx2=now_shape.x-older.x,dy2=now_shape.y-older.y,dz2=now_shape.z-older.z;
                    if (previous->diag_shape_count>=2 && dx1*dx1+dy1*dy1+dz1*dz1>.0225f && dx2*dx2+dy2*dy2+dz2*dz2<.0064f) {
                        if (q==0) ++g_cham_diag.head_spikes;
                        else if (q==1) ++g_cham_diag.left_spikes;
                        else ++g_cham_diag.right_spikes;
                    }
                    previous->diag_shape_older[q]=last;
                    previous->diag_shape[q]=now_shape;
                }
                if (previous->diag_shape_count<2) ++previous->diag_shape_count;
            } else {
                previous->diag_shape_count=0;
            }
            previous->diag_delta=(RR_V3){dx,dy,dz}; previous->diag_have_delta=same && elapsed>0 && elapsed<100;
            previous->diag_bone=bone; previous->diag_have_bone=1;
        }
        if (!identity) { previous->diag_have_delta=0; previous->diag_have_bone=0; previous->diag_shape_count=0; }
    }
    /* Keep the filtered rig close to the current pose while damping small
     * animation jitter. A long filter makes moving targets visibly trail. */
    float blend = (float)elapsed / ((float)elapsed + 3.0f);
    if (reset) {
        memcpy(previous->skin, p->cham_skin, sizeof(previous->skin));
    } else {
        /* Carry the entire pose with the live root immediately. Filter only
         * the remaining bone motion so the body stays with the runner. */
        for (int bone = 0; bone < RR_CHAM_BONES; ++bone) {
            float* smoothed = previous->skin[bone];
            const float* live = p->cham_skin[bone];
            if(!isfinite(live[0]) || !isfinite(smoothed[0])) {
                memcpy(smoothed,live,sizeof(previous->skin[bone]));continue;
            }
            /* Keep the root and torso responsive; damp arm/hand jitter and
             * damp the legs separately while seated in a moving vehicle. */
            float bone_blend=blend;
            if (bone>=RR_RIG_L_ARM_0_JNT && bone<=RR_RIG_R_ARMTWIST_JNT)
                bone_blend=(float)elapsed/((float)elapsed+20.0f);
            else if (p->inveh && bone>=RR_RIG_L_LEG_1_JNT && bone<=RR_RIG_R_FOOT_2_JNT)
                bone_blend=(float)elapsed/((float)elapsed+14.0f);
            smoothed[3] += dx;
            smoothed[7] += dy;
            smoothed[11] += dz;
            for (int component = 0; component < 12; ++component)
                smoothed[component] += (live[component] - smoothed[component]) * bone_blend;
        }
    }
    /* Project markers from the same filtered world pose as the mesh. Keep
       camera motion immediate by applying this before screen projection. */
    for (int i = 0; i < RR_MAX_BONES + 2; ++i) {
        if (p->full_rig && i>=2) {
            int bone=i-2;
            if(!isfinite(p->bone[bone][0]) || !isfinite(p->bone[bone][1]) || !isfinite(p->bone[bone][2])) {
                previous->anchors[i]=(RR_V3){NAN,NAN,NAN};continue;
            }
            const float* m=previous->skin[bone];
            const float* a=rr_cham_bones[bone].anchor;
            previous->anchors[i]=(RR_V3){
                m[0]*a[0]+m[1]*a[1]+m[2]*a[2]+m[3],
                m[4]*a[0]+m[5]*a[1]+m[6]*a[2]+m[7],
                m[8]*a[0]+m[9]*a[1]+m[10]*a[2]+m[11]};
            continue;
        }
        RR_V3 live;
        if (i == 0) live = p->pos;
        else if (i == 1) live = p->head;
        else memcpy(&live, p->bone[i - 2], sizeof(live));
        RR_V3* anchor = &previous->anchors[i];
        if (reset) *anchor = live;
        else {
            anchor->x += dx + (live.x - anchor->x - dx) * blend;
            anchor->y += dy + (live.y - anchor->y - dy) * blend;
            anchor->z += dz + (live.z - anchor->z - dz) * blend;
        }
    }
    previous->object = p->addr;
    previous->id = p->id;
    previous->rig = p->cham_rig;
    previous->sampled_at = p->cham_sample_at;
    previous->crouch = p->crouch;
    previous->dead = p->dead;
    previous->inveh = p->inveh;
    previous->full_rig = p->full_rig;
    previous->root = root;
    return previous;
}
void cham_diag_sample(const RR_PLAYER* player) {
    if (player && player->valid && !player->local && !player->is_vehicle && !player->is_loot &&
        player->cham_valid && player->cham_rig && player->cham_sample_at) (void)cham_visual_pose(player);
}

typedef struct {
    uint64_t object;
    int32_t id;
    uint32_t used, frame;
    int have;
    int pose_kind;
    RR_V3 point[RR_MAX_BONES + 2]; /* newest sample                      */
#ifdef RR_INTERNAL
    LONGLONG last_draw_at;
    int vehicle_id;
    int bone_count;
#endif
    RR_V3 out[RR_MAX_BONES + 2];   /* pose advanced to this present        */
} ESP_RENDER_POSE;
static ESP_RENDER_POSE* g_render_pose;
static uint32_t g_render_serial;
#ifdef RR_INTERNAL
static LONGLONG g_qpc_freq;

static LONGLONG render_now(void) {
    if (!g_qpc_freq) {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        g_qpc_freq = f.QuadPart > 0 ? f.QuadPart : 1;
    }
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return c.QuadPart;
}
#endif

/* Interpolate measured world positions before projecting with the current camera. */
static const RR_V3* cham_render_anchors(const RR_PLAYER* p) {
    if (!g_render_pose) {
        g_render_pose = (ESP_RENDER_POSE*)draw_table(sizeof(ESP_RENDER_POSE) * RR_MAX_PLAYERS);
        if (!g_render_pose) return NULL;
    }
    ESP_RENDER_POSE* pick = &g_render_pose[0];
    for (int i = 0; i < RR_MAX_PLAYERS; ++i) {
        ESP_RENDER_POSE* candidate = &g_render_pose[i];
        if (candidate->have && candidate->object == p->addr && candidate->id == p->id) {
            pick = candidate;
            break;
        }
        if (g_render_serial - candidate->used > g_render_serial - pick->used) pick = candidate;
    }
    pick->used = g_render_serial;
    if (pick->have && pick->frame == g_render_serial &&
        pick->object == p->addr && pick->id == p->id) return pick->out;

    const int identical = pick->have && pick->object == p->addr && pick->id == p->id;
    (void)identical;
    RR_V3 measured[RR_MAX_BONES + 2];
#ifndef RR_INTERNAL
    const int live = p->cham_valid && p->cham_rig && p->cham_sample_at && (!p->inveh || p->full_rig);
    if (live) {
        memcpy(measured, cham_visual_pose(p)->anchors, sizeof measured);
    } else
#endif
    {
        measured[0] = p->pos;
        measured[1] = p->head;
        memcpy(&measured[2], p->bone, sizeof p->bone);
    }
    const int pose_kind = (p->full_rig ? 1 : 0) | (p->inveh ? 2 : 0) | (p->hidden_in_object ? 4 : 0);
#ifdef RR_INTERNAL
    const LONGLONG now = render_now();
    const LONGLONG elapsed = now - pick->last_draw_at;
    const float dx = measured[0].x - pick->point[0].x;
    const float dy = measured[0].y - pick->point[0].y;
    const float dz = measured[0].z - pick->point[0].z;
    const int reset = !identical || pick->pose_kind != pose_kind || pick->bone_count != p->nbones ||
                      (p->inveh && pick->vehicle_id != p->vehicle_id) ||
                      elapsed > g_qpc_freq / 5 || dx*dx + dy*dy + dz*dz > 4.0f;
    /* Advance on every present, even if a fresh sample arrives every time. */
    const double smoothing_ticks = (double)g_qpc_freq * 0.004;
    const float t = reset ? 1.0f : (float)((double)elapsed / ((double)elapsed + smoothing_ticks));
    /* Keep live rig translation responsive while filtering relative joint motion. */
    const RR_V3 carry = !reset && (p->inveh || p->render_anchor)
        ? (RR_V3){measured[0].x - pick->out[0].x, measured[0].y - pick->out[0].y,
                  measured[0].z - pick->out[0].z}
        : (RR_V3){0};
    memcpy(pick->point, measured, sizeof measured);
    pick->last_draw_at = now;
    pick->vehicle_id = p->vehicle_id;
    pick->bone_count = p->nbones;
#else
    /* Present the newest sample. The snapshot thread runs at ~700 Hz and the
     * render pass is paced by the signature, so a prev-to-newest blend could
     * only ever resolve to the older pose on the frame it arrived in: one
     * frame of lag, which is the trail the overlay was showing. Snapshot
     * rate is already above display rate, so the newest sample is the only
     * correct thing to draw. */
    memcpy(pick->point, measured, sizeof measured);
    const float t = 1.0f;
    (void)t;
#endif
    pick->pose_kind = pose_kind;
    pick->have = 1;
    pick->object = p->addr;
    pick->id = p->id;
    pick->frame = g_render_serial;
    for (int i = 0; i < RR_MAX_BONES + 2; ++i) {
        const RR_V3 a = pick->point[i];
#ifdef RR_INTERNAL
        const RR_V3 b = {pick->out[i].x + carry.x, pick->out[i].y + carry.y, pick->out[i].z + carry.z};
        if (t >= 1.0f || !isfinite(a.x) || !isfinite(a.y) || !isfinite(a.z) ||
            !isfinite(b.x) || !isfinite(b.y) || !isfinite(b.z)) {
            pick->out[i] = a;
            continue;
        }
        pick->out[i].x = b.x + (a.x - b.x) * t;
        pick->out[i].y = b.y + (a.y - b.y) * t;
        pick->out[i].z = b.z + (a.z - b.z) * t;
#else
        pick->out[i] = a;
#endif
    }
    return pick->out;
}

typedef struct { float left, top, right, bottom; int count; } SCREEN_BOUNDS;

static void include_point(SCREEN_BOUNDS* bounds, float x, float y, float pad) {
    if (!isfinite(x) || !isfinite(y)) return;
    if (!bounds->count) {
        *bounds = (SCREEN_BOUNDS){x-pad, y-pad, x+pad, y+pad, 1};
    } else {
        bounds->left = fminf(bounds->left, x-pad);
        bounds->top = fminf(bounds->top, y-pad);
        bounds->right = fmaxf(bounds->right, x+pad);
        bounds->bottom = fmaxf(bounds->bottom, y+pad);
        ++bounds->count;
    }
}

#ifdef RR_INTERNAL
/* Skinned palette for the exported body mesh: the live pose palette with each
 * bone's translation moved to the joint position this frame actually draws at,
 * so the mesh stays welded to the interpolated skeleton instead of lagging a
 * frame behind it. Bones without a finite palette or anchor are dropped, and
 * every vertex weighted to them drops with them. */
static void cham_prepare_skin(const RR_PLAYER* sampled, const RR_PLAYER* displayed,
                              float skin[RR_CHAM_BONES][12], unsigned char usable[RR_CHAM_BONES]) {
    memcpy(skin, sampled->cham_skin, sizeof(float) * 12 * RR_CHAM_BONES);
    for (int bone = 0; bone < RR_CHAM_BONES; ++bone) {
        const float* anchor = rr_cham_bones[bone].anchor;
        for (int axis = 0; axis < 3; ++axis) {
            float* row = &skin[bone][axis * 4];
            row[3] = displayed->bone[bone][axis] -
                     (row[0]*anchor[0] + row[1]*anchor[1] + row[2]*anchor[2]);
        }
        usable[bone] = rr_rig_is_body_joint(bone) &&
                       isfinite(skin[bone][3]) && isfinite(skin[bone][7]) && isfinite(skin[bone][11]);
    }
}

static SCREEN_BOUNDS body_mesh_bounds(const RR_PLAYER* sampled, const RR_PLAYER* displayed,
                                      const RR_CAMERA* cam, int vw, int vh) {
    SCREEN_BOUNDS bounds = {0};
    if (!sampled->full_rig || !sampled->cham_valid || sampled->nbones != RR_CHAM_BONES)
        return bounds;

    float skin[RR_CHAM_BONES][12];
    unsigned char usable[RR_CHAM_BONES];
    cham_prepare_skin(sampled, displayed, skin, usable);

    RR_V3 vertices[RR_CHAM_VERTICES];
    float projected[RR_CHAM_VERTICES * 2];
    int count = 0;
    for (int i = 0; i < RR_CHAM_VERTICES; ++i) {
        const RR_CHAM_VERTEX* vertex = &rr_cham_vertices[i];
        RR_V3 world = {0};
        float total = 0;
        for (int j = 0; j < 4; ++j) {
            const int bone = vertex->bone[j];
            const float weight = vertex->weight[j];
            if (weight <= 0.0001f || bone >= RR_CHAM_BONES || !usable[bone]) continue;
            const float* m = skin[bone];
            world.x += weight * (m[0]*vertex->p[0] + m[1]*vertex->p[1] + m[2]*vertex->p[2] + m[3]);
            world.y += weight * (m[4]*vertex->p[0] + m[5]*vertex->p[1] + m[6]*vertex->p[2] + m[7]);
            world.z += weight * (m[8]*vertex->p[0] + m[9]*vertex->p[1] + m[10]*vertex->p[2] + m[11]);
            total += weight;
        }
        if (total < 0.99f || total > 1.01f) continue;
        const float dx = world.x - displayed->pos.x, dy = world.y - displayed->pos.y;
        const float dz = world.z - displayed->pos.z;
        if (dx*dx + dy*dy + dz*dz <= 36.0f) vertices[count++] = world;
    }
    /* A partial palette should use the joint estimate, not a cropped mesh. */
    if (count < RR_CHAM_VERTICES * 9 / 10) return bounds;
    game_w2s_many(cam, vertices, count, vw, vh, projected);
    for (int i = 0; i < count; ++i)
        include_point(&bounds, projected[i*2], projected[i*2+1], 0);
    return bounds;
}

/* Chams: the decimated body mesh (game/rig/rr_cham_hull.h), skinned by the
 * same palette the box uses and filled with one flat tint. The triangles go
 * into the shared ESP draw list, which is 16-bit indexed, so the pass draws
 * nearest-first within a vertex budget instead of letting a full lobby wrap
 * the index buffer. Farther bodies keep their box and labels. */
/* Chams materials (BR_CHAM_* in menu_bridge.h). The hull carries no texture, so
 * each look is a function of the projected stroke, its offset from the body
 * centre and the clock. The menu colour stays the base tint; the material only
 * pushes it around, which keeps the swatch meaningful. */
typedef struct { float cx, cy, half_w, half_h, seconds; } CHAM_SHADE;

static RGBA cham_mix(RGBA a, RGBA b, float weight) {
    if (!(weight > 0.0f)) return a;
    if (weight > 1.0f) weight = 1.0f;
    const float keep = 1.0f - weight;
    const unsigned ar = (a >> 16) & 0xFFu, ag = (a >> 8) & 0xFFu, ab = a & 0xFFu, aa = a >> 24;
    const unsigned br = (b >> 16) & 0xFFu, bg = (b >> 8) & 0xFFu, bb = b & 0xFFu, ba = b >> 24;
    const unsigned r = (unsigned)((float)ar * keep + (float)br * weight + 0.5f);
    const unsigned g = (unsigned)((float)ag * keep + (float)bg * weight + 0.5f);
    const unsigned bl = (unsigned)((float)ab * keep + (float)bb * weight + 0.5f);
    const unsigned al = (unsigned)((float)aa * keep + (float)ba * weight + 0.5f);
    return (r << 16) | (g << 8) | bl | (al << 24);
}

static RGBA cham_alpha(RGBA c, float factor) {
    int alpha = (int)((float)(c >> 24) * factor + 0.5f);
    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;
    return (c & 0x00FFFFFFu) | ((uint32_t)alpha << 24);
}

static RGBA cham_shade(int style, RGBA base, float x, float y, const CHAM_SHADE* s) {
    /* u: 0 at the top of the silhouette, 1 at the feet. radial: 0 at the body
     * centre line, 1 at the silhouette. */
    const float u = (y - (s->cy - s->half_h)) / (2.0f * s->half_h);
    const float dx = (x - s->cx) / s->half_w;
    const float dy = (y - s->cy) / s->half_h;
    const float radial = fminf(1.0f, sqrtf(dx*dx + dy*dy));
    const float t = s->seconds;
    switch (style) {
        case BR_CHAM_ICE: {
            /* Frost collects at the head and thins toward the feet. */
            const RGBA frost = RRGB(205, 246, 255);
            RGBA c = cham_mix(base, frost, 0.30f + 0.45f * (1.0f - u));
            return cham_alpha(c, 0.85f + 0.15f * (1.0f - u));
        }
        case BR_CHAM_PLASMA: {
            const float wave = 0.5f + 0.5f * sinf(t * 2.6f + x * 0.02f + y * 0.015f);
            return cham_mix(base, cham_mix(base, RRGB(255, 255, 255), 0.65f), wave);
        }
        case BR_CHAM_EMBER: {
            /* Heat at the chest, charcoal at the feet, breathing. */
            const RGBA flame = cham_mix(RRGB(90, 18, 8), RRGB(255, 150, 40), 1.0f - u);
            RGBA c = cham_mix(flame, base, 0.30f);
            return cham_alpha(c, 0.80f + 0.20f * (0.5f + 0.5f * sinf(t * 3.1f + u * 3.0f)));
        }
        case BR_CHAM_OBSIDIAN:
            /* Near-black glass with a glossy rim. */
            return cham_alpha(cham_mix(RRGB(10, 10, 14), base, 0.30f), 0.40f + 0.60f * radial);
        case BR_CHAM_ABYSS:
            /* Deep water: the body sinks into the dark toward the feet. */
            return cham_alpha(cham_mix(RRGB(6, 12, 30), base, 0.35f), 0.40f + 0.60f * (1.0f - u));
        case BR_CHAM_PULSE:
            return cham_alpha(base, 0.40f + 0.60f * (0.5f + 0.5f * sinf(t * 5.0f)));
        case BR_CHAM_INVISIBLE:
            /* Only the silhouette holds colour; the interior reads through. */
            return cham_alpha(cham_mix(base, RRGB(235, 250, 255), 0.35f),
                              0.08f + 0.62f * radial * radial);
        case BR_CHAM_MARBLE: {
            const float vein = 0.5f + 0.5f * sinf(dx * 3.0f + dy * 6.0f +
                                                  2.2f * sinf(dy * 1.8f + t * 0.4f));
            const float smooth = vein * vein * (3.0f - 2.0f * vein);
            return cham_mix(cham_mix(base, RRGB(8, 8, 10), 0.65f),
                            cham_mix(base, RRGB(255, 255, 255), 0.55f), smooth);
        }
        default:
            return base;
    }
}

void rr_cham_texture(unsigned char* pixels, int style, uint32_t color, float seconds) {
    const CHAM_SHADE shade = {0, 0, 20, 40, seconds};
    for (int y=0; y<RR_CHAM_TEXTURE_SIZE; ++y) {
        for (int x=0; x<RR_CHAM_TEXTURE_SIZE; ++x) {
            const float px = 40.0f * x / (RR_CHAM_TEXTURE_SIZE-1) - 20.0f;
            const float py = 40.0f - 80.0f * y / (RR_CHAM_TEXTURE_SIZE-1);
            const uint32_t c = cham_shade(style, color | 0xFF000000u, px, py, &shade);
            const int i = (y*RR_CHAM_TEXTURE_SIZE+x)*4;
            pixels[i] = (unsigned char)(c >> 16);
            pixels[i+1] = (unsigned char)(c >> 8);
            pixels[i+2] = (unsigned char)c;
            pixels[i+3] = (unsigned char)((c >> 24) * (color >> 24) / 255u);
        }
    }
}

#define CHAM_VERTEX_BUDGET 40000
static int draw_cham_body(const RR_PLAYER* sampled, const RR_PLAYER* displayed,
                          const RR_CAMERA* cam, int vw, int vh, RGBA colour, int style) {
    if (!sampled->full_rig || !sampled->cham_valid || sampled->nbones != RR_CHAM_BONES)
        return 0;
    float skin[RR_CHAM_BONES][12];
    unsigned char usable[RR_CHAM_BONES];
    cham_prepare_skin(sampled, displayed, skin, usable);

    RR_V3 world[RR_CHAM_HULL_VERTICES];
    unsigned char mapped[RR_CHAM_HULL_VERTICES];
    float xy[RR_CHAM_HULL_VERTICES * 2];
    int any = 0;
    for (int i = 0; i < RR_CHAM_HULL_VERTICES; ++i) {
        const RR_CHAM_VERTEX* vertex = &rr_cham_hull_vertices[i];
        RR_V3 point = {0};
        float total = 0;
        for (int j = 0; j < 4; ++j) {
            const int bone = vertex->bone[j];
            const float weight = vertex->weight[j];
            if (weight <= 0.0001f || bone >= RR_CHAM_BONES || !usable[bone]) continue;
            const float* m = skin[bone];
            point.x += weight * (m[0]*vertex->p[0] + m[1]*vertex->p[1] + m[2]*vertex->p[2] + m[3]);
            point.y += weight * (m[4]*vertex->p[0] + m[5]*vertex->p[1] + m[6]*vertex->p[2] + m[7]);
            point.z += weight * (m[8]*vertex->p[0] + m[9]*vertex->p[1] + m[10]*vertex->p[2] + m[11]);
            total += weight;
        }
        /* A vertex that lost a bone keeps its slot but is masked below; the
         * index table stays untouched. A corrupt palette is bounded by the
         * same 6 m radius the box uses. */
        int ok = total > 0.99f && total < 1.01f;
        if (ok) {
            const float dx = point.x - displayed->pos.x, dy = point.y - displayed->pos.y;
            const float dz = point.z - displayed->pos.z;
            ok = dx*dx + dy*dy + dz*dz <= 36.0f;
        }
        mapped[i] = (unsigned char)ok;
        if (ok) any = 1;
        world[i] = ok ? point : displayed->pos;
    }
    if (!any) return 0;
    game_w2s_many(cam, world, RR_CHAM_HULL_VERTICES, vw, vh, xy);
    float min_x = 1e30f, max_x = -1e30f, min_y = 1e30f, max_y = -1e30f;
    for (int i = 0; i < RR_CHAM_HULL_VERTICES; ++i) {
        if (!mapped[i]) { xy[i*2] = NAN; xy[i*2+1] = NAN; continue; }
        if (!isfinite(xy[i*2]) || !isfinite(xy[i*2+1])) { mapped[i] = 0; xy[i*2] = NAN; xy[i*2+1] = NAN; continue; }
        min_x = fminf(min_x, xy[i*2]); max_x = fmaxf(max_x, xy[i*2]);
        min_y = fminf(min_y, xy[i*2+1]); max_y = fmaxf(max_y, xy[i*2+1]);
    }
    if (!(max_x > min_x) || !(max_y > min_y)) return 0;
    CHAM_SHADE shade;
    shade.cx = (min_x + max_x) * 0.5f;
    shade.cy = (min_y + max_y) * 0.5f;
    shade.half_w = fmaxf(1.0f, (max_x - min_x) * 0.5f);
    shade.half_h = fmaxf(1.0f, (max_y - min_y) * 0.5f);
    shade.seconds = (float)(GetTickCount64() % 600000u) * 0.001f;
    RGBA shaded[RR_CHAM_HULL_VERTICES];
    for (int i = 0; i < RR_CHAM_HULL_VERTICES; ++i)
        shaded[i] = mapped[i] ? cham_shade(style, colour, xy[i*2], xy[i*2+1], &shade) : colour;
    return r_fill_triangles_shaded(xy, shaded, RR_CHAM_HULL_VERTICES, rr_cham_hull_triangles,
                                   RR_CHAM_HULL_TRIANGLES, colour);
}

/* Nearest-first cham quota: the per-frame vertex budget divided by one body. */
typedef struct { float dist; int index; } CHAM_ORDER;
static int cham_order_cmp(const void* a, const void* b) {
    const float da = ((const CHAM_ORDER*)a)->dist, db = ((const CHAM_ORDER*)b)->dist;
    return (da > db) - (da < db);
}
#endif

static float joint_radius(int bone) {
    if (bone <= RR_RIG_M_SPINE_3_JNT) return 0.16f;
    if (bone <= RR_RIG_M_HEAD_2_JNT) return 0.12f;
    if (bone < RR_RIG_L_ARM_0_JNT) return 0.025f;
    if (bone >= RR_RIG_L_LEG_1_JNT) return 0.10f;
    if ((bone >= RR_RIG_L_INDEX_1_JNT && bone <= RR_RIG_L_THUMB_3_JNT) ||
        (bone >= RR_RIG_R_INDEX_1_JNT && bone <= RR_RIG_R_THUMB_3_JNT)) return 0.035f;
    return 0.085f;
}

static SCREEN_BOUNDS rig_bounds(const RR_PLAYER* p, const RR_CAMERA* cam, int vw, int vh) {
    SCREEN_BOUNDS bounds = {0};
    if (p->nbones < 3 || p->nbones > RR_CHAM_BONES || p->hidden_in_object) return bounds;
    const float forward_length = sqrtf(cam->fwd.x*cam->fwd.x + cam->fwd.y*cam->fwd.y + cam->fwd.z*cam->fwd.z);
    const float lens = tanf(cam->fov * 3.14159265f / 360.0f);
    if (!(forward_length > 1e-6f) || !(lens > 1e-6f) || !isfinite(forward_length) || !isfinite(lens))
        return bounds;
    const float pixels_per_depth = 0.5f * vh * forward_length / lens;
    RR_V3 joints[RR_CHAM_BONES];
    float points[RR_CHAM_BONES*2];
    memcpy(joints, p->bone, sizeof(joints[0]) * p->nbones);
    game_w2s_many(cam, joints, p->nbones, vw, vh, points);
    if (!p->full_rig) {
        float x, y;
        if (game_w2s(cam, &p->pos, vw, vh, &x, &y)) include_point(&bounds, x, y, 0.0f);
        if (game_w2s(cam, &p->head, vw, vh, &x, &y)) include_point(&bounds, x, y, 0.0f);
    }
    for (int bone = 0; bone < p->nbones; ++bone) {
        if (p->full_rig && !rr_rig_is_body_joint(bone)) continue;
        const RR_V3* joint = &joints[bone];
        const float depth = (joint->x-cam->pos.x)*cam->fwd.x +
                            (joint->y-cam->pos.y)*cam->fwd.y +
                            (joint->z-cam->pos.z)*cam->fwd.z;
        if (!(depth > 0.05f * forward_length)) continue;
        /* A bone lies inside the body. Expand it by its part's radius in world units. */
        const float radius = p->full_rig ? joint_radius(bone) : 0.22f;
        const float pad = fminf(0.2f * vh, radius * pixels_per_depth / depth);
        include_point(&bounds, points[2*bone], points[2*bone+1], pad);
    }
    return bounds;
}

/* Draw anatomical parent links. Attachment helpers can be parked at the
 * character origin and must not appear as body skeleton segments. Facial and
 * finger joints are mapped when the mesh ships them but are not drawn: at ESP
 * range they are sub-pixel noise on top of the head and hands. */
static void draw_full_skeleton(const RR_PLAYER* p, const RR_CAMERA* cam, int vw, int vh) {
    RR_V3 world[RR_CHAM_BONES]; float xy[RR_CHAM_BONES*2];
    /* draw_esp already replaced the joints with the render-rate filtered pose,
     * so this draws the same body the box was measured from. */
    memcpy(world,p->bone,sizeof(world));
    rr_aim_head_center(p, &world[RR_RIG_M_HEAD_1_JNT]);
    game_w2s_many(cam,world,RR_CHAM_BONES,vw,vh,xy);
    for (int b=0;b<RR_CHAM_BONES;++b) {
        int parent=rr_cham_bones[b].parent;
        /* The rig hangs both shoulders off spine_3 (upper chest), which draws
         * a V into the sternum at ESP scale. Lift the collar links to the neck
         * so the torso reads as a Y from the neck. */
        if (b==RR_RIG_L_ARM_0_JNT || b==RR_RIG_R_ARM_0_JNT) parent=RR_RIG_M_NECK_JNT;
        if (parent<0 || parent>=RR_CHAM_BONES) continue;
        if(!rr_rig_skeleton_joint(b) || !rr_rig_skeleton_joint(parent))continue;
        float x=xy[b*2],y=xy[b*2+1],px=xy[parent*2],py=xy[parent*2+1];
        if (isfinite(x) && isfinite(y) && isfinite(px) && isfinite(py)) r_line(px,py,x,y);
    }
}

/* Build short labels without variadic formatting in the draw loop. */
static void fmt_num(char* out, size_t n, unsigned v, const char* suffix) {
    char tmp[16];
    int k = 0;
    size_t o = 0;
    if (!n) return;
    do { tmp[k++] = (char)('0' + (v % 10u)); v /= 10u; } while (v && k < 15);
    while (k > 0 && o + 1 < n) out[o++] = tmp[--k];
    while (suffix && *suffix && o + 1 < n) out[o++] = *suffix++;
    out[o] = 0;
}
static void fmt_copy(char* out, size_t n, const char* a, const char* b) {
    size_t o = 0;
    if (!n) return;
    if (a) while (*a && o + 1 < n) out[o++] = *a++;
    if (b) while (*b && o + 1 < n) out[o++] = *b++;
    out[o] = 0;
}

static int world_marker_visible(const RR_PLAYER* p, const BR_Config* b) {
    return ((p->is_loot == 1 && b->show_loot) || (p->is_loot == 2 && b->show_loot_nodes))
               ? br_loot_rarity_visible(b, p->loot_rarity)
               : (p->is_loot == 3 && b->show_containers) ||
           (p->is_loot == 4 && b->show_doors) ||
           (p->is_loot == 5 && b->show_dropped_loot) ||
           (p->is_loot == 6 && b->show_vehicle_loot);
}

static void draw_radar(const RR_PLAYER* arr, int n, const RR_CAMERA* cam,
                       int vw, int vh, const BR_Config* b) {
    const RR_PLAYER* local = NULL;
    for (int i = 0; i < n; ++i) {
        if (arr[i].valid && arr[i].local == RR_LOCAL_CONFIRMED) {
            local = &arr[i];
            break;
        }
    }
    /* Match the centre of ROB RUN's top-left minimap at 1920x1080; scale the
     * HUD coordinates with height so the overlay follows the game client. */
    float width = fminf((float)vh * (210.0f / 1080.0f), (float)vw * .25f);
    float height = fminf((float)vh * (180.0f / 1080.0f), (float)vw * .25f);
    if (!local || width < 70.0f || height < 70.0f || b->radar_range <= 0) return;
    float fx = cam->fwd.x, fz = cam->fwd.z;
    float length = hypotf(fx, fz);
    if (!(length > 0.01f) || !isfinite(length)) {
        fx = sinf(local->rot_yaw);
        fz = cosf(local->rot_yaw);
    } else {
        fx /= length;
        fz /= length;
    }
    float cx = (float)vh * (148.0f / 1080.0f);
    float cy = (float)vh * (130.0f / 1080.0f);
    float left = cx - width * .5f, top = cy - height * .5f;
    float plot_x = width * .5f - 6.0f, plot_y = height * .5f - 6.0f;
    float range = (float)b->radar_range;
    /* The transparent centre leaves the game's map and icons visible. */
    r_rect(left, top, width, height, RARGB(180, 230, 224, 245), 1.0f);

    for (int i = 0; i < n; ++i) {
        const RR_PLAYER* p = &arr[i];
        if (!p->valid || p->local) continue;
        if (p->is_vehicle && (!b->radar_world || !b->show_vehicle_markers)) continue;
        if (p->is_loot) {
            if (!b->radar_world || !world_marker_visible(p, b)) continue;
        } else if (!p->is_vehicle && !rr_relationship_allowed(p->relationships, b->show_clan, b->show_police)) continue;
        float dx = p->pos.x - local->pos.x, dz = p->pos.z - local->pos.z;
        float lateral=dx*fz-dz*fx, forward=dx*fx+dz*fz;
        if (!(fabsf(lateral)<=range && fabsf(forward)<=range)) continue;
        float x = cx + lateral*(plot_x/range), y = cy - forward*(plot_y/range);
        if (p->is_vehicle) {
            r_fill_ellipse(x, y, 2.8f, 2.8f, RRGB(255, 214, 110));
            continue;
        }
        if (p->is_loot) {
            RGBA color = p->is_loot == 1 ? RRGB(235, 235, 235) :
                         p->is_loot == 2 ? RRGB(201, 155, 255) :
                         p->is_loot == 3 ? RRGB(255, 190, 92) :
                         p->is_loot == 4 ? RRGB(200, 200, 200) :
                         p->is_loot == 5 ? RRGB(111, 232, 145) : RRGB(105, 178, 255);
            r_fill_ellipse(x, y, 2.3f, 2.3f, color);
            continue;
        }
        RGBA color = player_color(p);
        if (p->dead) r_ellipse(x, y, 3.0f, 3.0f, color, 1.0f);
        else {
            r_fill_ellipse(x, y, 4.5f, 4.5f, RRGB(16, 11, 25));
            r_fill_ellipse(x, y, 3.1f, 3.1f, color);
        }
    }
}

static float tracer_depth(const RR_CAMERA* cam, RR_V3 p) {
    return (p.x-cam->pos.x)*cam->fwd.x + (p.y-cam->pos.y)*cam->fwd.y +
           (p.z-cam->pos.z)*cam->fwd.z;
}

static void draw_tracers(const RR_CAMERA* cam, int vw, int vh, const BR_Config* b) {
    if (!cam || !cam->valid) return;
    RR_TRACER tr[32];
    const uint32_t life = (uint32_t)(b->tracer_ms > 0 ? b->tracer_ms : 900);
    const int n = game_tracers(tr, _countof(tr), life);
    const RGBA base = slot_color(BR_COLOR_TRACER);
    const float width = b->tracer_width / 10.0f;
    const float glow = b->tracer_glow / 10.0f;
    const float near_plane = 0.055f;
    for (int i=0; i<n; ++i) {
        RR_V3 a = tr[i].a, end = tr[i].b;
        const float start_depth = tracer_depth(cam, a), end_depth = tracer_depth(cam, end);
        if (!(end_depth > near_plane)) continue;
        if (start_depth < near_plane) {
            const float mix = (near_plane-start_depth)/(end_depth-start_depth);
            a.x += mix*(end.x-a.x); a.y += mix*(end.y-a.y); a.z += mix*(end.z-a.z);
        }
        float ax = NAN, ay = NAN, bx = NAN, by = NAN;
        game_w2s(cam, &a, vw, vh, &ax, &ay);
        game_w2s(cam, &end, vw, vh, &bx, &by);
        if (!isfinite(ax) || !isfinite(ay) || !isfinite(bx) || !isfinite(by)) continue;
        const float fade = 1.0f - (float)tr[i].age_ms/life;
        if (glow > 0 && hypotf(bx-ax, by-ay) > 1) {
            for (int layer=3; layer>0; --layer)
                r_line_c(ax, ay, bx, by, fade_color(base, fade*glow*(4-layer)*0.055f),
                         width + 2.0f*layer*glow);
        }
        const RGBA color = fade_color(base, fade);
        if (hypotf(bx-ax, by-ay) > 1) r_line_c(ax, ay, bx, by, color, width);
        if (tr[i].hit) r_fill_ellipse(bx, by, 2.0f, 2.0f, color);
    }
}

static void draw_rain(int vw, int vh, int amount) {
    const int drops = amount * 2;
    const DWORD tick = GetTickCount() % 120000;
    for (int i=0; i<drops; ++i) {
        const uint32_t seed = (uint32_t)(i+1)*0x9E3779B9u;
        const float depth = 0.35f + ((seed >> 10) & 255u)/255.0f;
        const float y = fmodf((float)(seed % (unsigned)vh) + tick*0.85f*depth, (float)vh+42.0f)-24.0f;
        const float x = (float)((seed >> 12) % (unsigned)(vw+40)) - y*0.13f;
        r_line_c(x, y, x-2.0f*depth, y+16.0f*depth,
                 RARGB((int)(54.0f*depth), 176, 202, 224), depth);
    }
}

/* Use the camera basis so arrows keep pointing toward players behind the view. */
static void draw_oof(const RR_PLAYER* arr, int n, const RR_CAMERA* cam,
                     int vw, int vh, const BR_Config* b) {
    if (!arr || !cam || !cam->valid || n <= 0) return;
    const RGBA col = slot_color(BR_COLOR_OOF);
    if (!col) return;
    const float size = fminf(fmaxf((float)b->oof_size, 3.0f), 25.0f);
    const float cx = vw * 0.5f, cy = vh * 0.5f;
    const float radius = fminf(fmaxf((float)b->oof_radius, 30.0f), 500.0f);
    const int style = b->oof_style < 0 || b->oof_style > 2 ? 0 : b->oof_style;
    static const float points[3][7][2] = {
        {{1,0},{-.30f,.76f},{-.64f,.76f},{-1,.4256f},{-1,-.4256f},{-.64f,-.76f},{-.30f,-.76f}},
        {{1,0},{-1,.76f},{-.4f,0},{-1,-.76f}},
        {{1,0},{-1,.76f},{-1,-.76f}}
    };
    static const int counts[] = {7, 4, 3};
    static const unsigned short indices[] = {0,1,2,0,2,3,0,3,4,0,4,5,0,5,6};
    const int vertices = counts[style];
    if (cx <= 30.0f || cy <= 30.0f) return;

    for (int i = 0; i < n; ++i) {
        const RR_PLAYER* p = &arr[i];
        if (!p->valid || p->local || p->is_vehicle || p->is_loot || p->dead) continue;
        if (b->max_distance > 0 && p->dist > (float)b->max_distance / 100.0f) continue;
        float sx, sy;
        if (game_w2s(cam, &p->head, vw, vh, &sx, &sy) &&
            sx >= 0.0f && sx < (float)vw && sy >= 0.0f && sy < (float)vh) continue;

        const float rx = p->head.x - cam->pos.x;
        const float ry = p->head.y - cam->pos.y;
        const float rz = p->head.z - cam->pos.z;
        float dx = rx * cam->right.x + ry * cam->right.y + rz * cam->right.z;
        float dy = -(rx * cam->up.x + ry * cam->up.y + rz * cam->up.z);
        float len = sqrtf(dx*dx + dy*dy);
        if (!isfinite(len) || len < 1e-4f) { dx = 0.0f; dy = -1.0f; len = 1.0f; }
        dx /= len; dy /= len;

        const float ex = fminf(fmaxf(cx + dx * radius, 30.0f), (float)vw - 30.0f);
        const float ey = fminf(fmaxf(cy + dy * radius, 30.0f), (float)vh - 30.0f);
        const float px = -dy, py = dx;
        float xy[16];
        for (int layer = 3; layer >= 0; --layer) {
            const float scale = size + 2.0f * layer;
            for (int k = 0; k < vertices; ++k) {
                const float x = points[style][k][0], y = points[style][k][1];
                xy[2*k] = ex + (x*dx + y*px)*scale;
                xy[2*k+1] = ey + (x*dy + y*py)*scale;
            }
            const RGBA tint = layer ? fade_color(col, (float)(4-layer)*0.07f) : col;
            r_fill_triangles(xy, vertices, indices, vertices - 2, tint);
        }
        xy[2*vertices] = xy[0];
        xy[2*vertices+1] = xy[1];
        r_polyline(xy, vertices + 1, RARGB(220,0,0,0), 1.0f);
    }
}

static void draw_loot(const RR_PLAYER* p, const RR_CAMERA* cam, int vw, int vh) {
    static const char* const rarity_names[] = { "", "Common", "Uncommon", "Rare", "Epic", "Legendary", "Exotic" };
    static const RGBA rarity_colors[] = {
        RRGB(230, 230, 230), RRGB(230, 230, 230), RRGB(111, 232, 145), RRGB(105, 178, 255),
        RRGB(211, 155, 255), RRGB(255, 190, 92), RRGB(255, 113, 149)
    };
    float x, y;
    if (!game_w2s(cam, &p->pos, vw, vh, &x, &y)) return;
    int rarity = p->loot_rarity >= 1 && p->loot_rarity <= 6 ? p->loot_rarity : 0;
    const char* name = p->name[0] ? p->name : "Loot";
    const char* prefix = p->is_loot == 2 ? "Node: " : "";
    char label[RR_NAME_MAX + 48];
    snprintf(label, sizeof(label), "%s%s%s%s (%.0f m)", prefix, name,
             rarity ? " / " : "", rarity_names[rarity], p->dist);
    RGBA color = rarity_colors[rarity];
    if (p->is_loot == 2) r_ellipse(x, y, 4.0f, 4.0f, color, 1.5f);
    else r_fill_ellipse(x, y, 4.0f, 4.0f, color);
    r_esp_text(x, y + 6.0f, label, color, RT_CENTER | RT_OUTLINE);
}

/* An ESP box can come from three sources on any given frame: the skinned mesh
 * bounds, the articulated rig bounds, or the projected feet/head pair. The
 * source drops out for single frames when a rig or mesh sample is missed, and
 * rr_game.c re-anchors a character's pos/head to the resolved rig on the
 * frames that pose validates, so a seated or distant box changes size and
 * flips back and forth between two rects. Each rect is low-passed per player,
 * and a source change is held until the same source reports twice, so one
 * dropped sample cannot resize the box. Real discontinuities (respawn, stance
 * change, teleport, long gap) reset instead of blending.
 *
 * Internal keeps the hook too: in-process reads are fast but a pose can still
 * drop for a frame while the game rebuilds a rig, and an unstable source made
 * the box snap between the mesh rect and the feet/head estimate. */
typedef struct {
    uint64_t obj;
    int32_t  id;
    uint32_t used;
    DWORD    at;
    float    x, y, w, h;
    int      state, src, pending_src, pending_n, pending_state, pending_state_n, valid;
} ESP_BOX;
static ESP_BOX g_esp_boxes[RR_MAX_PLAYERS];

static ESP_BOX* esp_box_slot(uint64_t obj, int32_t id) {
    static uint32_t serial;
    ++serial;
    ESP_BOX* pick = &g_esp_boxes[0];
    for (int i = 0; i < RR_MAX_PLAYERS; ++i) {
        ESP_BOX* c = &g_esp_boxes[i];
        if (c->valid && c->obj == obj && c->id == id) { c->used = serial; return c; }
        if (!pick->valid || (c->valid && c->used < pick->used)) pick = c;
    }
    memset(pick, 0, sizeof(*pick));
    pick->obj = obj;
    pick->id = id;
    pick->valid = 1;
    pick->used = serial;
    return pick;
}

static void esp_box_stabilize(uint64_t obj, int32_t id, int state, int src, float dist,
                              float* bx, float* by, float* bw, float* bh) {
    ESP_BOX* c = esp_box_slot(obj, id);
    const DWORD now = GetTickCount();
    const DWORD dt = now - c->at;
    const float nx = *bx, ny = *by, nw = *bw, nh = *bh;
    const float jx = (nx + nw * 0.5f) - (c->x + c->w * 0.5f);
    const float jy = (ny + nh * 0.5f) - (c->y + c->h * 0.5f);

    /* A joint swinging out of the bounds can double the width, and a crouch
     * flag read as unknown flips the head estimate; both are pose noise, not
     * teleports. Only a hard discontinuity rebuilds the filter. */
    if (!c->at || dt > 250 ||
        nw < c->w * 0.33f || nw > c->w * 3.0f || nh < c->h * 0.33f || nh > c->h * 3.0f ||
        jx * jx + jy * jy > 300.0f * 300.0f) {
        c->at = now;
        c->state = state;
        c->src = src;
        c->pending_src = src;
        c->pending_n = 0;
        c->pending_state = state;
        c->pending_state_n = 0;
        c->x = nx; c->y = ny; c->w = nw; c->h = nh;
        return;
    } else if (src != c->src) {
        if (src != c->pending_src) {
            c->pending_src = src;
            c->pending_n = 0;
        }
        if (++c->pending_n < 2) {
            /* Present the last rect while the new source proves itself. */
            *bx = c->x; *by = c->y; *bw = c->w; *bh = c->h;
            c->at = now;
            return;
        }
        c->src = src;
        c->pending_n = 0;
    } else {
        c->pending_src = src;
        c->pending_n = 0;
    }
    if (state != c->state) {
        if (state != c->pending_state) {
            c->pending_state = state;
            c->pending_state_n = 0;
        }
        if (++c->pending_state_n < 4) {
            *bx = c->x; *by = c->y; *bw = c->w; *bh = c->h;
            c->at = now;
            return;
        }
        c->state = state;
        c->pending_state_n = 0;
    } else {
        c->pending_state = state;
        c->pending_state_n = 0;
    }

    /* The rect is built from the live pose every sample, so smoothing it only
     * made the box trail a running player - which reads as the ESP predicting
     * the movement. The source and state holds above absorb one-frame
     * dropouts; the rect itself is published as measured. */
    (void)dist;
    c->at = now;
    c->x = nx; c->y = ny; c->w = nw; c->h = nh;
    *bx = nx; *by = ny; *bw = nw; *bh = nh;
}

void draw_esp(const RR_PLAYER* arr, int n, const RR_CAMERA* cam, int vw, int vh) {
    BR_Config* b = br_get();
    b->telem_cham_n=0;
    /* The screen-space FOV circle does not depend on a player, a camera or the
     * ESP master toggle; drawing it behind the enabled gate kept the overlay
     * looking dead in the lobby and with ESP off. */
    if (vw < 16 || vh < 16) return;
    if (!arr || !cam || !cam->valid || n < 0) n = 0;
    if (b->aa_on && b->aa_show_fov) draw_fov_fill(b, vw, vh);
    if (cam && cam->valid && b->world_rain) draw_rain(vw, vh, b->world_rain);
    if (b->show_tracers) draw_tracers(cam, vw, vh, b);
    if (!b->enabled) return;

    float line_w = (float)(b->line_w < 1 ? 1 : (b->line_w > 4 ? 4 : b->line_w));
    if (n>RR_MAX_PLAYERS) n=RR_MAX_PLAYERS;
    ++g_render_serial;

#ifdef RR_INTERNAL
    unsigned char cham_allow[RR_MAX_PLAYERS];
    memset(cham_allow, 0, sizeof cham_allow);
    if ((b->cham_on && b->cham_opacity > 0) ||
        (b->local_chams && b->local_chams_opacity > 0)) {
        CHAM_ORDER order[RR_MAX_PLAYERS];
        int count = 0;
        for (int i = 0; i < n; ++i) {
            if (!arr[i].valid || arr[i].is_loot || arr[i].is_vehicle) continue;
            if (arr[i].local) {
                if (!b->local_chams || b->local_chams_opacity <= 0) continue;
            } else {
                if (!b->cham_on || b->cham_opacity <= 0) continue;
                if (b->cham_vis_check &&
                    rr_los_check(cam->pos, arr[i].head) != RR_LOS_CLEAR) continue;
            }
            order[count].dist = arr[i].dist;
            order[count].index = i;
            ++count;
        }
        qsort(order, (size_t)count, sizeof(order[0]), cham_order_cmp);
        const int quota = CHAM_VERTEX_BUDGET / RR_CHAM_HULL_VERTICES;
        if (count > quota) count = quota;
        for (int i = 0; i < count; ++i) cham_allow[order[i].index] = 1;
    }
#endif

    if (b->show_self) {
        const RR_PLAYER* self = NULL;
        for (int i = 0; i < n; ++i) if (arr[i].valid && arr[i].local) { self = &arr[i]; break; }
        r_esp_text(12.0f, 12.0f, !self ? "SELF: unavailable" :
               self->hidden_in_object ? "SELF: hiding object detected" : "SELF: no hiding object detected",
               RRGB(227, 215, 252), RT_LEFT | RT_OUTLINE);
    }
    int drawn = 0;
    for (int i = 0; i < n; i++) {
        /* Apply the player drawing budget while walking the mixed snapshot. */
        if (drawn >= RR_MAX_PLAYERS) continue;
        const RR_PLAYER* p = &arr[i];
        if (!p->valid) continue;
        const int is_local = p->local;
        if (is_local && !br_local_esp_enabled(b)) continue;
        if (!is_local && !rr_relationship_allowed(p->relationships, b->show_clan, b->show_police)) continue;
        /* Menu settings are stored in centimetres; positions use metres. */
        if (!is_local && b->max_distance > 0 && p->dist > (float)b->max_distance / 100.0f) continue;
        const int draw_box = is_local ? b->local_box : b->show_box;
        const int draw_skeleton = is_local ? b->local_skeleton : b->show_skeleton;
        const int draw_name = is_local ? b->local_name : b->show_name;
        const int draw_weapon = is_local ? b->local_weapon : b->show_weapon;
        const int draw_distance = is_local ? b->local_distance : b->show_distance;
        const int draw_armor = is_local ? b->local_armor : b->show_armor;

        if (p->is_loot) {
            int visible = (p->is_loot == 1 && b->show_loot) ||
                          (p->is_loot == 2 && b->show_loot_nodes) ||
                          world_marker_visible(p, b);
            if (visible && (p->is_loot > 2 || br_loot_rarity_visible(b, p->loot_rarity)))
                draw_loot(p, cam, vw, vh);
            continue;
        }
        if (p->is_vehicle) {
            if (b->show_vehicle_markers) {
                float x, y;
                if (game_w2s(cam, &p->pos, vw, vh, &x, &y)) {
                    char label[RR_NAME_MAX + 32];
                    const char* name = p->name[0] ? p->name : (p->is_vehicle == 2 ? "Helicopter" : "Car");
                    if (b->show_vehicle_occupants && p->occupants_known)
                        snprintf(label, sizeof label, "%s [%d] (%.0f m)", name, p->occupants, p->dist);
                    else snprintf(label, sizeof label, "%s (%.0f m)", name, p->dist);
                    RGBA color = RRGB(255, 214, 110);
                    r_fill_ellipse(x, y, 4.0f, 4.0f, color);
                    r_esp_text(x, y + 6.0f, label, color, RT_CENTER | RT_OUTLINE);
                }
            }
            continue;
        }

        RR_PLAYER visual;
        const RR_V3* anchors = cham_render_anchors(p);
        if (anchors) {
            visual = *p;
            visual.pos = anchors[0];
            visual.head = anchors[1];
            memcpy(visual.bone, &anchors[2], sizeof(visual.bone));
            p = &visual;
        }

        float fx = NAN, fy = NAN, hx = NAN, hy = NAN;
        /* A seated body without a rig is estimated at torso height above the
         * replicated transform, which sits on the seat: a 1.45 m head estimate
         * reached over the roof and the widened box framed the whole car. The
         * resolved rig wins whenever it exists. */
        const int seated_estimate = p->inveh && !p->mesh_bones;
        RR_V3 top = p->head;
        if (seated_estimate) { top.x = p->pos.x; top.y = p->pos.y + 1.15f; top.z = p->pos.z; }
        int feet_ok = game_w2s(cam, &p->pos, vw, vh, &fx, &fy);
        int head_ok = game_w2s(cam, &top, vw, vh, &hx, &hy);
        SCREEN_BOUNDS bounds = {0};
        int source = 0;
#ifdef RR_INTERNAL
        if (!p->hidden_in_object) bounds = body_mesh_bounds(&arr[i], p, cam, vw, vh);
#endif
        if (bounds.count < 3 && !p->hidden_in_object) {
            bounds = rig_bounds(p, cam, vw, vh);
            if (bounds.count >= 3) source = 2;    /* articulated rig */
        }
        float bx, by, bw, bh;
        if (bounds.count >= 3 && bounds.right > bounds.left && bounds.bottom > bounds.top) {
            if (bounds.right < 0 || bounds.left > vw || bounds.bottom < 0 || bounds.top > vh) continue;
            /* A joint just in front of the camera can project far off screen. */
            bx = floorf(fmaxf(bounds.left, -(float)vw)) - 2.0f;
            by = floorf(fmaxf(bounds.top, -(float)vh)) - 2.0f;
            bw = fmaxf(4.0f, ceilf(fminf(bounds.right, 2.0f*vw)) + 2.0f - bx);
            bh = fmaxf(4.0f, ceilf(fminf(bounds.bottom, 2.0f*vh)) + 2.0f - by);
        } else {
            source = 3;                            /* projected feet/head */
            if ((!feet_ok && !head_ok) || !isfinite(fx) || !isfinite(fy) || !isfinite(hx) || !isfinite(hy)) continue;
            bh = fmaxf(4.0f, fabsf(fy - hy));
            if (p->inveh) bh = fminf(bh, 420.0f);
            bw = bh * (seated_estimate ? 0.62f : 0.52f) + fabsf(hx - fx);
            bx = floorf((fx + hx - bw) * 0.5f) - 1.0f;
            by = floorf(fminf(hy, fy)) - 2.0f;
            bw = fmaxf(4.0f, roundf(bw) + 2.0f);
            bh = roundf(bh) + 4.0f;
            if (bx < -200 || bx > vw + 200 || by < -200 || by > vh + 200) continue;
        }
        /* Rig bounds pad each joint by its body radius, so the frame reads
         * taller than its player: it starts above the crown and hangs past the
         * boots. The projected head and feet are the body's own vertical
         * extent, so the Y side snaps to them while the joints stay
         * authoritative across X, where arms and gear reach wider than the two
         * anchors. Mesh bounds already trace the body and are left alone. */
        if (source != 0 && head_ok && feet_ok &&
            isfinite(hy) && isfinite(fy) && fabsf(fy - hy) > 8.0f) {
            const float top = fminf(hy, fy) - 1.0f;
            const float bottom = fmaxf(hy, fy) + 2.0f;
            by = top;
            bh = fmaxf(4.0f, bottom - top);
        }
        esp_box_stabilize(p->addr, p->id,
                          (p->crouch > 0) | (p->inveh << 1) | (p->dead << 2) |
                          ((p->hidden_in_object != 0) << 3),
                          source, p->dist, &bx, &by, &bw, &bh);
        float center = bx + bw * 0.5f;
        const float scale = gpu_esp_distance(p->dist);
        const float player_line = fmaxf(1.0f, roundf(line_w * scale));

        RGBA col = player_color(p);
        if (p->dead) {
            /* downed: dim, but still visible */
            int a = 255 * 60 / 100;
            col = (col & 0x00FFFFFFu) | ((uint32_t)a << 24);
        }

#ifdef RR_INTERNAL
        /* Overlay triangles have no scene depth buffer. */
        if (cham_allow[i] && !p->hidden_in_object) {
            int percent = is_local ? b->local_chams_opacity : b->cham_opacity;
            if (percent < 0) percent = 0;
            if (percent > 100) percent = 100;
            const int tinted = is_local ? b->local_chams_tint : b->cham_tint;
            const RGBA base = tinted ? slot_color(is_local ? BR_COLOR_LOCAL_CHAM : BR_COLOR_OVERLAY_CHAM) : RRGB(255,255,255);
            const RGBA tint = (base & 0x00FFFFFFu) | ((uint32_t)(percent * 255 / 100) << 24);
            if (draw_cham_body(&arr[i], p, cam, vw, vh, tint, is_local ? b->local_chams_style : b->cham_style))
                ++b->telem_cham_n;
        }
#endif


        if (draw_box) {
            float stroke = fminf(player_line, floorf(fminf(bw, bh) * 0.5f));
            box_stroke(bx - 1, by - 1, bw + 2, bh + 2, stroke + 2, RRGB(0, 0, 0));
            box_stroke(bx, by, bw, bh, stroke, slot_color(is_local ? BR_COLOR_LOCAL_BOX : p->dead ? BR_COLOR_DOWN : BR_COLOR_BOX));
        }
        if (draw_skeleton && (!p->inveh || p->mesh_bones) && !p->hidden_in_object) {
            gpu_pen(slot_color(is_local ? BR_COLOR_LOCAL_SKELETON : BR_COLOR_SKELETON), fmaxf(1.0f, (float)b->skeleton_width));
            /* Only a mapped rig draws. The 7-point estimate carries no joint
             * identity, and drawing it in array order put the neck inside the
             * chest and strung limbs across the body; the box already shows
             * where a body is when the rig is unavailable. */
            if (p->full_rig && p->nbones == RR_CHAM_BONES) draw_full_skeleton(p,cam,vw,vh);
        }

        /* Exactly two pixels between stacked label rows. r_esp_text reports the
         * row height it drew, so every row measures the same gap. */
        const float label_gap = 2.0f;
        const int centered = RT_CENTER | RT_OUTLINE;
        float above = by - 2.0f;
        if (draw_name) {
            char label[RR_NAME_MAX];
            if (p->local) fmt_copy(label, sizeof(label), "YOU", 0);
            else if (p->name[0]) fmt_copy(label, sizeof(label), p->name, 0);
            else if (p->id) { label[0] = '#'; fmt_num(label + 1, sizeof(label) - 1, (unsigned)p->id, 0); }
            else label[0] = 0;
            if (label[0]) {
                above -= r_esp_text(center, above, label, slot_color(is_local ? BR_COLOR_LOCAL_NAME : BR_COLOR_NAME), centered | RT_BOTTOM) + label_gap;
            }
        }
        if (!is_local && b->show_team) {
            int team_color;
            const char* team = team_caption(p->role, &team_color);
            if (team) {
                above -= r_esp_text(center, above, team, slot_color(team_color), centered | RT_BOTTOM) + label_gap;
            }
        }
        if (!is_local && p->hidden_in_object)
            above -= r_esp_text(center, above, p->hidden_in_object == 2 ? "POSSIBLY HIDING" : "HIDING",
                                col, centered | RT_BOTTOM) + label_gap;

        if (draw_armor && p->armor == 1)
            r_esp_text(bx + bw + 5.0f, by - 2.0f, "ARMOR", slot_color(is_local ? BR_COLOR_LOCAL_ARMOR : BR_COLOR_ARMOR), RT_OUTLINE);

        /* Below the box, top to bottom: distance, vehicle seat, weapon name,
         * then the weapon silhouette. The weapon and seat state used to climb
         * the stack upward with the name, which put the gun above the distance
         * and made the block grow away from the body. */
        float below = by + bh + 4.0f;
        if (draw_distance) {
            char buf[32];
            fmt_num(buf, sizeof(buf), (unsigned)(p->dist + 0.5f), " m");
            below += r_esp_text(center, below, buf, slot_color(is_local ? BR_COLOR_LOCAL_DISTANCE : BR_COLOR_DISTANCE), centered) + label_gap;
        }
        if (!is_local && p->inveh) {
            char vehicle[32];
            const char* kind = p->veh[0] ? p->veh : "vehicle";
            static const char prefix[] = "Inside ";
            size_t k = 0;
            while (k < sizeof(prefix) - 1) vehicle[k] = prefix[k], ++k;
            vehicle[k++] = (char)toupper((unsigned char)kind[0]);
            for (const char* s = kind + 1; *s && k < sizeof(vehicle) - 1; ++s)
                vehicle[k++] = *s;
            vehicle[k] = 0;
            below += r_esp_text(center, below, vehicle, col, centered) + label_gap;
        }
        if (draw_weapon && p->weapon[0]) {
            char weapon[24];
            size_t k = 0;
            for (const char* s = p->weapon; *s && *s != '_' && k < sizeof(weapon) - 1; ++s)
                weapon[k++] = (char)toupper((unsigned char)*s);
            weapon[k] = 0;
            if (k) {
                below += r_esp_text(center, below, weapon, slot_color(is_local ? BR_COLOR_LOCAL_WEAPON : BR_COLOR_WEAPON), centered) + label_gap;
                const int icon = held_icon_kind(p->weapon);
                if (icon >= 0) {
                    /* Item art keeps the game's colours; weapon art is white
                     * so the ESP weapon colour tints it. Advance by the art's
                     * own height so short weapons sit right under the label. */
                    const float drawn =
                        draw_weapon_icon(icon, center, below,
                                         icon >= RR_WI_FIRST_ITEM ? RRGB(255, 255, 255)
                                                                  : slot_color(is_local ? BR_COLOR_LOCAL_WEAPON : BR_COLOR_WEAPON));
                    below += (drawn > 0.0f ? drawn : (float)WI_H) + label_gap;
                }
            }
        }
        if (!is_local && p->dead)
            r_esp_text(center, by + bh * 0.5f, "DOWN", slot_color(BR_COLOR_DOWN), centered);
        drawn++;
    }

    /* Aim-assist FOV circle: same circle that defines the capture zone. */
    if (b->oof_on) draw_oof(arr, n, cam, vw, vh, b);
    if (b->aa_on && b->aa_show_fov) {
        int r = b->aa_fov_px;
        if (r < 10) r = 10;
        if (r > 1200) r = 1200;
        r_ellipse((float)vw * 0.5f, (float)vh * 0.5f, (float)r, (float)r,
                  slot_color(BR_COLOR_FOV), 1.0f);
    }
    if (b->radar_on && n > 0) draw_radar(arr, n, cam, vw, vh, b);

    b->telem_targets = drawn;
}












