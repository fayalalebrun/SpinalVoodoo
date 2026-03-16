#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <unistd.h>
#endif
#ifdef DE10_BACKEND
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#endif
#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#endif

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#ifdef SIM_BACKEND
#include <fxsim.h>
#include <sim_harness.h>
#endif
#include "fxglide.h"

#if defined(__linux__)
static struct {
  Display *display;
  Window parent;
  Window window;
  GC gc;
  XImage *image;
  unsigned char *pixels;
  FxU16 *frontBuffer;
  FxU32 lastBufferOffset;
#ifdef DE10_BACKEND
  int ddrFd;
  void *ddrRaw;
  uintptr_t ddrBase;
  size_t ddrLength;
#endif
  Visual *visual;
  Colormap colormap;
  unsigned long redMask;
  unsigned long greenMask;
  unsigned long blueMask;
  unsigned long redMax;
  unsigned long greenMax;
  unsigned long blueMax;
  int redShift;
  int greenShift;
  int blueShift;
  int depth;
  int width;
  int height;
  int windowWidth;
  int windowHeight;
  int visible;
  int enabled;
  int presenting;
} x11Presenter = {
#ifdef DE10_BACKEND
  .ddrFd = -1,
#endif
};

#ifdef DE10_BACKEND
#define DE10_PRESENTER_DDR_BASE 0x3f000000u
#define DE10_PRESENTER_DDR_SPAN 0x01000000u
#endif

static int x11PresenterDebugEnabled(void) {
  static int enabled = -1;
  if (enabled < 0) enabled = (getenv("GLIDE_X11_DEBUG") != NULL);
  return enabled;
}

static int x11PresenterRequested(void) {
  const char *value = getenv("GLIDE_X11_ENABLE");
  if (value != NULL) return (*value != '\0' && strcmp(value, "0") != 0);
#ifndef DE10_BACKEND
  return 1;
#else
  return 0;
#endif
}

static int x11PresenterUseTopLevel(void) {
  const char *value = getenv("GLIDE_X11_TOPLEVEL");
  return value != NULL && *value != '\0' && strcmp(value, "0") != 0;
}

static void x11PresenterLog(const char *message) {
  const char *path = getenv("GLIDE_X11_STAGE_FILE");
  if (!x11PresenterDebugEnabled()) return;
  fprintf(stderr, "[gx11present] %s\n", message);
  fflush(stderr);
  if (path && *path) {
    FILE *fp = fopen(path, "a");
    if (fp) {
      fprintf(fp, "%s\n", message);
      fflush(fp);
#if defined(__linux__)
      fsync(fileno(fp));
#endif
      fclose(fp);
    }
  }
}

static void x11PresenterDumpFrame(const char *label) {
  const char *dir = getenv("GLIDE_X11_DUMP_DIR");
  FILE *fp;
  char path[512];
  size_t x;
  size_t y;
  static unsigned int frameId;

  if (!dir || !*dir || !x11Presenter.frontBuffer) return;

  snprintf(path, sizeof(path), "%s/%04u-%s.ppm", dir, frameId++, label ? label : "frame");
  fp = fopen(path, "wb");
  if (!fp) return;

  fprintf(fp, "P6\n%d %d\n255\n", x11Presenter.width, x11Presenter.height);
  for (y = 0; y < (size_t)x11Presenter.height; y++) {
    for (x = 0; x < (size_t)x11Presenter.width; x++) {
      FxU16 px = x11Presenter.frontBuffer[y * (size_t)x11Presenter.width + x];
      unsigned char rgb[3];
      rgb[0] = (unsigned char)(((px >> 11) & 0x1f) * 255 / 31);
      rgb[1] = (unsigned char)(((px >> 5) & 0x3f) * 255 / 63);
      rgb[2] = (unsigned char)((px & 0x1f) * 255 / 31);
      fwrite(rgb, 1, 3, fp);
    }
  }

  fclose(fp);
}

#ifdef DE10_BACKEND
static FxU32 x11PresenterParseEnvU32(const char *name, FxU32 fallback) {
  const char *value = getenv(name);
  char *end = NULL;
  unsigned long parsed;

  if (!value || !*value) return fallback;
  errno = 0;
  parsed = strtoul(value, &end, 0);
  if (errno != 0 || end == value || *end != '\0') return fallback;
  return (FxU32)parsed;
}

