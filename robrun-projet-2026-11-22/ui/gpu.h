/* gpu.h - Drawing interface for the ROB RUN overlay.
 *
 * Call gpu_init() once, then gpu_begin() and gpu_present() for each frame.
 * Direct2D draws screen-space shapes and text; the CPU-rasterized mesh is
 * uploaded separately and composited beneath them. Colors use straight alpha
 * at this interface and are converted as required by the backend.
 */
#ifndef WDE_GPU_H
#define WDE_GPU_H

#include <stdint.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 0xAARRGGBB, straight (not premultiplied) - the backend premultiplies. */
typedef uint32_t RGBA;

#define RRGB(r,g,b)      (0xFF000000u | ((uint32_t)(r)<<16) | ((uint32_t)(g)<<8) | (uint32_t)(b))
#define RARGB(a,r,g,b)   (((uint32_t)(a)<<24) | ((uint32_t)(r)<<16) | ((uint32_t)(g)<<8) | (uint32_t)(b))
#define RCOLORREF(c)     (0xFF000000u | ((uint32_t)GetRValue(c)<<16) | ((uint32_t)GetGValue(c)<<8) | (uint32_t)GetBValue(c))
#define RA(c)            ((int)(((c)>>24)&0xFF))

/* Lifecycle. */
int  gpu_init(HWND hwnd, int w, int h);   /* 1 on success, 0 = caller falls back */
void gpu_shutdown(void);
int  gpu_ready(void);
void gpu_size(int w, int h);

/* Frame. */
void gpu_begin(void);        /* clear to transparent, bind D2D target */
int  gpu_present(void);      /* 1 when accepted, 0 when caller must retry */
int  gpu_frame_ready(void);  /* nonblocking compositor pacing */

/* State. */
void gpu_pen(RGBA c, float width);
void gpu_font(float px);     /* text size for the next r_text calls */
float gpu_esp_distance(float metres); /* select a pixel font size and return its scale */

/* Primitives (screen space, pixels). */
void r_line(float x1, float y1, float x2, float y2);
void r_line_c(float x1, float y1, float x2, float y2, RGBA c, float w);
void r_rect(float x, float y, float w, float h, RGBA c, float stroke);
void r_fill(float x, float y, float w, float h, RGBA c);
void r_fill_meter(float x, float y, float w, float h, float fraction, RGBA top, RGBA middle, RGBA bottom);
void r_ellipse(float cx, float cy, float rx, float ry, RGBA c, float stroke);
void r_fill_ellipse(float cx, float cy, float rx, float ry, RGBA c);
/* Two-stop linear gradient clipped to the ellipse. angle is radians; the
 * gradient runs from (from) to (to) across the diameter along that angle. */
void r_fill_ellipse_gradient(float cx, float cy, float rx, float ry,
                             RGBA from, RGBA to, float angle);
int  r_fill_triangles(const float* xy, int vertex_count, const unsigned short* indices, int triangle_count, RGBA color);
void gpu_mesh_flush(void);
void r_polyline(const float* xy, int n, RGBA c, float w);

/* Text is UTF-8. Coordinates identify the top-left of the line box unless
 * alignment changes the horizontal anchor. RT_OUTLINE adds a dark halo.
 */
#define RT_LEFT    0
#define RT_CENTER  1
#define RT_RIGHT   2
#define RT_OUTLINE 4
#define RT_BOTTOM  8
void r_text(float x, float y, const char* utf8, RGBA c, int flags);
/* HUD chrome text: the system UI face instead of the pixel-grid ESP face. */
void gpu_ui_font(float px);
void r_ui_text(float x, float y, const char* utf8, RGBA c, int flags);
float r_ui_text_width(const char* utf8);
float r_esp_text(float x, float y, const char* utf8, RGBA c, int flags);
/* ESP labels use their own face and size, independent of the menu text. The
 * internal renderer binds the game's pixel font here; when nothing is bound,
 * ESP text falls back to the current UI font. */
void gpu_set_esp_font(void* font, float px);
void r_text_w(float x, float y, const wchar_t* s, int len, RGBA c, int flags);
float r_text_width(const char* utf8);
float r_text_height(void);

#ifdef RR_INTERNAL
/* Same hull fill, with one 0xAARRGGBB colour per vertex (colours[i] overrides
 * color). The chams material table rides this instead of a texture. */
int  r_fill_triangles_shaded(const float* xy, const RGBA* colours, int vertex_count,
                             const unsigned short* indices, int triangle_count, RGBA color);
/* Weapon icon atlas (internal/rr_weapon_icons.h). Hand the renderer the
 * backend's ID3D11Device once it exists; the texture is built on first draw
 * and rebuilt if a different device shows up. */
void gpu_set_device(void* d3d11_device);
#endif

/* Draw RR_WI_* centred on cx with its top edge at y, tinted. Returns the
 * drawn height in pixels (0 when the atlas is not up yet). Both backends
 * carry their own copy: the internal one binds the game's D3D11 device, the
 * external one builds a Direct2D bitmap from the same baked atlas. */
float r_weapon_icon(int icon, float cx, float top, RGBA tint);

/* Character mesh layer.
 * The CPU rasterizer supplies pixels for the changed rectangle. The shader
 * expands the silhouette by outline_px pixels and draws it behind the fill.
 */
void gpu_chams_alloc(int w, int h);
void gpu_chams_upload(const uint32_t* px, int w, int h, int stride,
                      int x0, int y0, int x1, int y1);
void gpu_chams_draw(int outline_px, RGBA outline_col, float opacity);

#ifdef __cplusplus
}
#endif

#endif
