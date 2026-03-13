#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "glide.h"

static int parse_lod(unsigned value, GrLOD_t *lod, unsigned *size) {
  switch (value) {
    case 256: *lod = GR_LOD_256; *size = 256; return 1;
    case 128: *lod = GR_LOD_128; *size = 128; return 1;
    case 64:  *lod = GR_LOD_64;  *size = 64;  return 1;
    case 32:  *lod = GR_LOD_32;  *size = 32;  return 1;
    case 16:  *lod = GR_LOD_16;  *size = 16;  return 1;
    case 8:   *lod = GR_LOD_8;   *size = 8;   return 1;
    case 4:   *lod = GR_LOD_4;   *size = 4;   return 1;
    case 2:   *lod = GR_LOD_2;   *size = 2;   return 1;
    case 1:   *lod = GR_LOD_1;   *size = 1;   return 1;
    default: return 0;
  }
}

static int parse_aspect(unsigned w, unsigned h, GrAspectRatio_t *aspect) {
  if (w == 8 && h == 1) { *aspect = GR_ASPECT_8x1; return 1; }
  if (w == 4 && h == 1) { *aspect = GR_ASPECT_4x1; return 1; }
  if (w == 2 && h == 1) { *aspect = GR_ASPECT_2x1; return 1; }
  if (w == 1 && h == 1) { *aspect = GR_ASPECT_1x1; return 1; }
  if (w == 1 && h == 2) { *aspect = GR_ASPECT_1x2; return 1; }
  if (w == 1 && h == 4) { *aspect = GR_ASPECT_1x4; return 1; }
  if (w == 1 && h == 8) { *aspect = GR_ASPECT_1x8; return 1; }
  return 0;
}

static int parse_format(const char *fmt, GrTextureFormat_t *format, unsigned *bpp, unsigned *table_bytes) {
  *table_bytes = 0;
  if (!strcmp(fmt, "rgb332")) { *format = GR_TEXFMT_RGB_332; *bpp = 1; return 1; }
  if (!strcmp(fmt, "rgb565")) { *format = GR_TEXFMT_RGB_565; *bpp = 2; return 1; }
  if (!strcmp(fmt, "argb4444")) { *format = GR_TEXFMT_ARGB_4444; *bpp = 2; return 1; }
  if (!strcmp(fmt, "argb1555")) { *format = GR_TEXFMT_ARGB_1555; *bpp = 2; return 1; }
  if (!strcmp(fmt, "p8")) { *format = GR_TEXFMT_P_8; *bpp = 1; *table_bytes = 1024; return 1; }
  if (!strcmp(fmt, "ap88")) { *format = GR_TEXFMT_AP_88; *bpp = 2; *table_bytes = 1024; return 1; }
  if (!strcmp(fmt, "yiq")) { *format = GR_TEXFMT_YIQ_422; *bpp = 1; *table_bytes = 80; return 1; }
  if (!strcmp(fmt, "ayiq8422")) { *format = GR_TEXFMT_AYIQ_8422; *bpp = 2; *table_bytes = 80; return 1; }
  return 0;
}

static void lod_dims(unsigned base, GrAspectRatio_t aspect, unsigned *w, unsigned *h) {
  switch (aspect) {
    case GR_ASPECT_8x1: *w = base; *h = base / 8; break;
    case GR_ASPECT_4x1: *w = base; *h = base / 4; break;
    case GR_ASPECT_2x1: *w = base; *h = base / 2; break;
    case GR_ASPECT_1x1: *w = base; *h = base; break;
    case GR_ASPECT_1x2: *w = base / 2; *h = base; break;
    case GR_ASPECT_1x4: *w = base / 4; *h = base; break;
    case GR_ASPECT_1x8: *w = base / 8; *h = base; break;
    default: *w = base; *h = base; break;
  }
}

static unsigned compute_mem_required(unsigned large_size, unsigned small_size, GrAspectRatio_t aspect, unsigned bpp) {
  unsigned total = 0;
  unsigned size;
  for (size = large_size; size >= small_size; size >>= 1) {
    unsigned w, h;
    lod_dims(size, aspect, &w, &h);
    total += w * h * bpp;
    if (size == small_size) break;
  }
  return total;
}

