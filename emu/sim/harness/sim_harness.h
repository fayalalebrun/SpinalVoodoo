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

/* Write a 16-bit halfword to the unified CPU bus.
 * addr keeps its byte address so sub-word destinations can decode the lane.
 */
void sim_write16(uint32_t addr, uint16_t data);

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

/* Drain dirty framebuffer write-buffer lines into fbRam. */
void sim_flush_fb_cache(void);

/* Force framebuffer plane write buffers back to a clean idle state.
 * Useful after bulk-loading fbRam directly, which bypasses normal buffered traffic. */
void sim_invalidate_fb_cache(void);

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

/* Read the current swap buffer count register. */
uint32_t sim_get_swap_count(void);

/* Return current simulation cycle count. */
uint64_t sim_get_cycle(void);

/* Return aggregate bus timing stats collected by the harness. */
uint64_t sim_get_total_read_ticks(void);
uint64_t sim_get_total_read_count(void);
uint64_t sim_get_total_write_ticks(void);
uint64_t sim_get_total_write_count(void);
uint32_t sim_get_pixels_in(void);
uint32_t sim_get_pixels_out(void);
uint32_t sim_get_fb_fill_hits(void);
uint32_t sim_get_fb_fill_misses(void);
uint32_t sim_get_fb_fill_burst_count(void);
uint32_t sim_get_fb_fill_burst_beats(void);
uint32_t sim_get_fb_fill_stall_cycles(void);
uint32_t sim_get_fb_write_stall_cycles(void);
uint32_t sim_get_fb_write_drain_count(void);
uint32_t sim_get_fb_write_full_drain_count(void);
uint32_t sim_get_fb_write_partial_drain_count(void);
uint32_t sim_get_fb_write_drain_reason_full_count(void);
uint32_t sim_get_fb_write_drain_reason_rotate_count(void);
uint32_t sim_get_fb_write_drain_reason_flush_count(void);
uint32_t sim_get_fb_write_drain_dirty_word_total(void);
uint32_t sim_get_fb_write_rotate_blocked_cycles(void);
uint32_t sim_get_fb_write_single_word_drain_count(void);
uint32_t sim_get_fb_write_single_word_drain_start_at_zero_count(void);
uint32_t sim_get_fb_write_single_word_drain_start_at_last_count(void);
uint32_t sim_get_fb_write_rotate_adjacent_line_count(void);
uint32_t sim_get_fb_write_rotate_same_line_gap_count(void);
uint32_t sim_get_fb_write_rotate_other_line_count(void);
uint32_t sim_get_fb_mem_color_write_cmd_count(void);
uint32_t sim_get_fb_mem_aux_write_cmd_count(void);
uint32_t sim_get_fb_mem_color_read_cmd_count(void);
uint32_t sim_get_fb_mem_aux_read_cmd_count(void);
uint32_t sim_get_fb_mem_lfb_read_cmd_count(void);
uint32_t sim_get_fb_mem_color_write_blocked_cycles(void);
uint32_t sim_get_fb_mem_aux_write_blocked_cycles(void);
uint32_t sim_get_fb_mem_color_read_blocked_cycles(void);
uint32_t sim_get_fb_mem_aux_read_blocked_cycles(void);
uint32_t sim_get_fb_mem_lfb_read_blocked_cycles(void);
uint32_t sim_get_fb_read_req_count(void);
uint32_t sim_get_fb_read_req_forward_step_count(void);
uint32_t sim_get_fb_read_req_backward_step_count(void);
uint32_t sim_get_fb_read_req_same_word_count(void);
uint32_t sim_get_fb_read_req_same_line_count(void);
uint32_t sim_get_fb_read_req_other_count(void);
uint32_t sim_get_fb_read_single_beat_burst_count(void);
uint32_t sim_get_fb_read_multi_beat_burst_count(void);
uint32_t sim_get_fb_read_max_queue_occupancy(void);

/* Return non-zero once the harness has hit a fatal stall/abort condition. */
int sim_stalled(void);

#ifdef __cplusplus
}
#endif

#endif /* SIM_HARNESS_H */
