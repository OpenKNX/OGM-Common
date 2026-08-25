# ---------------------------------------------------------------------------
#  OpenKNX ESP32 combined-image builder
#
#  Merges bootloader, partition table, boot_app0 and the application into the
#  single firmware.factory.bin that a USB flash writes to offset 0.
#
#  The report shows what actually ends up on the chip: one row per image with
#  its REAL size and the region it occupies. Earlier versions printed the gap
#  to the next image as if it were the file size (a 2 MB application appeared
#  as "7 MB") and drew the usage bar against a hard-coded 8 MB regardless of
#  the board, so both numbers disagreed with the flash size printed above them.
# ---------------------------------------------------------------------------

Import("env")
platform = env.PioPlatform()

import gzip
import os
import subprocess
import sys
from os.path import basename, join

sys.path.insert(0, next((p for p in ("lib/OGM-Common/scripts/pio", "scripts/pio")
                         if os.path.exists(os.path.join(p, "_pio_common.py"))), "."))
from _pio_common import C, RULE, warn, identity_line, show_lib_sizes
from _pio_common import human_bytes as _human


def _flash_bytes(spec):
    """'16MB' / '4MB' -> bytes. Falls back to 4 MB, the smallest OpenKNX ESP32 layout."""
    try:
        return int(str(spec).upper().replace("MB", "")) * 1024 * 1024
    except ValueError:
        return 4 * 1024 * 1024


# Partition types worth naming; anything else is shown by its raw type/subtype.
_P_TYPE = {0x00: "app", 0x01: "data"}
_P_SUB = {
    (0x00, 0x00): "factory", (0x00, 0x10): "ota_0", (0x00, 0x11): "ota_1",
    (0x00, 0x20): "test",
    (0x01, 0x00): "ota data", (0x01, 0x01): "phy", (0x01, 0x02): "nvs",
    (0x01, 0x03): "coredump", (0x01, 0x04): "nvs keys", (0x01, 0x05): "efuse",
    (0x01, 0x82): "spiffs", (0x01, 0x83): "littlefs",
}


def _partitions(part_path):
    """Every entry of the partition table: (offset, size, label, kind)."""
    try:
        with open(part_path, "rb") as f:
            tbl = f.read()
    except OSError:
        return []
    out = []
    for off in range(0, len(tbl) - 32 + 1, 32):
        e = tbl[off:off + 32]
        if e[0] != 0xAA or e[1] != 0x50:
            continue
        p_type, p_sub = e[2], e[3]
        start = int.from_bytes(e[4:8], "little")
        size = int.from_bytes(e[8:12], "little")
        label = e[12:28].split(b"\x00")[0].decode("ascii", "replace")
        kind = _P_SUB.get((p_type, p_sub), f"{_P_TYPE.get(p_type, hex(p_type))}/{hex(p_sub)}")
        out.append((start, size, label, kind))
    out.sort()
    return out


def _show_partitions(part_path, total, app_size):
    """The flash map as the chip really has it — read from the table, not inferred.

    Gaps between partitions are shown too: unpartitioned flash is invisible to the firmware and is the
    first place to look when a filesystem turns out smaller than expected.
    """
    parts = _partitions(part_path)
    if not parts:
        return
    print(f"{C.BOLD}Partitions{C.END}  {C.GRAY}(from the partition table){C.END}")
    prev_end = parts[0][0]
    for start, size, label, kind in parts:
        if start > prev_end:
            print(f"{C.GRAY}  {hex(prev_end):<10} {_human(start - prev_end):>10}   (unpartitioned){C.END}")
        used = ""
        if kind in ("factory", "ota_0") and app_size and size > 0:
            pct = app_size * 100 // size
            col = C.AMBER if pct > 90 else C.GREEN
            used = f"   {col}{_human(app_size)} used · {pct} %{C.END}"
        print(f"  {C.BLUE}{hex(start):<10}{C.END} {_human(size):>10}   {label:<10} {C.GRAY}{kind}{C.END}{used}")
        prev_end = start + size
    if total > prev_end:
        print(f"{C.GRAY}  {hex(prev_end):<10} {_human(total - prev_end):>10}   (unpartitioned){C.END}")


