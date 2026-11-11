"""
Open ■
┬────┴  prepare_flash_size
■ KNX   2026 OpenKNX - Erkan Çolak

PlatformIO extra_script (pre:): prepare_flash_size.py
Sets the flash size and the derived flash layout from one place.

Optional. It only acts when the environment sets custom_flash_size:
  custom_flash_size = hwcfg   size from DEVICE_FLASH_SIZE_MB of the board in OGM-HardwareConfig;
                              without that define the ini values stay in use
  custom_flash_size = 8MB     this size, the board header is not used for it
Without custom_flash_size nothing changes.

Derived layout (an explicit key wins over the derived value):
  ESP32  board_build.partitions = custom_flash_partitions, else esp32_<N>mb.csv of OGM-Common
         (the largest table not bigger than the flash); upload.flash_size = size of that table.
         prepare_esp32_partitions.py runs afterwards and places knx/openknx as before.
  RP     upload.maximum_size = flash; build.filesystem_size = custom_fs_size, else
         flash - 4096 (EEPROM) - end of the highest KNX/OPENKNX flash region.
With hwcfg and a board header without the define, only the explicit key of the platform is applied
(RP custom_fs_size against the ini flash size, ESP custom_flash_partitions); all other ini values stay.

Replaced values are listed in the build log. The build stops on an invalid value, a failing
preprocessor run for hwcfg, or a filesystem that overlaps a KNX/OPENKNX region.

Registered as "pre:" before prepare_esp32_partitions.py: both platform scripts read the layout
while they run, which is after the pre: scripts. Clean targets are skipped (pre: scripts run
before the clean, possibly without an installed toolchain).
"""
Import("env")

import glob
import os
import re
import subprocess
import sys
import tempfile

sys.path.insert(0, next((p for p in ("lib/OGM-Common/scripts/pio", "scripts/pio")
                         if os.path.exists(os.path.join(p, "_pio_common.py"))), "."))
from _pio_common import step, ok, warn, err

EEPROM_SIZE = 4096
SECTOR = 0x1000
MIB = 1024 * 1024
ESP32_TABLES = (4, 8, 16)  # data/partitions/esp32_<N>mb.csv
ESP32_TABLE_DIR = "lib/OGM-Common/data/partitions"
ESPTOOL_SIZES = (1, 2, 4, 8, 16, 32, 64, 128)  # esptool --flash-size
REGIONS = (("KNX_FLASH_OFFSET", "KNX_FLASH_SIZE"), ("OPENKNX_FLASH_OFFSET", "OPENKNX_FLASH_SIZE"))


def _fail(msg):
    err("Flash size: " + msg)
    env.Exit(1)


def _option(name):
    return str(env.GetProjectOption(name, "") or "").strip()


def _mb(text, name):
    """'8MB' / '8M' / '8' -> 8"""
    m = re.fullmatch(r"(\d+)\s*(MB|M)?", str(text).strip(), re.IGNORECASE)
    if not m or int(m.group(1)) <= 0:
        _fail("%s = %s is not a size in MB (e.g. 4MB)" % (name, text))
    return int(m.group(1))


def _bytes(text, name):
    """custom_fs_size: decimal or hex bytes, or a whole number with K/KB/M/MB"""
    m = re.fullmatch(r"(0x[0-9a-fA-F]+|\d+)\s*(K|KB|M|MB)?", text.strip(), re.IGNORECASE)
    if not m:
        _fail("%s = %s is not a size (e.g. 1056768, 0x102000, 1032K)" % (name, text))
    scale = {"": 1, "K": 1024, "KB": 1024, "M": MIB, "MB": MIB}[(m.group(2) or "").upper()]
    number = m.group(1)
    return int(number, 16 if number.lower().startswith("0x") else 10) * scale


def _effective_defines():
    """BUILD_FLAGS minus BUILD_UNFLAGS, matched like PlatformIO's ProcessUnFlags."""
    def parse(flags):
        flags = [f for f in (flags or []) if str(f).strip()]
        return env.ParseFlagsExtended(flags).get("CPPDEFINES", []) if flags else []

    defines = parse(env.get("BUILD_FLAGS"))
    for unflag in parse(env.get("BUILD_UNFLAGS")):
        for current in list(defines):
            if unflag == current or (not isinstance(unflag, (tuple, list))
                                     and isinstance(current, (tuple, list)) and unflag == current[0]):
                defines.remove(current)
    return defines