static int x11PresenterEnsureDe10DdrMap(void) {
  FxU32 physBase;
  FxU32 physSpan;
  long pageSize;
  uintptr_t alignedBase;
  size_t pageOffset;
  size_t mapLength;

  if (x11Presenter.ddrRaw && x11Presenter.ddrRaw != MAP_FAILED) return 1;
  x11PresenterLog("de10 ddr map begin");

  physBase = x11PresenterParseEnvU32("DE10_VOODOO_DDR_BASE", DE10_PRESENTER_DDR_BASE);
  physSpan = x11PresenterParseEnvU32("DE10_VOODOO_DDR_SPAN", DE10_PRESENTER_DDR_SPAN);
  pageSize = sysconf(_SC_PAGESIZE);
  if (pageSize <= 0) return 0;

  x11Presenter.ddrFd = open("/dev/mem", O_RDWR | O_SYNC);
  if (x11Presenter.ddrFd < 0) return 0;

  alignedBase = physBase & ~((uintptr_t)pageSize - 1u);
  pageOffset = (size_t)(physBase - alignedBase);
  mapLength = pageOffset + physSpan;
  x11Presenter.ddrRaw = mmap(NULL,
                             mapLength,
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED,
                             x11Presenter.ddrFd,
                             (off_t)alignedBase);
  if (x11Presenter.ddrRaw == MAP_FAILED) {
    close(x11Presenter.ddrFd);
    x11Presenter.ddrFd = -1;
    x11Presenter.ddrRaw = NULL;
    return 0;
  }

  x11Presenter.ddrBase = (uintptr_t)x11Presenter.ddrRaw + pageOffset;
  x11Presenter.ddrLength = mapLength;
  x11PresenterLog("de10 ddr map ready");
  return 1;
}
#endif

static int x11PresenterMaskShift(unsigned long mask) {
  int shift = 0;
  if (!mask) return 0;
  while ((mask & 1ul) == 0ul) {
    mask >>= 1;
    shift++;
  }
  return shift;
}

static unsigned long x11PresenterScaleComponent(unsigned char value,
                                                unsigned long maxValue,
                                                int shift) {
  unsigned long scaled = 0;
  if (maxValue) {
    scaled = ((unsigned long)value * maxValue + 127ul) / 255ul;
  }
  return scaled << shift;
}

static void x11PresenterDestroyWindow(void) {
  if (x11Presenter.window && x11Presenter.display) {
    XDestroyWindow(x11Presenter.display, x11Presenter.window);
  }
  x11Presenter.window = 0;
}

static void x11PresenterFreeImage(void) {
  if (x11Presenter.image) {
    x11Presenter.image->data = (char *)x11Presenter.pixels;
    XDestroyImage(x11Presenter.image);
    x11Presenter.image = NULL;
    x11Presenter.pixels = NULL;
  }
}

static int x11PresenterCreateImage(void) {
  XImage *image;
  int imageWidth;
  int imageHeight;

  imageWidth = x11Presenter.windowWidth > 0 ? x11Presenter.windowWidth : x11Presenter.width;
  imageHeight = x11Presenter.windowHeight > 0 ? x11Presenter.windowHeight : x11Presenter.height;
  if (imageWidth <= 0 || imageHeight <= 0) return 0;

  x11PresenterFreeImage();
  image = XCreateImage(x11Presenter.display,
                       x11Presenter.visual,
                       (unsigned int)x11Presenter.depth,
                       ZPixmap,
                       0,
                       NULL,
                       imageWidth,
                       imageHeight,
                       BitmapPad(x11Presenter.display),
                       0);
  if (!image) return 0;

  x11Presenter.pixels = (unsigned char *)malloc((size_t)image->bytes_per_line *
                                                (size_t)imageHeight);
  if (!x11Presenter.pixels) {
    image->data = NULL;
    XDestroyImage(image);
    return 0;
  }

  image->data = (char *)x11Presenter.pixels;
  x11Presenter.image = image;
  return 1;
}

