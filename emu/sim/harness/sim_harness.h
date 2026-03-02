/*
 * sim_harness.h — C-callable API for in-process Verilator CoreSim model.
 *
 * Replaces TCP-based SimServer IPC. All functions are synchronous
 * and execute Verilator ticks internally as needed.
 */

#ifndef SIM_HARNESS_H
#define SIM_HARNESS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the Verilator model. Returns 0 on success, -1 on failure. */
int sim_init(void);

/* Shut down and free the Verilator model. */
void sim_shutdown(void);

/* Write a 32-bit word to the unified CPU bus.
 * addr: 24-bit address (0x000000-0xFFFFFF)
 *   0x000000-0x3FFFFF = registers
 *   0x400000-0x7FFFFF = LFB
 *   0x800000-0xFFFFFF = texture memory
 */
void sim_write(uint32_t addr, uint32_t data);

/* Read a 32-bit word from the unified CPU bus.
 * addr: 24-bit address (same layout as sim_write)
 * Returns the read data.
 */
uint32_t sim_read(uint32_t addr);


/* Spin until pipeline is idle and FIFO is empty.
 * Returns the status register value.
 */
uint32_t sim_idle_wait(void);

/* Advance simulation by n clock cycles. */
void sim_tick(int n);

/* Bulk read from CoreSim's fbRam (bypasses bus protocol).
 * Reads word_count 32-bit words starting at byte_offset into dst. */
void sim_read_fb(uint32_t byte_offset, uint32_t *dst, uint32_t word_count);

/* Bulk write to CoreSim's fbRam (bypasses bus protocol). */
void sim_write_fb_bulk(uint32_t byte_offset, const uint32_t *src, uint32_t word_count);

/* Bulk write to CoreSim's texRam (bypasses bus protocol). */
void sim_write_tex_bulk(uint32_t byte_offset, const uint32_t *src, uint32_t word_count);

/* Bulk read from CoreSim's texRam (bypasses bus protocol).
 * Reads word_count 32-bit words starting at byte_offset into dst. */
void sim_read_tex(uint32_t byte_offset, uint32_t *dst, uint32_t word_count);

/* Set the swap buffer count register directly (bypasses bus protocol).
 * Only the low 2 bits are used. Bit 0 determines front/back buffer assignment. */
void sim_set_swap_count(uint32_t count);

#ifdef __cplusplus
}
#endif

#endif /* SIM_HARNESS_H */