def _define(defines, name):
    """Value of a -D flag as int, None when unset; no value or differing values stop the build."""
    values = set(str(d[1]) for d in defines if isinstance(d, (tuple, list)) and d[0] == name)
    if any(d == name for d in defines if not isinstance(d, (tuple, list))):
        values.add("")
    if not values:
        return None
    if len(values) > 1:
        _fail("%s is set more than once with different values: %s" % (name, ", ".join(sorted(values))))
    value = values.pop()
    try:
        return int(value, 0)
    except ValueError:
        _fail("%s=%s is not a number" % (name, value))


def _toolchain_gcc(platform):
    for name, opts in platform.packages.items():
        if opts.get("type") != "toolchain":
            continue
        pdir = platform.get_package_dir(name)
        if not pdir:
            continue
        hits = sorted(glob.glob(os.path.join(pdir, "bin", "*-gcc")) + glob.glob(os.path.join(pdir, "bin", "*-gcc.exe")))
        if hits:
            return hits[0]
    return None


def _header_flash_mb(platform, defines):
    """DEVICE_FLASH_SIZE_MB from HardwareConfig.h, preprocessed with the -D/-I of BUILD_FLAGS and ARDUINO_ARCH_*.

    Returns (MB, None), (0, None) without the define, or (None, problem).
    """
    gcc = _toolchain_gcc(platform)
    if not gcc:
        return None, "no toolchain compiler found to read HardwareConfig.h"
    args = [gcc, "-E", "-dM", "-x", "c"]
    arch = {"espressif32": "ARDUINO_ARCH_ESP32", "raspberrypi": "ARDUINO_ARCH_RP2040"}.get(platform.name)
    if arch:
        args.append("-D" + arch)
    for d in defines:
        args.append("-D%s=%s" % (d[0], d[1]) if isinstance(d, (tuple, list)) else "-D%s" % d)
    flags = [f for f in (env.get("BUILD_FLAGS") or []) if str(f).strip()]
    for inc in (env.ParseFlagsExtended(flags).get("CPPPATH", []) if flags else []):
        args.append("-I" + env.subst(str(inc)))
    src = tempfile.NamedTemporaryFile("w", suffix=".c", delete=False)
    src.write('#include "HardwareConfig.h"\n')
    src.close()
    try:
        res = subprocess.run(args + [src.name], cwd=env.subst("$PROJECT_DIR"), capture_output=True,
                             text=True, encoding="utf-8", errors="replace")
    finally:
        os.unlink(src.name)
    if res.returncode:
        lines = res.stderr.strip().splitlines() or ["?"]
        first = next((l for l in lines if "error:" in l.lower()), lines[-1])
        return None, "preprocessing HardwareConfig.h failed: " + first.strip()
    for line in res.stdout.splitlines():
        parts = line.split(None, 2)
        if len(parts) == 3 and parts[0] == "#define" and parts[1] == "DEVICE_FLASH_SIZE_MB":
            value = parts[2].strip().strip("()").strip()
            try:
                return int(value, 0), None
            except ValueError:
                return None, "DEVICE_FLASH_SIZE_MB = %s is not a number" % parts[2]
    return 0, None


def _report_change(key, old, new):
    if old is not None and str(old) != str(new):
        print("  %s %s replaced by %s" % (key, old, new))


def _apply_esp32(board, flash_mb):
    own = _option("custom_flash_partitions")
    if own:
        table, table_mb = own, flash_mb
        if table_mb not in ESPTOOL_SIZES:
            _fail("%d MB is no flash size esptool accepts (%s MB)" % (table_mb, "/".join(map(str, ESPTOOL_SIZES))))
    else:
        fitting = [n for n in ESP32_TABLES if n <= flash_mb]
        if not fitting:
            _fail("no ESP32 partition table for %d MB flash (tables: %s MB)" % (flash_mb, "/".join(map(str, ESP32_TABLES))))
        table_mb = fitting[-1]
        if table_mb != flash_mb:
            warn("no partition table for %d MB, using the %d MB table" % (flash_mb, table_mb))
        table = "%s/esp32_%dmb.csv" % (ESP32_TABLE_DIR, table_mb)
    _report_change("board_build.partitions", board.get("build.partitions", None), table)
    _report_change("board_upload.flash_size", board.get("upload.flash_size", None), "%dMB" % table_mb)
    board.update("build.partitions", table)
    board.update("upload.flash_size", "%dMB" % table_mb)
    ok("partitions %s" % table, "flash %d MB" % table_mb)