def _fs_partition(part_path):
    """Size of the LittleFS/SPIFFS partition, read from the partition table (type 0x01, subtype 0x82/0x83).

    Entries are 32 bytes: magic 0xAA50, type, subtype, offset, size, 16-byte label, flags.
    """
    try:
        with open(part_path, "rb") as f:
            tbl = f.read()
    except OSError:
        return 0
    for off in range(0, len(tbl) - 32 + 1, 32):
        e = tbl[off:off + 32]
        if e[0] != 0xAA or e[1] != 0x50:
            continue
        p_type, p_sub = e[2], e[3]
        size = int.from_bytes(e[8:12], "little")
        if p_type == 0x01 and p_sub in (0x82, 0x83):  # DATA/SPIFFS, DATA/LITTLEFS
            return size
    return 0


def _packed_size(app_path, app_size):
    """Size of the staged image: gzip the real binary instead of guessing.

    The estimate this replaced used a flat 0.65 ratio. That is far off for an ESP application (measured
    0.55-0.58 on RP2350/C3/C6 builds) and turned layouts that fit in practice into a false "does not fit".
    Compressing the actual file costs a fraction of a second and reports what the device will really
    receive. The ratio stays as a fallback for the case where the binary is not readable yet.
    """
    try:
        with open(app_path, "rb") as fh:
            return len(gzip.compress(fh.read(), 9)), True
    except (OSError, ValueError):
        return int(app_size * 0.65), False


def _ota_slots(part_path):
    """Number of switchable app slots (ota_0/ota_1) in the table.

    Staging space is only half the story: applying the image needs a slot to write into. With a single
    app partition esp_ota_get_next_update_partition() does not fail, it returns the RUNNING partition —
    so a report that only compares sizes would promise an update that cannot be applied.
    """
    return sum(1 for _, _, _, kind in _partitions(part_path) if kind in ("ota_0", "ota_1"))


BUS_SLOW = 480  # B/s, OpenKNX IP-Interface / IPro
BUS_FAST = 630  # B/s, MDT with its fast mode
# Measured on real image pairs: a packed release-to-release patch came out at 8 % of the image, a raw one
# at 13 %. Eight is what a normal release costs; a release that moves a lot of code can double it.
PATCH_FRACTION = 0.08


def _has_define(name):
    """Is a build flag set for this environment?"""
    try:
        from SCons.Script import DefaultEnvironment
        defines = DefaultEnvironment().get("CPPDEFINES", [])
    except Exception:
        return False
    return any((d == name) or (isinstance(d, (tuple, list)) and len(d) > 0 and d[0] == name) for d in defines)


def _bus_time(nbytes):
    """How long that many bytes take on the bus, as the span between a slow and a fast interface."""
    slow = nbytes / BUS_SLOW / 60.0
    fast = nbytes / BUS_FAST / 60.0
    if slow < 1.5:
        return f"{nbytes / BUS_FAST:.0f}-{nbytes / BUS_SLOW:.0f} s"
    if slow < 10:
        return f"{fast:.1f}-{slow:.1f} min"
    return f"{fast:.0f}-{slow:.0f} min"


def _row(label, over_bus, staged, verdict, note=""):
    """One line of the update table.

    The verdict leads, because that is the one thing a reader is looking for; the numbers behind it are
    what makes the verdict checkable. Columns stay aligned so the two ways to update can be compared
    without reading a sentence.
    """
    glyph, col = {"ok": ("✔", C.GREEN), "no": ("✘", C.RED),
                  "off": ("·", C.GRAY), "usb": ("!", C.AMBER)}[verdict]
    bus = _human(over_bus) if over_bus else "—"
    st = _human(staged) if staged else "—"
    tm = _bus_time(over_bus) if over_bus else "—"
    print(f"  {col}{glyph}{C.END} {label:<26}{bus:>10}{st:>11}{tm:>12}"
          + (f"   {C.GRAY}{note}{C.END}" if note else ""))


