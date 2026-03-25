/*
 * ref_model.h — Golden reference model for Voodoo rendering.
 *
 * Wraps 86Box's vid_voodoo_render.c + vid_voodoo_texture.c compiled
 * standalone, providing a simple C API for trace_test.cpp.
 */
#ifndef REF_MODEL_H
#define REF_MODEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the reference model. Returns 0 on success. */
int ref_init(int fb_size_mb, int tex_size_mb);

/* Optional per-pixel reference trace export. */
int ref_trace_open(const char *path);
void ref_trace_close(void);

/* Shut down and free all memory. */
void ref_shutdown(void);

/* Write a 32-bit register value (Voodoo register space, 10-bit addr). */
void ref_write_reg(uint32_t addr, uint32_t data);

/* Write a 32-bit word to texture memory. addr is byte offset. */
void ref_write_tex(uint32_t addr, uint32_t data);

/* Write a 32-bit word to framebuffer memory. addr is byte offset. */
void ref_write_fb(uint32_t addr, uint32_t data);

/* Write a 16-bit word to framebuffer memory. addr is byte offset. */
void ref_write_fb_w(uint32_t addr, uint16_t data);

/* Get pointer to raw framebuffer memory. */
uint16_t *ref_get_fb(void);

/* Get framebuffer size in bytes. */
uint32_t ref_get_fb_size(void);

/* Get current draw offset (byte offset into fb_mem where rendering lands). */
uint32_t ref_get_draw_offset(void);

/* Get current front buffer offset (byte offset of displayed/just-rendered content). */
uint32_t ref_get_front_offset(void);

/* Get current row width in bytes. */
uint32_t ref_get_row_width(void);

/* Get current LFB write offset (byte offset where LFB writes land). */
uint32_t ref_get_fb_write_offset(void);

/* Get pointer to raw texture memory (TMU 0). */
uint8_t *ref_get_tex(void);

/* Get texture memory size in bytes. */
uint32_t ref_get_tex_size(void);

/* Debug/state helpers used by roundtrip validation tests. */
uint32_t ref_get_clip_left_right(void);
uint32_t ref_get_clip_lowy_highy(void);
uint32_t ref_get_swapbuffer_cmd(void);
uint32_t ref_get_tex_base_addr(int tmu, int which);
uint32_t ref_get_clut_rgb(int index);
int ref_dump_state_to_dir(const char *dir, uint32_t frame_num);
uint32_t ref_get_fbi_pixels_in(void);
uint32_t ref_get_fbi_pixels_out(void);
uint32_t ref_get_fbi_zfunc_fail(void);
uint32_t ref_get_fbi_afunc_fail(void);
uint32_t ref_get_fbi_chroma_fail(void);
uint32_t ref_get_fbz_mode(void);
uint32_t ref_get_za_color(void);
uint32_t ref_get_triangle_coverage_pixels(void);
uint32_t ref_get_triangle_coverage_total(void);
uint32_t ref_get_triangle_color_writes(void);
uint32_t ref_get_triangle_black_writes(void);
uint32_t ref_get_triangle_depth_only_updates(void);
uint32_t ref_get_triangles_textured(void);
uint32_t ref_get_triangles_untextured(void);
uint32_t ref_get_triangles_all_black_writes(void);
uint32_t ref_get_palette_nonzero_count(int tmu);
void ref_dump_triangle_debug(void);
void ref_dump_layout_debug(void);

/* Load a state snapshot into the reference model.
 * data points to the raw state.bin file contents, size is the file length.
 * Returns 0 on success, -1 on error. */
int ref_load_state(const uint8_t *data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* REF_MODEL_H */
