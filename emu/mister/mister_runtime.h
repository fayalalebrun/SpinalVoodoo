#ifndef SPINALVOODOO_MISTER_RUNTIME_H
#define SPINALVOODOO_MISTER_RUNTIME_H

#include "mister_mailbox.h"

#ifdef __cplusplus
extern "C" {
#endif

int mister_runtime_acquire(void);
int mister_runtime_release(void);
int mister_runtime_exchange(const mister_command *commands, size_t count,
                           uint32_t *results, mister_keys *keys);
int mister_runtime_submit_writes(const mister_command *commands, size_t count);
int mister_runtime_upload_palette(const uint32_t *data, unsigned start, unsigned end);
int mister_runtime_present(const void *rgb565, size_t bytes, size_t pitch);
int mister_runtime_present_renderer(void);
int mister_runtime_stop_presenting(void);
int mister_runtime_map_renderer(uint32_t offset, size_t bytes, volatile uint8_t **base);

#ifdef __cplusplus
}
#endif

#endif
