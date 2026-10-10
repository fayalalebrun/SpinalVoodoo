#ifndef SPINALVOODOO_MISTER_MAILBOX_H
#define SPINALVOODOO_MISTER_MAILBOX_H

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#define MISTER_HOST_BASE UINT32_C(0x3e000000)
#define MISTER_FRAME_BASE UINT32_C(0x3e400000)
#define MISTER_FRAME_SLOT_BYTES UINT32_C(0x400000)
#define MISTER_RENDERER_BASE UINT32_C(0x3f400000)
#define MISTER_RENDERER_BYTES UINT32_C(0x400000)
#define MISTER_MAILBOX_BYTES UINT32_C(0x3000)
#define MISTER_MAILBOX_MAGIC UINT32_C(0x4d565331)
#define MISTER_MAILBOX_ACK UINT32_C(0x80)
#define MISTER_MAILBOX_COMMANDS UINT32_C(0x100)
#define MISTER_MAILBOX_RESULTS UINT32_C(0x2000)
#define MISTER_MAILBOX_KEYS UINT32_C(0x2800)
#define MISTER_MAILBOX_MAX_COMMANDS 256

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t opcode;
    uint32_t address;
    uint32_t value;
    uint32_t mask;
} mister_command;

typedef struct {
    uint8_t bits[64];
    uint32_t sequence;
} mister_keys;

typedef struct {
    volatile uint32_t *words;
    uint32_t sequence;
    uint32_t status;
    unsigned timeout_ms;
    int faulted;
    int pending;
    size_t pending_count;
    uint64_t started, submitted;
    pthread_mutex_t lock;
} mister_mailbox;

int mister_mailbox_attach(mister_mailbox *mailbox, volatile void *memory, size_t bytes,
                          unsigned timeout_ms);
void mister_mailbox_detach(mister_mailbox *mailbox);
int mister_mailbox_exchange(mister_mailbox *mailbox, const mister_command *commands,
                            size_t count, uint32_t *results, mister_keys *keys);
int mister_mailbox_submit_writes(mister_mailbox *mailbox, const mister_command *commands,
                                 size_t count);
int mister_mailbox_drain(mister_mailbox *mailbox);
int mister_command_valid(const mister_command *command);

#ifdef __cplusplus
}
#endif

#endif
