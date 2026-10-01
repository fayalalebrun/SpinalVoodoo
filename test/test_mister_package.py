import configparser
import importlib.machinery
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
LOADER = importlib.machinery.SourceFileLoader("mister_package", str(ROOT / "scripts/package-mister-runtime"))
SPEC = importlib.util.spec_from_loader(LOADER.name, LOADER)
MODULE = importlib.util.module_from_spec(SPEC)
LOADER.exec_module(MODULE)


class MisterPackageTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=ROOT / "output")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def elf(self, name):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        header = bytearray(52)
        header[:6] = b"\x7fELF\x01\x01"
        struct.pack_into("<H", header, 18, 40)
        struct.pack_into("<I", header, 36, 0x5000400)
        path.write_bytes(header)
        return path

    def test_default_config_enables_glide_passthrough(self):
        config = configparser.ConfigParser()
        config.read(ROOT / "emu/mister/dosbox-x.conf")
        self.assertTrue(config.getboolean("voodoo", "glide"))
        self.assertFalse(config.getboolean("voodoo", "voodoo_card"))
        self.assertEqual(config.get("voodoo", "lfb"), "full")

    def test_arm_abi_validation(self):
        path = self.elf("elf")
        MODULE.arm_elf(path)
        for offset, value in ((4, 2), (5, 2), (18, 62), (39, 4), (37, 0)):
            self.elf("elf")
            header = bytearray(path.read_bytes())
            header[offset] = value
            path.write_bytes(header)
            with self.assertRaises(ValueError):
                MODULE.arm_elf(path)
        path.write_bytes(b"ELF")
        with self.assertRaises(ValueError):
            MODULE.arm_elf(path)

    def test_dependency_closure_includes_origin_loader_and_dynamic_sdl3(self):
        executable = self.elf("bin/dosbox-x")
        runtime = self.elf("local/libmister_runtime.so")
        glide = self.elf("local/libglide2x.so")
        loader = self.elf("vendor/ld-linux-armhf.so.3")
        libc = self.elf("vendor/libc.so.6")
        sdl2 = self.elf("vendor/libSDL2.so")
        sdl3 = self.elf("vendor/libSDL3.so.0")
        def metadata(*args):
            option, path = args[1], Path(args[2])
            if option == "--print-interpreter":
                return str(loader)
            if option == "--print-rpath":
                return str(libc.parent) if path in (executable, runtime, glide) else ""
            return "libSDL2.so\nlibc.so.6" if path == executable else "ld-linux-armhf.so.3" if path == libc else ""
        with patch.object(MODULE, "command", side_effect=metadata):
            files = MODULE.dependencies(executable, runtime, glide)
            self.assertEqual(files["libSDL3.so.0"], sdl3)
            self.assertEqual(files["ld-linux-armhf.so.3"], loader)
            sdl3.rename(self.root / "absent")
            with self.assertRaisesRegex(ValueError, "unresolved dependency libSDL3"):
                MODULE.dependencies(executable, runtime, glide)
        with patch.object(MODULE, "command", return_value="relative"):
            with self.assertRaisesRegex(ValueError, "relative library"):
                MODULE.dependencies(executable, runtime, glide)

    def test_cached_output_uses_a_locally_valid_derivation(self):
        source = self.root / "vendor"
        source.mkdir()
        (source / "LICENSE").write_text("license")
        metadata = {"local.drv": {"env": {"src": str(source), "name": "vendor"}}}
        def run(*args):
            if "--valid-derivers" in args:
                return "/nix/store/local.drv"
            if "--deriver" in args:
                return "/nix/store/missing.drv"
            if args[:3] == ("nix", "derivation", "show"):
                self.assertEqual(args[3], "/nix/store/local.drv")
                return json.dumps(metadata)
            return str(source)
        with patch.object(MODULE, "command", side_effect=run), patch.object(MODULE.subprocess, "check_output", return_value=b""):
            MODULE.sources(self.root, {"library": Path("/nix/store/cached/lib/library.so")})
        self.assertIn("cached", json.loads((self.root / "sources/inventory.json").read_text()))

    def test_nix_derivation_formats(self):
        environment = {"src": "/nix/store/source", "name": "library"}
        derivations = {"library.drv": {"env": environment}}
        self.assertEqual(MODULE.derivation_environment(derivations), environment)
        self.assertEqual(MODULE.derivation_environment({"version": 1, "derivations": derivations}), environment)
        structured = {"library.drv": {"env": {}, "structuredAttrs": environment}}
        self.assertEqual(MODULE.derivation_environment(structured), environment)
        self.assertEqual(MODULE.derivation_environment({"version": 1, "derivations": structured}), environment)
        for metadata in ({}, {"library.drv": {}}, {"a": {}, "b": {}}):
            with self.assertRaises(ValueError):
                MODULE.derivation_environment(metadata)

    def test_archive_reproducibility_and_destination_guards(self):
        path = self.elf("source")
        MODULE.archive(self.root / "a.tar.gz", {"source": path})
        MODULE.archive(self.root / "b.tar.gz", {"source": path})
        self.assertEqual((self.root / "a.tar.gz").read_bytes(), (self.root / "b.tar.gz").read_bytes())
        with self.assertRaises(ValueError):
            MODULE.package(self.root, self.root)
        with self.assertRaises(ValueError):
            MODULE.package(ROOT / "not-output", self.root)

    def test_launcher_default_and_audio_device_guard(self):
        bundle = self.root / "bundle"
        (bundle / "lib").mkdir(parents=True)
        shutil.copyfile(ROOT / "emu/mister/launch", bundle / "launch")
        loader = bundle / "lib/ld-linux-armhf.so.3"
        loader.write_text('#!/bin/sh\nprintf "%s\\n" "$SDL_AUDIO_DRIVER" "$ALSA_CONFIG_PATH" "$@"\n')
        loader.chmod(0o755)
        (bundle / "SHA256SUMS").write_bytes(subprocess.check_output(
            ["sha256sum", "launch", "lib/ld-linux-armhf.so.3"], cwd=bundle))
        tools = self.root / "tools"
        tools.mkdir()
        mkdir = tools / "mkdir"
        mkdir.write_text('#!/bin/sh\nprintf "%s\\n" "$@" > "$MKDIR_LOG"\n')
        mkdir.chmod(0o755)
        environment = dict(os.environ, PATH=str(tools) + ":" + os.environ["PATH"],
                           MKDIR_LOG=str(self.root / "mkdir.log"))
        result = subprocess.run(["sh", str(bundle / "launch"), "-c", "exit"],
                                capture_output=True, text=True, env=environment)
        self.assertEqual((self.root / "mkdir.log").read_text().splitlines(),
                         ["-p", "home", "/run/lock"])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.splitlines()[:2], ["dummy", str(bundle / "asound.conf")])
        self.assertEqual(result.stdout.splitlines()[-2:], ["-c", "exit"])
        if not Path("/dev/MrAudio").exists():
            result = subprocess.run(["sh", str(bundle / "launch"), "--audio"],
                                    capture_output=True, text=True, env=environment)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("requires writable /dev/MrAudio", result.stderr)

        mkdir.write_text('#!/bin/sh\nexit 1\n')
        result = subprocess.run(["sh", str(bundle / "launch")],
                                capture_output=True, text=True, env=environment)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "")

    def test_package_includes_private_audio_configuration(self):
        destination = self.root / "bundle"
        executable = self.elf("dosbox-x")
        with patch.object(MODULE, "command", return_value=""), \
             patch.object(MODULE, "dependencies", return_value={"dosbox-x": executable}), \
             patch.object(MODULE, "sources"):
            MODULE.package(destination, self.root)
        self.assertEqual((destination / "asound.conf").read_bytes(),
                         (ROOT / "emu/mister/asound.conf").read_bytes())
        self.assertIn("  asound.conf\n", (destination / "SHA256SUMS").read_text())
        self.assertEqual((destination / "asound.conf").stat().st_mode & 0o777, 0o644)

    def test_installer_rejects_recursion_and_preserves_existing_destination(self):
        bundle = self.root / "bundle"
        bundle.mkdir()
        shutil.copyfile(ROOT / "emu/mister/install", bundle / "install")
        (bundle / "data").write_text("preserved")
        sums = subprocess.check_output(["sha256sum", "data", "install"], cwd=bundle)
        (bundle / "SHA256SUMS").write_bytes(sums)
        def install(destination):
            return subprocess.run(["sh", str(bundle / "install"), str(destination)], capture_output=True)
        self.assertNotEqual(install(bundle / "recursive").returncode, 0)
        (self.root / "alias").symlink_to(bundle, target_is_directory=True)
        self.assertNotEqual(install(self.root / "alias/recursive").returncode, 0)
        target = self.root / "installed"
        self.assertEqual(install(target).returncode, 0)
        self.assertEqual((target / "data").read_text(), "preserved")
        (target / "data").write_text("user data")
        self.assertNotEqual(install(target).returncode, 0)
        self.assertEqual((target / "data").read_text(), "user data")
        (bundle / "data").write_text("corrupt")
        self.assertNotEqual(install(self.root / "bad").returncode, 0)
        self.assertFalse((self.root / "bad").exists())


if __name__ == "__main__":
    unittest.main()