static int x11PresenterEnsureChildWindow(void) {
  Region inputRegion;
  XSetWindowAttributes windowAttr;
  Window hostParent;
  unsigned long windowMask = 0;
  int parentWidth;
  int parentHeight;
  int childX;
  int childY;
  unsigned int childWidth;
  unsigned int childHeight;

  hostParent = x11Presenter.parent;
  if (x11PresenterUseTopLevel()) {
    int screen = DefaultScreen(x11Presenter.display);
    hostParent = RootWindow(x11Presenter.display, screen);
    parentWidth = DisplayWidth(x11Presenter.display, screen);
    parentHeight = DisplayHeight(x11Presenter.display, screen);
  } else {
    XWindowAttributes parentAttr;
    if (!XGetWindowAttributes(x11Presenter.display, x11Presenter.parent, &parentAttr)) {
      return 0;
    }
    parentWidth = parentAttr.width;
    parentHeight = parentAttr.height;
  }

  childWidth = (unsigned int)x11Presenter.width;
  childHeight = (unsigned int)x11Presenter.height;
  if ((unsigned int)parentWidth != childWidth || (unsigned int)parentHeight != childHeight) {
    unsigned long scaledWidth = (unsigned long)parentHeight * (unsigned long)x11Presenter.width;
    unsigned long scaledHeight = (unsigned long)parentWidth * (unsigned long)x11Presenter.height;

    if (scaledWidth <= scaledHeight) {
      childWidth = (unsigned int)parentWidth;
      childHeight = (unsigned int)(((unsigned long)parentWidth * (unsigned long)x11Presenter.height) /
                                   (unsigned long)x11Presenter.width);
    } else {
      childHeight = (unsigned int)parentHeight;
      childWidth = (unsigned int)(((unsigned long)parentHeight * (unsigned long)x11Presenter.width) /
                                  (unsigned long)x11Presenter.height);
    }
  }
  if ((unsigned int)parentWidth < childWidth) childWidth = (unsigned int)parentWidth;
  if ((unsigned int)parentHeight < childHeight) childHeight = (unsigned int)parentHeight;
  if (!childWidth || !childHeight) return 0;

  childX = 0;
  childY = 0;
  if (parentWidth > (int)childWidth) childX = (parentWidth - (int)childWidth) / 2;
  if (parentHeight > (int)childHeight) childY = (parentHeight - (int)childHeight) / 2;

  if (!x11Presenter.window) {
    memset(&windowAttr, 0, sizeof(windowAttr));
    windowAttr.background_pixmap = None;
    windowMask |= CWBackPixmap;
    windowAttr.border_pixel = 0;
    windowMask |= CWBorderPixel;
    if (x11PresenterUseTopLevel()) {
      windowAttr.override_redirect = True;
      windowMask |= CWOverrideRedirect;
    }
    if (x11Presenter.colormap) {
      windowAttr.colormap = x11Presenter.colormap;
      windowMask |= CWColormap;
    }

    x11Presenter.window = XCreateWindow(x11Presenter.display,
                                        hostParent,
                                        childX,
                                        childY,
                                        childWidth,
                                        childHeight,
                                        0,
                                        x11Presenter.depth,
                                        InputOutput,
                                        x11Presenter.visual,
                                        windowMask,
                                        &windowAttr);
    if (!x11Presenter.window) return 0;
    if (x11PresenterUseTopLevel()) {
      XStoreName(x11Presenter.display, x11Presenter.window, "Glide X11 Presenter");
    }
    x11PresenterLog("created child window");
  } else if (x11Presenter.windowWidth != (int)childWidth ||
             x11Presenter.windowHeight != (int)childHeight) {
    XResizeWindow(x11Presenter.display,
                  x11Presenter.window,
                  childWidth,
                  childHeight);
  }

  XMoveWindow(x11Presenter.display, x11Presenter.window, childX, childY);
  inputRegion = XCreateRegion();
  if (inputRegion) {
    XShapeCombineRegion(x11Presenter.display,
                        x11Presenter.window,
                        ShapeInput,
                        0,
                        0,
                        inputRegion,
                        ShapeSet);
    XDestroyRegion(inputRegion);
  }

  x11Presenter.windowWidth = (int)childWidth;
  x11Presenter.windowHeight = (int)childHeight;
  if (!x11Presenter.image || x11Presenter.image->width != x11Presenter.windowWidth ||
      x11Presenter.image->height != x11Presenter.windowHeight) {
    if (!x11PresenterCreateImage()) return 0;
  }
  return x11Presenter.windowWidth > 0 && x11Presenter.windowHeight > 0;
}

