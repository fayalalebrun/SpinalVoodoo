#ifndef MISTER_GLIDE_GUEST_H
#define MISTER_GLIDE_GUEST_H

#include <stdint.h>

typedef struct {
    uint32_t size, pointer, stride, mode, origin;
} mister_guest_lfb_info;

#ifdef __cplusplus
extern "C" {
#endif
int mister_glide_lfb_lock(uint32_t type, uint32_t buffer, uint32_t mode,
                         uint32_t origin, uint32_t pipeline, mister_guest_lfb_info *info);
int mister_glide_lfb_unlock(uint32_t type, uint32_t buffer);
uint32_t mister_glide_lfb_read(uint32_t buffer, uint32_t offset, uint32_t size);
void mister_glide_lfb_write(uint32_t buffer, uint32_t offset, uint32_t value, uint32_t size);
void mister_glide_lfb_reset(void);
#ifdef __cplusplus
}
#endif

#endif