def _ota_fit(app_size, fs_size, app_path=None, part_path=None):
    """Report whether a firmware update over the KNX bus can be staged on this device.

    The staged image lands in the filesystem before it is written to the OTA slot, so the filesystem — not
    the OTA partition — is the limit. A device that unpacks the stream receives it compressed; one that does
    not receives the whole image, so both numbers matter and both are shown.
    """
    if fs_size <= 0 or app_size <= 0:
        return
    USABLE = 0.90     # filesystem overhead plus room to breathe
    usable = int(fs_size * USABLE)
    packed, measured = _packed_size(app_path, app_size) if app_path else (int(app_size * 0.65), False)
    how = "gzip -9" if measured else "estimated"

    slots = _ota_slots(part_path) if part_path is not None else None
    delta_on = _has_define("OPENKNX_FTC_DELTA_UPDATE")

    print(f"{C.BOLD}{C.CYAN}knxOTA{C.END}  {C.BOLD}firmware update over the KNX bus{C.END}  "
          f"{C.GRAY}what it costs to reach this device without touching it{C.END}")
    print(f"{C.GRAY}    {'':<26}{'over bus':>10}{'staged':>11}{'time':>12}{C.END}")

    if slots is not None and slots < 2:
        _row("full image · gzip", packed, packed, "usb", "single app partition — nothing can be applied here")
        print(f"{C.GRAY}    filesystem {_human(fs_size)} · usable {_human(usable)} · {C.AMBER}no second app slot{C.END}")
        print(f"{C.GRAY}    other ways in: ArduinoOTA over the network · USB (esptool, firmware.factory.bin){C.END}")
        return

    # Full image: the whole thing travels and the whole thing is staged.
    if packed <= usable:
        _row("full image · gzip", packed, packed, "ok")
    else:
        _row("full image · gzip", packed, packed, "no", f"needs {_human(packed)} of {_human(usable)} usable")

    # Delta: the patch travels AND is the only thing staged -- on ESP32 the rebuilt image goes straight
    # into the second app slot and never becomes a file. That is the whole difference to RP.
    patch = int(app_size * PATCH_FRACTION)
    if not delta_on:
        _row("delta patch", None, None, "off", "add -D OPENKNX_FTC_DELTA_UPDATE to offer it")
    elif patch <= usable:
        _row("delta patch · typical", patch, patch, "ok", "rebuilt straight into the second slot")
    else:
        _row("delta patch · typical", patch, patch, "no", f"needs {_human(patch)} of {_human(usable)} usable")

    print(f"{C.GRAY}    filesystem {_human(fs_size)} · usable {_human(usable)} · app slots {slots} · {how} ({packed * 100 // app_size} %) · applied by esp_ota{C.END}")
    if delta_on:
        print(f"{C.GRAY}    a patch is ~{int(PATCH_FRACTION * 100)} % of the image for a normal release; a big change can double it{C.END}")
    # knxOTA is not the only way in. ArduinoOTA writes straight into the slot and stages nothing, so a
    # verdict here says nothing about whether the device can be reached remotely at all.
    print(f"{C.GRAY}    other ways in: ArduinoOTA over the network (stages nothing) · USB (esptool, firmware.factory.bin){C.END}")


def _identity(app_path):
    """Read the OpenKNX identity back out of the finished app image (esp_app_desc_t.reserv1).

    Reading it here rather than trusting the stamper means the line printed is what a client will actually
    find in the file — the stamp is reported only once it demonstrably survived into the binary.
    """
    try:
        with open(app_path, "rb") as f:
            d = f.read(48)
    except OSError:
        return None
    if len(d) < 44 or d[0] != 0xE9 or d[40:44] == b"\x00\x00\x00\x00":
        return None
    return d[40], d[41], d[42], d[43]


def _report(images, total, app_max=0):
    """One table: where each image goes, how big it really is, how much room it has.

    The application's room is its PARTITION, not the rest of the chip — that is the number the size check
    reports and the only one that can actually overflow.
    """
    print(f"{C.BOLD}Flash image{C.END}  {C.GRAY}(chip flash {_human(total)}){C.END}")
    print(f"{C.GRAY}{'OFFSET':<10}{'SIZE':>10}  {'ROOM':>10}  NAME{C.END}")

    used = 0
    for idx, (off, path) in enumerate(images):
        size = os.path.getsize(path) if os.path.exists(path) else 0
        used += size
        if idx + 1 < len(images):
            room = images[idx + 1][0] - off
        elif app_max:
            room = app_max
        else:
            room = total - off
        tight = size > room * 0.9
        color = C.AMBER if tight else (C.BLUE if idx % 2 else C.GREEN)
        print(f"{color}{hex(off):<10}{_human(size):>10}  {C.GRAY}{_human(room):>10}{color}  {basename(path)}{C.END}")

    # Usage bar over the real flash size: filled = written, dim = free.
    width = 66
    fill = min(width, max(1, int(used / total * width)))
    print(f"{C.GRAY}0x000000{C.END} [{C.GREEN}{'█' * fill}{C.GRAY}{'·' * (width - fill)}{C.END}] {C.GRAY}{_human(total)}{C.END}")
    print(f"{C.GRAY}written {_human(used)} · {used * 100 // total} % of the chip{C.END}")


