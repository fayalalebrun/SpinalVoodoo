/*
 * voodoo_trace_writer.h — Standalone binary trace writer.
 *
 * No 86Box or Verilator dependencies. Writes the trace format defined
 * in voodoo_trace_format.h with entry coalescing for identical
 * consecutive commands.
 */

#ifndef VOODOO_TRACE_WRITER_H
#define VOODOO_TRACE_WRITER_H

#include <stdio.h>
#include "voodoo_trace_format.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    FILE     *file;
    uint64_t  entry_count;
    uint32_t  seq;               /* monotonic timestamp counter */
    int       has_pending;
    voodoo_trace_entry_t pending; /* for coalescing */
} voodoo_trace_writer_t;

/* Open a trace file and write the header.
 * Returns 0 on success, -1 on failure. */
int  trace_writer_open(voodoo_trace_writer_t *w, const char *path,
                       const voodoo_trace_header_t *hdr);

/* Record a trace entry with auto-incrementing timestamp.
 * Identical consecutive entries are coalesced. */
void trace_writer_record(voodoo_trace_writer_t *w, uint8_t cmd_type,
                         uint32_t addr, uint32_t data);

/* Record a trace entry with an explicit timestamp.
 * Use this when the caller provides real cycle counts (e.g. 86Box's tsc). */
void trace_writer_record_ts(voodoo_trace_writer_t *w, uint32_t timestamp,
                            uint8_t cmd_type, uint32_t addr, uint32_t data);

/* Flush any pending coalesced entry to disk. */
void trace_writer_flush(voodoo_trace_writer_t *w);

/* Flush, update entry count in header, and close the file. */
void trace_writer_close(voodoo_trace_writer_t *w);

#ifdef __cplusplus
}
#endif

#endif /* VOODOO_TRACE_WRITER_H */