def _rp_filesystem(flash, defines):
    """custom_fs_size, else flash - EEPROM - end of the KNX/OPENKNX region; checked against that region."""
    region_end = 0
    for offset_flag, size_flag in REGIONS:
        offset, size = _define(defines, offset_flag), _define(defines, size_flag)
        if offset is None or not size:
            _fail("%s and %s are needed to place the filesystem" % (offset_flag, size_flag))
        region_end = max(region_end, offset + size)
    own = _option("custom_fs_size")
    fs = _bytes(own, "custom_fs_size") if own else flash - EEPROM_SIZE - region_end
    if fs <= 0 or fs % SECTOR:
        _fail("filesystem size %d must be > 0 and a multiple of %d" % (fs, SECTOR))
    fs_start = flash - EEPROM_SIZE - fs
    if fs_start < region_end:
        _fail("filesystem 0x%06X..0x%06X overlaps the KNX/OPENKNX region ending at 0x%06X" % (max(fs_start, 0), fs_start + fs, region_end))
    return fs, fs_start


def _set_rp_filesystem(board, fs, fs_start):
    _report_change("board_build.filesystem_size", board.get("build.filesystem_size", None), fs)
    board.update("build.filesystem_size", str(fs))  # the platform parses this value as text
    ok("filesystem 0x%06X..0x%06X (%d B)" % (fs_start, fs_start + fs, fs))


def _apply_rp(board, flash_mb, defines):
    flash = flash_mb * MIB
    fs, fs_start = _rp_filesystem(flash, defines)
    _report_change("board_upload.maximum_size", board.get("upload.maximum_size", None), flash)
    board.update("upload.maximum_size", flash)
    ok("flash %d MB" % flash_mb)
    _set_rp_filesystem(board, fs, fs_start)


def _apply_explicit_only(platform, board, defines):
    """hwcfg, no header value: apply only the explicit key of this platform, keep everything else."""
    if platform.name == "espressif32":
        own = _option("custom_flash_partitions")
        if not own:
            return False
        _report_change("board_build.partitions", board.get("build.partitions", None), own)
        board.update("build.partitions", own)
        ok("partitions %s" % own, "flash size of the ini")
        return True
    if not _option("custom_fs_size"):
        return False
    flash = int(board.get("upload.maximum_size", 0))
    fs, fs_start = _rp_filesystem(flash, defines)
    _set_rp_filesystem(board, fs, fs_start)
    return True


def _main():
    setting = _option("custom_flash_size")
    if not setting or env.IsCleanTarget():
        return
    platform = env.PioPlatform()
    if platform.name not in ("espressif32", "raspberrypi"):
        _fail("custom_flash_size is supported on ESP32 and RP2040/RP2350 only")
    step("Flash size (custom_flash_size = %s)" % setting)
    defines = _effective_defines()
    board = env.BoardConfig()

    if setting.lower() == "hwcfg":
        flash_mb, problem = _header_flash_mb(platform, defines)
        if problem:
            _fail(problem)
        if not flash_mb:
            warn("board header has no DEVICE_FLASH_SIZE_MB", "ini values stay in use")
            _apply_explicit_only(platform, board, defines)
            return
        source = "DEVICE_FLASH_SIZE_MB"
    else:
        flash_mb = _mb(setting, "custom_flash_size")
        source = "custom_flash_size"
        header_mb, _ = _header_flash_mb(platform, defines)
        if header_mb and header_mb != flash_mb:
            ok("custom_flash_size %d MB overrides the board header (%d MB)" % (flash_mb, header_mb))
    print("  flash size %d MB from %s" % (flash_mb, source))

    if platform.name == "espressif32":
        _apply_esp32(board, flash_mb)
    else:
        _apply_rp(board, flash_mb, defines)


_main()