static int read_nonempty_line_fd(int fd, char *buf, size_t size) {
  size_t pos = 0;
  char ch;
  do {
    pos = 0;
    while (pos + 1 < size) {
      int got = read(fd, &ch, 1);
      if (got != 1) return 0;
      buf[pos++] = ch;
      if (ch == '\n') break;
    }
    buf[pos] = '\0';
  } while (buf[0] == '\n' || buf[0] == '\r');
  return 1;
}

static int read_fully_fd(int fd, void *dst, size_t bytes) {
  unsigned char *p = (unsigned char *)dst;
  unsigned char bounce[2048];
  while (bytes) {
    size_t chunk = bytes > sizeof(bounce) ? sizeof(bounce) : bytes;
    int got = read(fd, bounce, chunk);
    if (got != (int)chunk) return 0;
    memcpy(p, bounce, chunk);
    p += chunk;
    bytes -= chunk;
  }
  return 1;
}

FxBool FX_CALL gu3dfGetInfo(const char *filename, Gu3dfInfo *info) {
  int fd;
  char line[128];
  char fmt[32];
  unsigned small, large, aspect_w, aspect_h;
  unsigned large_size, small_size, bpp, table_bytes;

  memset(info, 0, sizeof(*info));
  fd = open(filename, O_RDONLY | O_BINARY);
  if (fd < 0) return FXFALSE;

  if (!read_nonempty_line_fd(fd, line, sizeof(line)) || strncmp(line, "3df", 3) != 0) goto fail;
  if (!read_nonempty_line_fd(fd, line, sizeof(line))) goto fail;
  if (sscanf(line, "%31s", fmt) != 1) goto fail;
  if (!parse_format(fmt, &info->header.format, &bpp, &table_bytes)) goto fail;
  if (!read_nonempty_line_fd(fd, line, sizeof(line))) goto fail;
  if (sscanf(line, "lod range: %u %u", &small, &large) != 2) goto fail;
  if (!parse_lod(small, &info->header.small_lod, &small_size)) goto fail;
  if (!parse_lod(large, &info->header.large_lod, &large_size)) goto fail;
  if (!read_nonempty_line_fd(fd, line, sizeof(line))) goto fail;
  if (sscanf(line, "aspect ratio: %u %u", &aspect_w, &aspect_h) != 2) goto fail;
  if (!parse_aspect(aspect_w, aspect_h, &info->header.aspect_ratio)) goto fail;

  info->mem_required = compute_mem_required(large_size, small_size, info->header.aspect_ratio, bpp);
  close(fd);
  return FXTRUE;

fail:
  close(fd);
  return FXFALSE;
}

FxBool FX_CALL gu3dfLoad(const char *filename, Gu3dfInfo *info) {
  int fd;
  char line[128];
  unsigned table_bytes = 0;
  unsigned dummy_bpp = 0;
  void *saved_data = info->data;
  if (!gu3dfGetInfo(filename, info)) return FXFALSE;
  info->data = saved_data;
  fd = open(filename, O_RDONLY | O_BINARY);
  if (fd < 0) {
      return FXFALSE;
  }

  if (!read_nonempty_line_fd(fd, line, sizeof(line))) goto fail;
  if (!read_nonempty_line_fd(fd, line, sizeof(line))) goto fail;
  if (!parse_format(strtok(line, "\r\n"), &info->header.format, &dummy_bpp, &table_bytes)) goto fail;
  if (!read_nonempty_line_fd(fd, line, sizeof(line))) goto fail;
  if (!read_nonempty_line_fd(fd, line, sizeof(line))) goto fail;

  if (table_bytes) {
    if (!read_fully_fd(fd, &info->table, table_bytes)) goto fail;
  }
  if (!read_fully_fd(fd, info->data, info->mem_required)) goto fail;
  close(fd);
  return FXTRUE;

fail:
  close(fd);
  return FXFALSE;
}
