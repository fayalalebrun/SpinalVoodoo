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

/* Get pointer to raw texture memory (TMU 0). */
uint8_t *ref_get_tex(void);

/* Get texture memory size in bytes. */
uint32_t ref_get_tex_size(void);

/* Load a state snapshot into the reference model.
 * data points to the raw state.bin file contents, size is the file length.
 * Returns 0 on success, -1 on error. */
int ref_load_state(const uint8_t *data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* REF_MODEL_H */
