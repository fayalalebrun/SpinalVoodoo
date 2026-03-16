/*
 * voodoo_trace_writer.c — Standalone binary trace writer implementation.
 *
 * No 86Box or Verilator dependencies.
 */

#include "voodoo_trace_writer.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>

int
trace_writer_open(voodoo_trace_writer_t *w, const char *path,
                  const voodoo_trace_header_t *hdr)
{
    if (!w || !path || !hdr)
        return -1;

    memset(w, 0, sizeof(*w));
    w->fd = -1;

    w->file = fopen(path, "wb");
    if (!w->file)
        return -1;
    w->fd = fileno(w->file);

    /* Write header */
    if (fwrite(hdr, sizeof(voodoo_trace_header_t), 1, w->file) != 1) {
        fclose(w->file);
        w->file = NULL;
        w->fd = -1;
        return -1;
    }

    return 0;
}

/* Flush the pending coalesced entry to disk. */
void
trace_writer_flush(voodoo_trace_writer_t *w)
{
    if (!w || !w->has_pending || !w->file)
        return;

    fwrite(&w->pending, sizeof(voodoo_trace_entry_t), 1, w->file);
    w->entry_count++;
    w->has_pending = 0;
}

/* Core coalescing logic shared by both record variants. */
static void
trace_writer_record_impl(voodoo_trace_writer_t *w, uint32_t ts,
                         uint8_t cmd_type, uint32_t addr, uint32_t data)
{
    if (!w || !w->file)
        return;

    /* Coalesce: identical consecutive entries merge into one with count++ */
    if (w->has_pending &&
        w->pending.cmd_type == cmd_type &&
        w->pending.addr == addr &&
        w->pending.data == data) {
        w->pending.count++;
        w->pending.timestamp_end = ts;
        return;
    }

    /* Different command — flush pending and start new one */
    trace_writer_flush(w);

    memset(&w->pending, 0, sizeof(w->pending));
    w->pending.timestamp     = ts;
    w->pending.timestamp_end = ts;
    w->pending.cmd_type      = cmd_type;
    w->pending.addr          = addr;
    w->pending.data          = data;
    w->pending.count         = 1;
    w->has_pending           = 1;
}

void
trace_writer_record(voodoo_trace_writer_t *w, uint8_t cmd_type,
                    uint32_t addr, uint32_t data)
{
    if (!w)
        return;
    trace_writer_record_impl(w, w->seq++, cmd_type, addr, data);
}

void
trace_writer_record_ts(voodoo_trace_writer_t *w, uint32_t timestamp,
                       uint8_t cmd_type, uint32_t addr, uint32_t data)
{
    trace_writer_record_impl(w, timestamp, cmd_type, addr, data);
}

void
trace_writer_close(voodoo_trace_writer_t *w)
{
    if (!w)
        return;

    trace_writer_flush(w);

    if (w->file) {
        /* Update entry_count in header */
        fseek(w->file, offsetof(voodoo_trace_header_t, entry_count), SEEK_SET);
        uint32_t count32 = (uint32_t)w->entry_count;
        fwrite(&count32, sizeof(uint32_t), 1, w->file);
        fclose(w->file);
        w->file = NULL;
        w->fd = -1;
    }
}

void
trace_writer_finalize_signal_safe(voodoo_trace_writer_t *w)
{
    uint32_t count32;

    if (!w || w->fd < 0)
        return;

    count32 = (uint32_t)w->entry_count;
    (void)pwrite(w->fd, &count32, sizeof(count32), offsetof(voodoo_trace_header_t, entry_count));
    (void)fsync(w->fd);
    (void)close(w->fd);
    w->fd = -1;
    w->file = NULL;
    w->has_pending = 0;
}
