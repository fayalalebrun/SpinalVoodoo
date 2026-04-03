#ifndef TRACE_REPLAY_BACKEND_H
#define TRACE_REPLAY_BACKEND_H

#include <cstdint>
#include <cstring>
#include <vector>

extern "C" {
#include "voodoo_trace_format.h"
}

struct TraceReplayStateInfo {
    uint32_t fbiInit2 = 0;
    uint32_t bufferOffset = 0;
    uint32_t drawOffset = 0;
    uint32_t auxOffset = 0;
    uint32_t rowWidth = 0;
    uint32_t hDisp = 0;
    uint32_t vDisp = 0;
};

class TraceReplayBackend {
public:
    virtual ~TraceReplayBackend() = default;

    virtual bool useDe10RegisterMap() const = 0;
    virtual bool writeReg32(uint32_t mappedAddr, uint32_t rawAddr, uint32_t value) = 0;
    virtual bool writeReg16(uint32_t mappedAddr, uint32_t rawAddr, uint16_t value) = 0;
    virtual bool writeFb32(uint32_t byteOffset, uint32_t value) = 0;
    virtual bool writeFb16(uint32_t byteOffset, uint16_t value) = 0;
    virtual bool writeTex32(uint32_t byteOffset, uint32_t value) = 0;
    virtual bool writeFbBulk(uint32_t byteOffset, const uint8_t *data, uint32_t size) = 0;
    virtual bool writeTexBulk(uint32_t byteOffset, const uint8_t *data, uint32_t size) = 0;
    virtual bool readFbWords(uint32_t byteOffset, uint32_t *dst, uint32_t wordCount) = 0;
    virtual bool idleWait() = 0;
    virtual bool invalidateFbCache() = 0;
    virtual bool flushFbCache() = 0;
    virtual bool setSwapCount(uint32_t count) = 0;
};

static inline uint32_t traceReplayRemapBackendRegAddr(bool de10, uint32_t addr) {
    const uint32_t base = addr & 0x3FFFFFu;
    if (!de10 || (base & 0x200000u) == 0) return base;
    switch (base & 0xfffu) {
        case 0x088: return 0x088;
        case 0x08c: return 0x08c;
        case 0x090: return 0x090;
        case 0x094: return 0x094;
        case 0x098: return 0x098;
        case 0x09c: return 0x09c;
        case 0x0a0: return 0x0a0;
        case 0x0a4: return 0x0c0;
        case 0x0a8: return 0x0e0;
        case 0x0ac: return 0x0a4;
        case 0x0b0: return 0x0c4;
        case 0x0b4: return 0x0e4;
        case 0x0b8: return 0x0a8;
        case 0x0bc: return 0x0c8;
        case 0x0c0: return 0x0e8;
        case 0x0c4: return 0x0ac;
        case 0x0c8: return 0x0cc;
        case 0x0cc: return 0x0ec;
        case 0x0d0: return 0x0b0;
        case 0x0d4: return 0x0d0;
        case 0x0d8: return 0x0f0;
        case 0x0dc: return 0x0b4;
        case 0x0e0: return 0x0d4;
        case 0x0e4: return 0x0f4;
        case 0x0e8: return 0x0b8;
        case 0x0ec: return 0x0d8;
        case 0x0f0: return 0x0f8;
        case 0x0f4: return 0x0bc;
        case 0x0f8: return 0x0dc;
        case 0x0fc: return 0x0fc;
        default: return base & ~0x200000u;
    }
}