static void x11PresenterSetVisible(int visible) {
  if (!x11Presenter.window || !x11Presenter.display) return;
  if (!!x11Presenter.visible == !!visible) return;

  if (visible) {
    XMapRaised(x11Presenter.display, x11Presenter.window);
    if (!x11PresenterUseTopLevel()) {
      XSetInputFocus(x11Presenter.display,
                     x11Presenter.parent,
                     RevertToParent,
                     CurrentTime);
    }
  } else {
    XUnmapWindow(x11Presenter.display, x11Presenter.window);
  }

  x11Presenter.visible = !!visible;
}

static size_t x11PresenterCountNonBlackPixels(void) {
  size_t count = (size_t)x11Presenter.width * (size_t)x11Presenter.height;
  size_t i;
  size_t nonBlack = 0;

  for (i = 0; i < count; i++) {
    if (x11Presenter.frontBuffer[i] != 0) nonBlack++;
  }

  return nonBlack;
}

static int x11PresenterReadFrontBuffer(void) {
  GrGC *gc = _GlideRoot.curGC;
#ifdef SIM_BACKEND
  FxBool readOk;
#elif defined(DE10_BACKEND)
  Sstregs *hw;
  FxU32 stridePixels;
  FxU32 bufferOffset;
  FxU32 displayedBuffer;
  FxU32 displayOffset;
  FxU16 *ddrBase;
  FxU32 y;
#else
  Sstregs *hw;
  FxU32 savedLfbMode;
  FxU32 readLfbMode;
  FxU32 scanline;
#endif

  if (!gc) return 0;
#ifdef SIM_BACKEND
  readOk = grLfbReadRegion(GR_BUFFER_FRONTBUFFER,
                           0,
                           0,
                           (FxU32)x11Presenter.width,
                           (FxU32)x11Presenter.height,
                           (FxU32)x11Presenter.width * sizeof(FxU16),
                           x11Presenter.frontBuffer);
  if (!readOk) return 0;
  x11Presenter.lastBufferOffset = 0u;
  x11PresenterLog("frontbuffer lfb read ok");
  return 1;
#elif defined(DE10_BACKEND)
  hw = (Sstregs *)gc->reg_ptr;
  if (!hw) return 0;
  if (!x11PresenterEnsureDe10DdrMap()) return 0;
  x11PresenterLog("de10 read begin");

  stridePixels = gc->fbStride;
  {
    FxU32 fbiInit1 = GET(hw->fbiInit1);
    FxU32 tiles = (fbiInit1 >> 4) & 0xFu;
    if (tiles) stridePixels = tiles << 6;
  }
  if (!stridePixels) return 0;
  x11PresenterLog("de10 stride ready");

  bufferOffset = ((GET(hw->fbiInit2) >> 11) & 0x1FFu) * 4096u;
  if (!bufferOffset) bufferOffset = stridePixels * (FxU32)x11Presenter.height * 2u;
  displayedBuffer = (GET(hw->status) & SST_DISPLAYED_BUFFER) >> SST_DISPLAYED_BUFFER_SHIFT;
  displayOffset = (displayedBuffer & 1u) * bufferOffset;
  ddrBase = (FxU16 *)(x11Presenter.ddrBase + displayOffset);
  x11PresenterLog("de10 buffer select ready");

  for (y = 0; y < (FxU32)x11Presenter.height; y++) {
    memcpy(x11Presenter.frontBuffer + (size_t)y * (size_t)x11Presenter.width,
           ddrBase + (size_t)y * (size_t)stridePixels,
           (size_t)x11Presenter.width * sizeof(FxU16));
    if (y == 0) x11PresenterLog("de10 first scanline copied");
  }

  x11Presenter.lastBufferOffset = displayOffset;
  x11PresenterLog("frontbuffer de10 ddr read ok");
  return 1;
#else
  hw = (Sstregs *)gc->reg_ptr;
  if (!hw || !gc->lfb_ptr) return 0;

  savedLfbMode = gc->state.fbi_config.lfbMode;
  readLfbMode = savedLfbMode & ~(SST_LFB_READBUFSELECT | SST_LFB_YORIGIN);
  readLfbMode |= SST_LFB_READFRONTBUFFER;

  x11PresenterLog("frontbuffer set lfbMode");
  SET(hw->lfbMode, readLfbMode);
  gc->state.fbi_config.lfbMode = readLfbMode;
  x11PresenterLog("frontbuffer lfbMode ready");

  for (scanline = 0; scanline < (FxU32)x11Presenter.height; scanline++) {
    FxU16 *dstLine = x11Presenter.frontBuffer +
                     (size_t)scanline * (size_t)x11Presenter.width;
    FxU32 x;

    for (x = 0; x < (FxU32)x11Presenter.width; x += 2) {
      volatile void *srcAddr = ((char *)gc->lfb_ptr) +
                               ((size_t)scanline * (size_t)gc->fbStride * 2u) +
                               ((size_t)x << 1);
#ifdef SIM_BACKEND
      FxU32 pair = simReadReg(srcAddr);
#else
      FxU32 pair = *(volatile FxU32 *)srcAddr;
#endif
      if (scanline == 0 && x == 0) x11PresenterLog("frontbuffer first read ok");

      dstLine[x] = (FxU16)(pair & 0xFFFFu);
      if ((x + 1u) < (FxU32)x11Presenter.width) {
        dstLine[x + 1u] = (FxU16)(pair >> 16);
      }
    }
  }

  SET(hw->lfbMode, savedLfbMode);
  gc->state.fbi_config.lfbMode = savedLfbMode;
  x11PresenterLog("frontbuffer restore lfbMode");
  return 1;
#endif
}

