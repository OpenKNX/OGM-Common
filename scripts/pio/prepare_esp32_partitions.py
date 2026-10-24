"""
Open ■
┬────┴  prepare_esp32_partitions
■ KNX   2026 OpenKNX - Erkan Çolak

PlatformIO extra_script (pre:): prepare_esp32_partitions.py
Places the ESP32 "knx" / "openknx" partitions from the same build flags the RP2040 uses.

Optional. It only acts when KNX_FLASH_OFFSET and/or OPENKNX_FLASH_OFFSET is set (after
build_unflags); without them the configured board_build.partitions is used unchanged.
For each set offset the matching *_SIZE must be set too. The configured table is the base:
only the offset/size of the affected row is replaced, every other row stays. The result is
written to $BUILD_DIR/partitions_openknx.csv and becomes build.partitions.

The build stops when an offset is not 64 KiB aligned (the partition is memory-mapped), a size is
not 4 KiB aligned, a partition overlaps another one or ends behind the flash.

Registered as "pre:" on purpose: the Arduino and the IDF framework script both read
build.partitions while the platform script runs, which is after the pre: scripts. CPPDEFINES do
not exist yet at that point, so the flags are read from BUILD_FLAGS / BUILD_UNFLAGS here.
"""
Import("env")

import os
import sys

sys.path.insert(0, next((p for p in ("lib/OGM-Common/scripts/pio", "scripts/pio")
                         if os.path.exists(os.path.join(p, "_pio_common.py"))), "."))
from _pio_common import step, ok, err

MMU_PAGE = 0x10000
SECTOR = 0x1000
REGIONS = (("knx", "KNX_FLASH_OFFSET", "KNX_FLASH_SIZE"),
           ("openknx", "OPENKNX_FLASH_OFFSET", "OPENKNX_FLASH_SIZE"))


def _fail(msg):
    err("ESP32 partitions: " + msg)
    env.Exit(1)


def _defines(flags):
    """CPPDEFINES of a flag list, as PlatformIO parses it."""
    flags = [f for f in (flags or []) if str(f).strip()]
    if not flags:
        return []
    return env.ParseFlagsExtended(flags).get("CPPDEFINES", [])


def _effective_defines():
    """BUILD_FLAGS minus BUILD_UNFLAGS, with the same matching as PlatformIO's ProcessUnFlags."""
    defines = _defines(env.get("BUILD_FLAGS"))
    for unflag in _defines(env.get("BUILD_UNFLAGS")):
        for current in list(defines):
            if unflag == current or (not isinstance(unflag, (tuple, list))
                                     and isinstance(current, (tuple, list)) and unflag == current[0]):
                defines.remove(current)
    return defines


def _flag(defines, name):
    """Value of a -D flag, None when unset; differing values are refused instead of guessed."""
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


def _size(text):
    """Offset/size field of a partition CSV: hex, decimal, or with a K/M suffix."""
    text = text.strip()
    scale = 1
    if text[-1:] in ("K", "k"):
        text, scale = text[:-1], 1024
    elif text[-1:] in ("M", "m"):
        text, scale = text[:-1], 1024 * 1024
    return int(text, 0) * scale


def _base_table(board):
    name = board.get("build.partitions", board.get("build.arduino.partitions", ""))
    if not name:
        _fail("no board_build.partitions configured to place knx/openknx in")
    candidates = [env.subst(name)]
    for package in ("framework-arduinoespressif32", "framework-espidf"):
        try:
            package_dir = env.PioPlatform().get_package_dir(package)
        except Exception:
            package_dir = None
        if package_dir:
            candidates.append(os.path.join(package_dir, "tools", "partitions", name))
            candidates.append(os.path.join(package_dir, "components", "partition_table", name))
    for path in candidates:
        if os.path.isfile(path):
            return os.path.abspath(path)
    _fail("partition table %s not found" % name)


def _flash_size(board):
    text = str(board.get("upload.flash_size", "4MB")).upper().replace("B", "")
    return _size(text)


defines = _effective_defines()
wanted = {}
for label, offset_flag, size_flag in REGIONS:
    offset = _flag(defines, offset_flag)
    if offset is None:
        continue
    size = _flag(defines, size_flag)
    if not size:
        _fail("%s is set, so %s must be set too and not be 0" % (offset_flag, size_flag))
    if offset < MMU_PAGE or offset % MMU_PAGE:
        _fail("%s=0x%X must be a multiple of 0x%X (memory-mapped partition)" % (offset_flag, offset, MMU_PAGE))
    if size % SECTOR:
        _fail("%s=0x%X must be a multiple of 0x%X" % (size_flag, size, SECTOR))
    wanted[label] = (offset, size)

if wanted:
    board = env.BoardConfig()
    base = _base_table(board)
    step("Generate partition table from the KNX/OPENKNX flash flags")

    with open(base) as f:
        lines = f.read().splitlines()

    rows = []
    out = []
    for line in lines:
        fields = [x.strip() for x in line.split(",")]
        if not line.strip() or line.lstrip().startswith("#") or len(fields) < 5:
            out.append(line)
            continue
        label = fields[0]
        if not fields[3] or not fields[4]:
            _fail("%s: row %s has no explicit offset/size" % (os.path.basename(base), label))
        if label in wanted:
            offset, size = wanted.pop(label)
            fields[3], fields[4] = "0x%X" % offset, "0x%X" % size
            line = "%-13s %-5s %-8s %-11s %s" % (fields[0] + ",", fields[1] + ",", fields[2] + ",",
                                                fields[3] + ",", ", ".join(fields[4:]))
            ok("%-8s 0x%06X  size 0x%X" % (label, offset, size))
        else:
            offset, size = _size(fields[3]), _size(fields[4])
        rows.append((label, offset, size))
        out.append(line)

    if wanted:
        _fail("%s has no partition named %s" % (os.path.basename(base), ", ".join(sorted(wanted))))

    flash = _flash_size(board)
    for i, (label, offset, size) in enumerate(rows):
        if offset + size > flash:
            _fail("%s ends at 0x%X, behind the 0x%X flash" % (label, offset + size, flash))
        for other, o_offset, o_size in rows[i + 1:]:
            if offset < o_offset + o_size and o_offset < offset + size:
                _fail("%s (0x%X..0x%X) overlaps %s (0x%X..0x%X)" % (
                    label, offset, offset + size, other, o_offset, o_offset + o_size))

    # The Arduino builder turns the file name into -DARDUINO_PARTITION_<name>: no dot in the name
    target = os.path.join(env.subst("$BUILD_DIR"), "partitions_openknx.csv")
    content = "# Generated by OGM-Common prepare_esp32_partitions.py from %s -- do not edit\n%s\n" % (
        os.path.basename(base), "\n".join(out))
    if not os.path.isfile(target) or open(target).read() != content:
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with open(target, "w") as f:
            f.write(content)
    board.update("build.partitions", target)
    print("  base %s" % os.path.relpath(base))