static inline bool traceReplayLoadState(TraceReplayBackend &backend,
                                        const uint8_t *stateData,
                                        uint32_t stateSize,
                                        bool skipFbState,
                                        TraceReplayStateInfo *info) {
    if (!stateData || stateSize < sizeof(voodoo_state_header_t)) return false;
    const auto *shdr = reinterpret_cast<const voodoo_state_header_t *>(stateData);
    if (shdr->magic != VOODOO_STATE_MAGIC || shdr->version != VOODOO_STATE_VERSION) return false;

    const uint8_t *ptr = stateData + sizeof(voodoo_state_header_t);
    const auto *regs = reinterpret_cast<const voodoo_state_reg_t *>(ptr);
    for (uint32_t i = 0; i < shdr->reg_count; i++) {
        uint32_t regAddr = regs[i].addr & 0x3fc;
        if (regAddr == 0x128 || regAddr == 0x124 || regAddr == 0x080 || regAddr == 0x100 || regAddr == 0x120)
            continue;
        uint32_t mapped = traceReplayRemapBackendRegAddr(backend.useDe10RegisterMap(), regs[i].addr);
        if (!backend.writeReg32(mapped, regs[i].addr, regs[i].value)) return false;
        if (regAddr == 0x218) info->fbiInit2 = regs[i].value;
    }
    ptr += shdr->reg_count * sizeof(voodoo_state_reg_t);

    info->drawOffset = shdr->draw_offset;
    info->auxOffset = shdr->aux_offset;
    info->rowWidth = shdr->row_width;
    info->hDisp = shdr->h_disp;
    info->vDisp = shdr->v_disp;

    if (!skipFbState && shdr->fb_size > 0) {
        if (!backend.writeFbBulk(0, ptr, shdr->fb_size)) return false;
    }
    ptr += shdr->fb_size;

    if (shdr->tex_size > 0) {
        if (!backend.writeTexBulk(0, ptr, shdr->tex_size)) return false;
    }

    info->bufferOffset = ((info->fbiInit2 >> 11) & 0x1FFu) * 4096u;
    if (info->bufferOffset > 0) {
        const uint32_t drawBuffer = info->drawOffset / info->bufferOffset;
        const uint32_t swapCount = 1u - (drawBuffer & 0x1u);
        if (!backend.setSwapCount(swapCount)) return false;
    }

    if (!backend.invalidateFbCache()) return false;
    if (!backend.idleWait()) return false;
    return true;
}

static inline bool traceReplayExecEntry(TraceReplayBackend &backend,
                                        const voodoo_trace_entry_t &entry,
                                        bool *sawSwap = nullptr) {
    const uint32_t repeat = entry.count ? entry.count : 1u;
    for (uint32_t rep = 0; rep < repeat; ++rep) {
        switch (entry.cmd_type) {
            case VOODOO_TRACE_WRITE_REG_L: {
                const uint32_t mapped = traceReplayRemapBackendRegAddr(backend.useDe10RegisterMap(), entry.addr);
                const uint32_t reg = mapped & 0x3fcu;
                if (reg == 0x128u && !backend.idleWait()) return false;
                if (!backend.writeReg32(mapped, entry.addr, entry.data)) return false;
                if (reg == 0x128u) {
                    if (!backend.idleWait()) return false;
                    if (sawSwap) *sawSwap = true;
                }
                break;
            }
            case VOODOO_TRACE_WRITE_REG_W:
                if (!backend.writeReg16(traceReplayRemapBackendRegAddr(backend.useDe10RegisterMap(), entry.addr),
                                        entry.addr, static_cast<uint16_t>(entry.data))) return false;
                break;
            case VOODOO_TRACE_WRITE_TEX_L:
                if (!backend.writeTex32(entry.addr & 0x7FFFFFu, entry.data)) return false;
                break;
            case VOODOO_TRACE_WRITE_FB_L:
                if (!backend.writeFb32(entry.addr & 0x3FFFFFu, entry.data)) return false;
                break;
            case VOODOO_TRACE_WRITE_FB_W:
                if (!backend.writeFb16(entry.addr & 0x3FFFFFu, static_cast<uint16_t>(entry.data))) return false;
                break;
            default:
                break;
        }
    }
    return true;
}

static inline bool traceReplayFinish(TraceReplayBackend &backend) {
    if (!backend.idleWait()) return false;
    if (!backend.flushFbCache()) return false;
    if (!backend.idleWait()) return false;
    return true;
}

static inline bool traceReplayCaptureRgb565(TraceReplayBackend &backend,
                                            uint32_t fbOffset,
                                            uint32_t pixelStride,
                                            uint32_t dispWidth,
                                            uint32_t dispHeight,
                                            std::vector<uint16_t> *out) {
    if (!out) return false;
    const uint32_t alignedOffset = fbOffset & ~0x3u;
    const uint32_t startHalfword = (fbOffset >> 1) & 0x1u;
    const uint32_t totalHalfwords = pixelStride * dispHeight + startHalfword;
    const uint32_t totalWords = (totalHalfwords + 1u) / 2u;
    std::vector<uint32_t> raw(totalWords);
    if (!backend.readFbWords(alignedOffset, raw.data(), totalWords)) return false;

    out->assign(pixelStride * dispHeight, 0);
    for (uint32_t y = 0; y < dispHeight; ++y) {
        for (uint32_t x = 0; x < dispWidth; ++x) {
            const uint32_t p = startHalfword + y * pixelStride + x;
            const uint32_t word = raw[p >> 1];
            (*out)[y * pixelStride + x] = (p & 1u) ? uint16_t((word >> 16) & 0xFFFFu)
                                                   : uint16_t(word & 0xFFFFu);
        }
    }
    return true;
}

#endif
