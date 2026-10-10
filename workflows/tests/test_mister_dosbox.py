import importlib.machinery
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
LOADER = importlib.machinery.SourceFileLoader("prepare_dosbox", str(ROOT / "workflows/build/prepare-mister-dosbox"))
SPEC = importlib.util.spec_from_loader(LOADER.name, LOADER)
MODULE = importlib.util.module_from_spec(SPEC)
LOADER.exec_module(MODULE)


class MisterDosboxTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=ROOT / "output")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / "build/deps/mister/dosbox-x"
        self.gui = self.source / "src/gui"
        self.gui.mkdir(parents=True)
        self.original = ('#include "keyboard.h"\nvoid GFX_Events() {\n}\n'
                         '        if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) >= 0)\n'
                         '            OUTPUT_SURFACE_EndUpdate(changedLines);\n'
                         '        DOS_ShutdownDrives();\n        DOS_ShutdownFiles();\n')
        (self.gui / "sdlmain.cpp").write_text(self.original)
        (self.gui / "Makefile.am").write_text("libgui_a_SOURCES = sdlmain.cpp\n")
        self.hardware = self.source / "src/hardware"
        self.hardware.mkdir()
        self.glide = '''class GLIDE_PageHandler : public PageHandler {
public:
    PhysPt base_addr[1] = {0x60000000};
    PhysPt GetLinPt(unsigned buffer) { return 0x60000000 + buffer * 0x800000; }
    GLIDE_PageHandler() { flags=PFLAG_READABLE|PFLAG_WRITEABLE|PFLAG_NOCODE; }
'''
        for suffix, ctype in (("b", "uint8_t"), ("w", "uint16_t"), ("d", "uint32_t")):
            self.glide += f'''    {ctype} read{suffix}(PhysPt addr) override {{
        return addr;
    }}
    void write{suffix}(PhysPt addr,{ctype} val) override {{
        mem_writed(addr, val);
    }}
'''
        for access in ("Read", "Write"):
            self.glide += f'''    HostPt GetHost{access}Pt(PageNum phys_page) override {{
        return reinterpret_cast<HostPt>(phys_page);
    }}
'''
        self.glide += '''};
static GLIDE_PageHandler handler;
static struct { bool enabled = true; GLIDE_PageHandler *lfb_pagehandler = &handler; } glide;
void GLIDE_DisableScreen(void)
{
    glide.enabled = false;
}
static void process_msg(Bitu value) {
    ret = param[0];
    ++host_calls;
    if (value == _grLfbLock24 || value == _grLfbUnlock8) mem_writed(ret, 1);
}
'''
        (self.hardware / "glide.cpp").write_text(self.glide)
        self.ints = self.source / 'src/ints'
        self.ints.mkdir()
        self.media = '''#include "cpu.h"
    this->diskimg = diskimg;
imageDisk::imageDisk(FILE* imgFile, const char* imgName, uint32_t imgSizeK, bool isHardDisk) : diskSizeK(imgSizeK), diskimg(imgFile), image_length((uint64_t)imgSizeK * 1024) {
fwrite(data, size, count, diskimg); fclose(diskimg);
fseek(diskimg, offset, SEEK_SET); fseeko64(diskimg, offset, SEEK_SET);
'''
        (self.ints / 'bios_disk.cpp').write_text(self.media)
        (self.source / 'src/dos').mkdir()
        (self.source / 'src/dos/dos.cpp').write_text('#include "dosbox.h"\nvoid DOS_ShutdownFiles() {\n}\n')
        (self.source / 'src/dos/dos_programs.cpp').write_text('#include "dosbox.h"\nFILE * fopen_lock(const char * fname, const char * mode, bool &readonly) {\n}\n')
        (self.source / "COPYING").write_text("license")
        (self.source / ".git").mkdir()
        for name in ("dosbox_input.cpp", "dosbox_output.cpp", "dosbox_media.cpp", "dosbox_media.h", "mister_runtime.h", "mister_mailbox.h", "dosbox_glide.h", "glide_guest.h"):
            directory = "software/runtime/mister" if name.startswith("mister_") else "software/integrations/glide" if name == "glide_guest.h" else "software/integrations/dosbox"
            local = self.root / directory
            local.mkdir(parents=True, exist_ok=True)
            (local / name).write_text(name)
        self.destination = self.root / "output/build"
        self.addCleanup(patch.stopall)
        patch.object(MODULE, "ROOT", self.root).start()

    def test_prepare_preserves_pinned_sources_and_license(self):
        with patch.object(MODULE.subprocess, "run") as run:
            MODULE.prepare(self.destination)
        run.assert_called_once()
        self.assertEqual((self.gui / "sdlmain.cpp").read_text(), self.original)
        self.assertEqual((self.hardware / "glide.cpp").read_text(), self.glide)
        self.assertEqual((self.destination / "COPYING").read_text(), "license")
        self.assertFalse((self.destination / ".git").exists())
        text = (self.destination / "src/gui/sdlmain.cpp").read_text()
        self.assertIn('if (mister_dosbox_input()) E_Exit', text)
        self.assertIn('if (mister_dosbox_start()) E_Exit', text)
        self.assertIn('if (!glide.enabled && mister_dosbox_present(sdl.surface, &sdl.clip)) E_Exit', text)
        self.assertIn('dosbox_output.cpp', (self.destination / "src/gui/Makefile.am").read_text())
        self.assertIn('dosbox_input.cpp', (self.destination / "src/gui/Makefile.am").read_text())
        self.assertEqual((self.destination / "src/gui/mister_runtime.h").read_text(), "mister_runtime.h")
        with self.assertRaisesRegex(ValueError, "already exists"):
            MODULE.prepare(self.destination)

    def test_checked_media_integration_is_exact_and_preserves_upstream(self):
        with patch.object(MODULE.subprocess, 'run'):
            MODULE.prepare(self.destination)
        self.assertEqual((self.ints / 'bios_disk.cpp').read_text(), self.media)
        text = (self.destination / 'src/ints/bios_disk.cpp').read_text()
        self.assertEqual(text.count('mister_media_track('), 2)
        gui = (self.destination / 'src/gui/sdlmain.cpp').read_text()
        self.assertLess(gui.index('mister_dosbox_flush_media_files();'), gui.index('DOS_ShutdownDrives();'))
        dos = (self.destination / 'src/dos/dos.cpp').read_text()
        self.assertIn('Files[i]->Flush();', dos)
        for name in ('mister_media_write', 'mister_media_close', 'mister_media_seek'):
            self.assertIn(name + '(', text)
        with self.assertRaises(ValueError):
            MODULE.integrate_media(self.media + self.media)
        with self.assertRaises(ValueError):
            MODULE.integrate_media(self.media.replace('fwrite(', 'missing('))
        (self.destination / 'src/ints/bios_disk.cpp').write_text('stale')
        with patch.object(MODULE.subprocess, 'run'), self.assertRaisesRegex(ValueError, 'stale or modified'):
            MODULE.prepare(self.destination, check=True)

    def test_check_rejects_stale_missing_and_modified_integration(self):
        with patch.object(MODULE.subprocess, "run"):
            with self.assertRaisesRegex(ValueError, "stale or modified"):
                MODULE.prepare(self.destination, check=True)
            self.assertFalse(self.destination.exists())
            MODULE.prepare(self.destination)
            MODULE.prepare(self.destination, check=True)
            path = self.destination / "src/gui/dosbox_output.cpp"
            path.write_text("stale")
            with self.assertRaisesRegex(ValueError, "stale or modified"):
                MODULE.prepare(self.destination, check=True)
            self.assertEqual(path.read_text(), "stale")

    def test_check_rejects_missing_or_modified_lfb_policy(self):
        with patch.object(MODULE.subprocess, "run"):
            MODULE.prepare(self.destination)
            path = self.destination / "src/hardware/glide.cpp"
            path.write_text(self.glide)
            with self.assertRaisesRegex(ValueError, "stale or modified"):
                MODULE.prepare(self.destination, check=True)
            self.assertEqual(path.read_text(), self.glide)
            path.unlink()
            with self.assertRaisesRegex(ValueError, "stale or modified"):
                MODULE.prepare(self.destination, check=True)
            self.assertFalse(path.exists())

    def test_lfb_patch_sites_must_be_unique_and_complete(self):
        for site in ("ret = param[0];", "GLIDE_DisableScreen", "flags=PFLAG_READABLE|PFLAG_WRITEABLE|PFLAG_NOCODE;",
                     "readb", "readw", "readd", "writeb", "writew", "writed", "GetHostReadPt", "GetHostWritePt"):
            with self.subTest(site=site):
                text = self.glide.replace(site, "missing")
                (self.hardware / "glide.cpp").write_text(text)
                with patch.object(MODULE.subprocess, "run"):
                    with self.assertRaisesRegex(ValueError, "exactly one"):
                        MODULE.prepare(self.destination)
                self.assertFalse(self.destination.exists())
        with self.assertRaisesRegex(ValueError, "exactly one"):
            MODULE.integrate_lfb(self.glide + self.glide)
        method = "    uint8_t readb(PhysPt addr) override {\n        return addr;\n    }\n"
        with self.assertRaisesRegex(ValueError, "exactly one"):
            MODULE.integrate_lfb(self.glide + method)

    def test_direct_lfb_routes_to_checked_bus_access_without_shadow_pointers(self):
        with patch.object(MODULE.subprocess, "run"):
            MODULE.prepare(self.destination)
        source = self.destination / "src/hardware/glide.cpp"
        prelude = '''#include <cassert>
#include <cstdint>
#include <stdexcept>
using PhysPt = uint32_t; using PageNum = uint32_t; using Bitu = uint32_t;
using HostPt = uint8_t *;
constexpr unsigned PFLAG_READABLE = 1, PFLAG_WRITEABLE = 2, PFLAG_NOCODE = 4;
constexpr unsigned _grLfbLock24 = 1, _grLfbUnlock8 = 2, FXFALSE = 0, G_OK = 7;
constexpr unsigned GLIDE_PAGE_BITS = 11;
static unsigned ret, param[7], ret_value, host_calls, writes, last_address, last_value;
static unsigned lfbacc = 3, locks, unlocks, resets, flushes, bus_buffer, bus_offset, bus_size, bus_value;
struct mister_guest_lfb_info { unsigned size, pointer, stride, mode, origin; } info = {20, 0, 0, 0, 0};
static void mem_writed(unsigned addr, unsigned value) { ++writes; last_address = addr; last_value = value; }
static void MEM_BlockRead32(unsigned addr, void *ptr, unsigned size) { assert(addr == 100 && size == 20); *static_cast<mister_guest_lfb_info *>(ptr) = info; }
static void MEM_BlockWrite32(unsigned addr, void *ptr, unsigned size) { assert(addr == 100 && size == 20); info = *static_cast<mister_guest_lfb_info *>(ptr); }
static int mister_guest_lock(unsigned type, unsigned buffer, unsigned, unsigned, unsigned, mister_guest_lfb_info *p) { assert(type == 1 && buffer == 1); ++locks; p->stride = 2048; return 1; }
static int mister_guest_unlock(unsigned type, unsigned buffer) { assert(type == 1 && buffer == 1); ++unlocks; return 1; }
static void mister_guest_reset() { ++resets; }
static void mister_guest_flush() { ++flushes; }
static unsigned mister_guest_read(unsigned buffer, unsigned offset, unsigned size) { bus_buffer = buffer; bus_offset = offset; bus_size = size; return 0x42; }
static void mister_guest_write(unsigned buffer, unsigned offset, unsigned value, unsigned size) { mister_guest_read(buffer, offset, size); bus_value = value; }
#define LOG_MSG(...) ((void)0)
class PageHandler {
public:
    unsigned flags = 0;
'''
        for suffix, ctype in (("b", "uint8_t"), ("w", "uint16_t"), ("d", "uint32_t")):
            prelude += f"    virtual {ctype} read{suffix}(PhysPt) = 0;\n    virtual void write{suffix}(PhysPt, {ctype}) = 0;\n"
        prelude += "    virtual HostPt GetHostReadPt(PageNum) = 0;\n    virtual HostPt GetHostWritePt(PageNum) = 0;\n};\n"
        main = '''int main() {
    param[1] = 1; param[2] = 1; param[6] = 100;
    for (unsigned op : {_grLfbLock24, _grLfbUnlock8}) {
        param[0] = 42; ret_value = 0; writes = 0;
        process_msg(op);
        assert(writes == 1 && last_address == 42 && last_value == 1 && ret_value == G_OK);
        param[0] = 0; ret_value = 99; writes = 0;
        process_msg(op);
        assert(writes == 0 && ret_value == 99);
    }
    assert(locks == 1 && unlocks == 1 && info.pointer == 0x60800000 && info.stride == 2048);
    param[0] = 42;
    for (unsigned buffer : {3u, 0xffffffffu}) { param[2] = buffer; process_msg(_grLfbLock24); assert(!last_value); }
    param[2] = 1; lfbacc = 0;
    process_msg(_grLfbLock24); assert(!last_value && locks == 1);
    lfbacc = 3; param[2] = 2;
    process_msg(_grLfbLock24); assert(!last_value && locks == 1);
    assert(host_calls == 0);
    process_msg(3); assert(host_calls == 1);
    assert(flushes == 9);
    assert(handler.flags == PFLAG_NOCODE);
    assert(!handler.GetHostReadPt(0x60000) && !handler.GetHostWritePt(0xffffffff));
'''
        for suffix, size in (("b", 1), ("w", 2), ("d", 4)):
            main += f"    assert(handler.read{suffix}(0x60800802) == 0x42 && bus_buffer == 1 && bus_offset == 2050 && bus_size == {size});\n"
            main += f"    handler.write{suffix}(0x61000003, 0x77); assert(bus_buffer == 2 && bus_offset == 3 && bus_size == {size} && bus_value == 0x77);\n"
        main += "    GLIDE_DisableScreen(); assert(resets == 1); param[2] = 1; process_msg(_grLfbLock24); assert(!last_value);\n}\n"
        harness = self.root / "lfb.cpp"
        harness.write_text(prelude + source.read_text().replace('#include "../gui/dosbox_glide.h"', '') + main)
        binary = self.root / "lfb"
        subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic", str(harness), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)

    def test_invalid_path_revision_or_patch_site_does_not_create_destination(self):
        with self.assertRaisesRegex(ValueError, "inside the workspace"):
            MODULE.prepare(self.root / "external")
        with patch.object(MODULE.subprocess, "run", side_effect=subprocess.CalledProcessError(1, "verify")):
            with self.assertRaises(subprocess.CalledProcessError):
                MODULE.prepare(self.destination)
        self.assertFalse(self.destination.exists())
        for text in ("no integration sites", self.original + self.original):
            (self.gui / "sdlmain.cpp").write_text(text)
            with patch.object(MODULE.subprocess, "run"):
                with self.assertRaisesRegex(ValueError, "exactly one"):
                    MODULE.prepare(self.destination)
            self.assertFalse(self.destination.exists())


if __name__ == "__main__":
    unittest.main()