void _grX11PresenterShutdown(void) {
  x11PresenterLog("shutdown");
  x11Presenter.enabled = 0;
  x11Presenter.presenting = 0;
  if (x11Presenter.gc && x11Presenter.display) {
    XFreeGC(x11Presenter.display, x11Presenter.gc);
  }
  x11Presenter.gc = 0;
  x11PresenterDestroyWindow();
  x11PresenterFreeImage();
  free(x11Presenter.frontBuffer);
  x11Presenter.frontBuffer = NULL;
#ifdef DE10_BACKEND
  if (x11Presenter.ddrRaw && x11Presenter.ddrRaw != MAP_FAILED) {
    munmap(x11Presenter.ddrRaw, x11Presenter.ddrLength);
  }
  x11Presenter.ddrRaw = NULL;
  x11Presenter.ddrBase = 0;
  x11Presenter.ddrLength = 0;
  if (x11Presenter.ddrFd >= 0) close(x11Presenter.ddrFd);
  x11Presenter.ddrFd = -1;
#endif
  if (x11Presenter.display) {
    XCloseDisplay(x11Presenter.display);
  }
  memset(&x11Presenter, 0, sizeof(x11Presenter));
#ifdef DE10_BACKEND
  x11Presenter.ddrFd = -1;
#endif
}

void _grX11PresenterInit(FxU32 hWnd, FxU32 width, FxU32 height) {
  XWindowAttributes attr;
  if (!x11PresenterRequested()) return;
  _grX11PresenterShutdown();
  if (!hWnd || !width || !height) return;
  x11PresenterLog("init begin");

  x11Presenter.display = XOpenDisplay(NULL);
  if (!x11Presenter.display) return;

  x11Presenter.parent = (Window)hWnd;
  if (!XGetWindowAttributes(x11Presenter.display, x11Presenter.parent, &attr)) {
    _grX11PresenterShutdown();
    return;
  }

  x11Presenter.width = (int)width;
  x11Presenter.height = (int)height;
  x11Presenter.visual = attr.visual;
  x11Presenter.colormap = attr.colormap;
  x11Presenter.depth = attr.depth;
  x11Presenter.redMask = attr.visual ? attr.visual->red_mask : 0ul;
  x11Presenter.greenMask = attr.visual ? attr.visual->green_mask : 0ul;
  x11Presenter.blueMask = attr.visual ? attr.visual->blue_mask : 0ul;
  x11Presenter.redShift = x11PresenterMaskShift(x11Presenter.redMask);
  x11Presenter.greenShift = x11PresenterMaskShift(x11Presenter.greenMask);
  x11Presenter.blueShift = x11PresenterMaskShift(x11Presenter.blueMask);
  x11Presenter.redMax = x11Presenter.redMask >> x11Presenter.redShift;
  x11Presenter.greenMax = x11Presenter.greenMask >> x11Presenter.greenShift;
  x11Presenter.blueMax = x11Presenter.blueMask >> x11Presenter.blueShift;

  x11Presenter.frontBuffer = (FxU16 *)malloc((size_t)width * (size_t)height * sizeof(FxU16));
  if (!x11Presenter.frontBuffer) {
    _grX11PresenterShutdown();
    return;
  }

  if (!x11PresenterCreateImage()) {
    _grX11PresenterShutdown();
    return;
  }

  x11Presenter.enabled = 1;
  x11PresenterLog("init ready");
}

