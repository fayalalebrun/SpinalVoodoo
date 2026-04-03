#ifndef TRACE_REPLAY_RUNNER_H
#define TRACE_REPLAY_RUNNER_H

#include <cstdint>

extern "C" {
#include "voodoo_trace_format.h"
}

#include "trace_replay_backend.h"

struct TraceReplayRunResult {
    uint32_t replayedEntries = 0;
    uint64_t logicalCommands = 0;
    uint32_t regWrites = 0;
    uint32_t texWrites = 0;
    uint32_t fbWrites = 0;
    uint32_t swapWrites = 0;
    uint32_t presentedOffset = 0;
    uint32_t currentDrawOffset = 0;
    bool havePresentedOffset = false;
};

template <typename OnEntry>
static inline bool traceReplayRunEntries(TraceReplayBackend &backend,
                                         const voodoo_trace_entry_t *entries,
                                         uint32_t entryCount,
                                         uint32_t initialDrawOffset,
                                         uint32_t bufferOffset,
                                         TraceReplayRunResult *out,
                                         OnEntry onEntry) {
    if (!entries || !out) return false;
    out->presentedOffset = initialDrawOffset;
    out->currentDrawOffset = initialDrawOffset;
    for (uint32_t i = 0; i < entryCount; ++i) {
        const auto &entry = entries[i];
        const uint32_t repeat = entry.count ? entry.count : 1u;
        out->logicalCommands += repeat;
        if (!onEntry(i, entry, repeat, *out, false)) {
            out->replayedEntries = i + 1;
            continue;
        }
        bool sawSwap = false;
        if (!traceReplayExecEntry(backend, entry, &sawSwap)) return false;
        if (sawSwap) {
            out->presentedOffset = out->currentDrawOffset;
            out->havePresentedOffset = true;
            if (bufferOffset > 0) {
                out->currentDrawOffset = (out->currentDrawOffset == 0) ? bufferOffset : 0;
            }
        }
        switch (entry.cmd_type) {
            case VOODOO_TRACE_WRITE_REG_L:
            case VOODOO_TRACE_WRITE_REG_W:
                out->regWrites += repeat;
                break;
            case VOODOO_TRACE_WRITE_TEX_L:
                out->texWrites += repeat;
                break;
            case VOODOO_TRACE_WRITE_FB_L:
            case VOODOO_TRACE_WRITE_FB_W:
                out->fbWrites += repeat;
                break;
            default:
                break;
        }
        if (sawSwap) out->swapWrites += repeat;
        out->replayedEntries = i + 1;
        onEntry(i, entry, repeat, *out, sawSwap);
    }
    return true;
}

static inline bool traceReplayRunEntries(TraceReplayBackend &backend,
                                         const voodoo_trace_entry_t *entries,
                                         uint32_t entryCount,
                                         uint32_t initialDrawOffset,
                                 uint32_t bufferOffset,
                                 TraceReplayRunResult *out) {
    return traceReplayRunEntries(backend, entries, entryCount, initialDrawOffset, bufferOffset, out,
                                 [](uint32_t, const voodoo_trace_entry_t &, uint32_t,
                                    const TraceReplayRunResult &, bool) { return true; });
}

#endif
