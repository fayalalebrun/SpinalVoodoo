/*
 * voodoo_trace_format.h — Shared trace format definitions.
 *
 * Structs and enums for the Voodoo binary trace format.
 * No dependencies on 86Box or Verilator — safe to include from
 * any C/C++ translation unit.
 */

#ifndef VOODOO_TRACE_FORMAT_H
#define VOODOO_TRACE_FORMAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Magic constants */
#define VOODOO_TRACE_MAGIC   0x564F4F44  /* "VOOD" */
#define VOODOO_STATE_MAGIC   0x41545356  /* "VSTA" */
#define VOODOO_STATE_VERSION 2

/* Trace command types */
typedef enum {
    VOODOO_TRACE_WRITE_REG_L  = 0,  /* 32-bit register write */
    VOODOO_TRACE_WRITE_REG_W  = 1,  /* 16-bit register write */
    VOODOO_TRACE_WRITE_FB_L   = 2,  /* 32-bit framebuffer write */
    VOODOO_TRACE_WRITE_FB_W   = 3,  /* 16-bit framebuffer write */
    VOODOO_TRACE_WRITE_TEX_L  = 4,  /* 32-bit texture write */
    VOODOO_TRACE_WRITE_CMDFIFO= 5,  /* Write to CMDFIFO region */
    VOODOO_TRACE_READ_REG_L   = 6,  /* 32-bit register read */
    VOODOO_TRACE_READ_REG_W   = 7,  /* 16-bit register read */
    VOODOO_TRACE_READ_FB_L    = 8,  /* 32-bit framebuffer read */
    VOODOO_TRACE_READ_FB_W    = 9,  /* 16-bit framebuffer read */
    VOODOO_TRACE_VSYNC        = 10, /* VSync event */
    VOODOO_TRACE_SWAP         = 11, /* Buffer swap */
    VOODOO_TRACE_CONFIG       = 12, /* Display config change */
    VOODOO_TRACE_FRAME_END    = 13, /* Frame end marker */
} voodoo_trace_cmd_t;

/* Trace file header (64 bytes) */
typedef struct {
    uint32_t magic;           /* 0x564F4F44 "VOOD" */
    uint32_t version;         /* Trace format version */
    uint32_t voodoo_type;     /* VOODOO_1, VOODOO_2, etc */
    uint32_t fb_size_mb;      /* Framebuffer size in MB */
    uint32_t tex_size_mb;     /* Texture size in MB (per TMU) */
    uint32_t num_tmus;        /* Number of TMUs (1 or 2) */
    uint32_t pci_base_addr;   /* PCI BAR base address */
    uint32_t flags;           /* Reserved flags */
    uint64_t cpu_speed_hz;    /* CPU speed in Hz */
    uint32_t pci_speed_hz;    /* PCI bus speed in Hz */
    uint32_t entry_count;     /* Total entries (for validation, 0=unknown) */
    uint32_t reserved[4];     /* Reserved for future use (16 bytes) */
} __attribute__((packed)) voodoo_trace_header_t;

/* Standard trace entry (24 bytes) */
typedef struct {
    uint32_t timestamp;       /* CPU cycles from start (first occurrence) */
    uint32_t timestamp_end;   /* CPU cycles for last occurrence (if count > 1) */
    uint8_t  cmd_type;        /* voodoo_trace_cmd_t */
    uint8_t  reserved[3];     /* Explicit padding for alignment */
    uint32_t addr;            /* Address within 16MB space */
    uint32_t data;            /* Value written/read */
    uint32_t count;           /* Number of times this command was repeated */
} __attribute__((packed)) voodoo_trace_entry_t;

/* Extended config entry (32 bytes) */
typedef struct {
    uint64_t timestamp;
    uint32_t cmd_type;        /* VOODOO_TRACE_CONFIG */
    uint32_t h_disp;
    uint32_t v_disp;
    uint32_t refresh_hz;
    uint32_t pixel_clock_hz;
    uint32_t reserved;        /* Reserved for future use (4 bytes) */
} __attribute__((packed)) voodoo_trace_config_t;

/* State dump file header (64 bytes) */
typedef struct {
    uint32_t magic;           /* VOODOO_STATE_MAGIC */
    uint32_t version;         /* Format version */
    uint32_t frame_num;       /* Frame number */
    uint32_t voodoo_type;     /* VOODOO_1, VOODOO_2, etc */
    uint32_t fb_size;         /* Framebuffer size in bytes */
    uint32_t tex_size;        /* Texture size per TMU in bytes */
    uint32_t num_tmus;        /* Number of TMUs (1 or 2) */
    uint32_t reg_count;       /* Number of register entries */
    uint32_t flags;           /* Reserved flags */
    /* Framebuffer layout info for conversion */
    uint32_t row_width;       /* Row stride in bytes */
    uint32_t draw_offset;     /* Color buffer offset in fb_mem */
    uint32_t aux_offset;      /* Depth buffer offset in fb_mem */
    uint32_t h_disp;          /* Horizontal display resolution */
    uint32_t v_disp;          /* Vertical display resolution */
    uint32_t reserved[2];     /* Padding to 64 bytes */
} __attribute__((packed)) voodoo_state_header_t;

/* State dump register entry (8 bytes) */
typedef struct {
    uint32_t addr;            /* Register address */
    uint32_t value;           /* Register value */
} __attribute__((packed)) voodoo_state_reg_t;

#ifdef __cplusplus
}
#endif

#endif /* VOODOO_TRACE_FORMAT_H */
