#ifndef TRACE_REPLAY_IO_H
#define TRACE_REPLAY_IO_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/stat.h>

extern "C" {
#include "voodoo_trace_format.h"
}

struct TraceReplayFiles {
    std::string inputPath;
    std::string traceFile;
    std::string stateFile;
};

struct TraceReplayLoadedTrace {
    uint8_t *traceData = nullptr;
    uint32_t traceSize = 0;
    const voodoo_trace_header_t *header = nullptr;
    const voodoo_trace_entry_t *entries = nullptr;
    uint32_t entryCount = 0;
};

static inline bool traceReplayIsDirectory(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static inline uint8_t *traceReplayReadFile(const char *path, uint32_t *outSize) {
    FILE *f = fopen(path, "rb");
    if (!f) return nullptr;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = static_cast<uint8_t *>(malloc(sz));
    if (!buf) {
        fclose(f);
        return nullptr;
    }
    if (static_cast<long>(fread(buf, 1, sz, f)) != sz) {
        free(buf);
        fclose(f);
        return nullptr;
    }
    fclose(f);
    *outSize = static_cast<uint32_t>(sz);
    return buf;
}

static inline bool traceReplayResolveFiles(const char *inputPath, TraceReplayFiles *out) {
    if (!inputPath || !out) return false;
    out->inputPath = inputPath;
    if (traceReplayIsDirectory(inputPath)) {
        out->traceFile = out->inputPath + "/trace.bin";
        out->stateFile = out->inputPath + "/state.bin";
        struct stat st;
        if (stat(out->stateFile.c_str(), &st) != 0) out->stateFile.clear();
    } else {
        out->traceFile = out->inputPath;
        std::string candidate = out->traceFile;
        size_t slash = candidate.find_last_of('/');
        candidate = (slash == std::string::npos)
            ? "state.bin"
            : candidate.substr(0, slash + 1) + "state.bin";
        struct stat st;
        if (stat(candidate.c_str(), &st) == 0) out->stateFile = candidate;
    }
    return true;
}

static inline bool traceReplayLoadTraceFile(const std::string &traceFile, TraceReplayLoadedTrace *out) {
    if (!out) return false;
    out->traceData = traceReplayReadFile(traceFile.c_str(), &out->traceSize);
    if (!out->traceData || out->traceSize < sizeof(voodoo_trace_header_t)) return false;
    out->header = reinterpret_cast<const voodoo_trace_header_t *>(out->traceData);
    if (out->header->magic != VOODOO_TRACE_MAGIC) return false;
    out->entryCount = (out->traceSize - sizeof(voodoo_trace_header_t)) / sizeof(voodoo_trace_entry_t);
    out->entries = reinterpret_cast<const voodoo_trace_entry_t *>(out->traceData + sizeof(voodoo_trace_header_t));
    return true;
}

#endif