try:
    sys.path.append(join(platform.get_package_dir("tool-esptoolpy")))
    import esptool  # noqa: F401  (presence check only; the merge runs as a subprocess)
except Exception:
    print("Installing esptool...")
    subprocess.run([sys.executable, "-m", "pip", "install", "--upgrade", "esptool"])


def esp32_create_combined_bin(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    prog_name = env.subst("${PROGNAME}")
    out = f"{build_dir}/{prog_name}.factory.bin"
    fw = f"{build_dir}/{prog_name}.bin"

    chip = env.get("BOARD_MCU")
    flash_size = env.BoardConfig().get("upload.flash_size")
    flash_freq = env.BoardConfig().get("build.f_flash", "40m").replace("000000L", "m")
    flash_mode = env.BoardConfig().get("build.flash_mode", "dio")
    memory_type = env.BoardConfig().get("build.arduino.memory_type", "qio_qspi")

    if flash_mode in ["qio", "qout"]:
        flash_mode = "dio"
    if memory_type in ["opi_opi", "opi_qspi"]:
        flash_mode = "dout"

    # Each extra image EXACTLY once: appending them twice puts the same file at the same offset, which
    # esptool >= 5.x rejects as an overlap ("Detected overlap at address: 0x0").
    images = []
    cmd = ["esptool", "--chip", chip, "merge-bin", "-o", out,
           "--flash-mode", flash_mode, "--flash-freq", flash_freq, "--flash-size", flash_size]
    for offset, image in env["FLASH_EXTRA_IMAGES"]:
        off = int(env.subst(str(offset)), 16)
        img = env.subst(str(image))
        images.append((off, img))
        cmd += [hex(off), img]
    images.append((0x10000, fw))
    cmd += [hex(0x10000), fw]
    images.sort(key=lambda x: x[0])

    print()
    print(RULE)
    print(f"{C.BOLD}OpenKNX ESP32 firmware image{C.END}   {C.GRAY}{chip} · {flash_mode} · {flash_freq} · {flash_size}{C.END}")
    ident = _identity(fw)
    if ident:
        print(identity_line(*ident, note="stamped, checksum + SHA-256 intact"))
    else:
        warn("identity", "not stamped — a client cannot tell which device this file is for")
    print(RULE)
    app_size_for_libs = os.path.getsize(fw) if os.path.exists(fw) else 0
    try:
        app_max = int(env.BoardConfig().get("upload.maximum_size", 0))
    except (TypeError, ValueError):
        app_max = 0
    print(RULE)
    show_lib_sizes(env, app_size_for_libs)
    print(RULE)
    _report(images, _flash_bytes(flash_size), app_max)
    part = next((p for o, p in images if basename(p).startswith("partitions")), None)
    app_size = os.path.getsize(fw) if os.path.exists(fw) else 0
    if part:
        print(RULE)
        _show_partitions(part, _flash_bytes(flash_size), app_size)
        fs = _fs_partition(part)
        if fs:
            print(RULE)
            _ota_fit(app_size, fs, fw, part)
    print(RULE)

    try:
        subprocess.run(cmd, check=True, capture_output=True, text=True)
    except subprocess.CalledProcessError as err:
        # Only now is the exact command worth printing — it is what you need to reproduce the failure.
        print(f"{C.AMBER}esptool failed:{C.END}\n{err.stderr or err.stdout}")
        print(f"{C.GRAY}{' '.join(cmd)}{C.END}")
        raise

    print(f"{C.GREEN}✔{C.END} {basename(out)}   {C.GRAY}{_human(os.path.getsize(out))} · flash to offset 0x0{C.END}")
    print(f"{C.GRAY}  {build_dir}{C.END}")
    print(RULE)
    print()


env.AddPostAction("buildprog", esp32_create_combined_bin)