void _grX11PresenterSwap(void) {
  size_t x;
  size_t y;
  size_t nonBlackPixels;
  int waitIters;

  if (!x11Presenter.enabled || x11Presenter.presenting) return;
  x11Presenter.presenting = 1;
  x11PresenterLog("swap begin");

  if (!x11PresenterReadFrontBuffer()) {
    x11PresenterLog("frontbuffer read failed");
    goto out;
  }

  for (waitIters = 0; waitIters < 4096; waitIters++) {
    if (grBufferNumPending() <= 0 && !grSstIsBusy()) break;
  }
  if (waitIters > 0) {
    if (!x11PresenterReadFrontBuffer()) {
      x11PresenterLog("frontbuffer reread failed");
      goto out;
    }
  }

  nonBlackPixels = x11PresenterCountNonBlackPixels();
  if (x11PresenterDebugEnabled()) {
    char msg[160];
    snprintf(msg,
             sizeof(msg),
             "frontbuffer final nonblack=%lu offset=0x%x waited=%d",
             (unsigned long)nonBlackPixels,
             x11Presenter.lastBufferOffset,
             waitIters);
    x11PresenterLog(msg);
  }
  if (nonBlackPixels < 2048u) {
    x11PresenterDumpFrame("blank");
    x11PresenterSetVisible(0);
    goto out;
  }
  x11PresenterDumpFrame("present");

  if (!x11PresenterEnsureChildWindow()) goto out;
  if (!x11Presenter.gc) {
    x11Presenter.gc = XCreateGC(x11Presenter.display, x11Presenter.window, 0, NULL);
    if (!x11Presenter.gc) goto out;
    x11PresenterLog("created child gc");
  }
  x11PresenterSetVisible(1);

  for (y = 0; y < (size_t)x11Presenter.windowHeight; y++) {
    size_t srcY = (y * (size_t)x11Presenter.height) / (size_t)x11Presenter.windowHeight;
    for (x = 0; x < (size_t)x11Presenter.windowWidth; x++) {
      size_t srcX = (x * (size_t)x11Presenter.width) / (size_t)x11Presenter.windowWidth;
      FxU16 px = x11Presenter.frontBuffer[srcY * (size_t)x11Presenter.width + srcX];
      unsigned char r = (unsigned char)(((px >> 11) & 0x1f) * 255 / 31);
      unsigned char g = (unsigned char)(((px >> 5) & 0x3f) * 255 / 63);
      unsigned char b = (unsigned char)((px & 0x1f) * 255 / 31);
      unsigned long nativePixel =
          x11PresenterScaleComponent(r, x11Presenter.redMax, x11Presenter.redShift) |
          x11PresenterScaleComponent(g, x11Presenter.greenMax, x11Presenter.greenShift) |
          x11PresenterScaleComponent(b, x11Presenter.blueMax, x11Presenter.blueShift);
      XPutPixel(x11Presenter.image, (int)x, (int)y, nativePixel);
    }
  }

  XPutImage(x11Presenter.display,
            x11Presenter.window,
            x11Presenter.gc,
            x11Presenter.image,
            0, 0, 0, 0,
             (unsigned)x11Presenter.windowWidth,
             (unsigned)x11Presenter.windowHeight);
  XRaiseWindow(x11Presenter.display, x11Presenter.window);
  XSync(x11Presenter.display, False);
  x11PresenterLog("swap presented");

out:
  x11Presenter.presenting = 0;
}

#else
void _grX11PresenterInit(FxU32 hWnd, FxU32 width, FxU32 height) {
  (void)hWnd;
  (void)width;
  (void)height;
}

void _grX11PresenterSwap(void) {}

void _grX11PresenterShutdown(void) {}
#endif
