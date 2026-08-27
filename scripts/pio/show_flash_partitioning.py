
Import("env")
Import("projenv")
import os
import re
import sys
import time
from platformio.proc import exec_command

sys.path.insert(0, next((p for p in ("lib/OGM-Common/scripts/pio", "scripts/pio")
                         if os.path.exists(os.path.join(p, "_pio_common.py"))), "."))
from _pio_common import C, section, ok, warn, err, human_bytes, show_lib_sizes, quiet_action, G_OK, G_FAIL


class FlashRegion:
    def __init__(self, name, start, end, container=False):
        self.name = name
        self.start = start
        self.end = end
        self.container = container

    def size(self):
        return self.end - self.start

    def oversized(self, flash):
        if self.container:
            return False
        for element in flash:
            if self == element:
                continue
            if element.end <= self.start:  # element before me
                continue
            if element.start >= self.end:  # element after me
                continue
            if element.container and element.start <= self.start and element.end >= self.end:  # I fit inside it
                continue
            return True
        return False


def show_flash_partitioning(source, target, env):
    flash_addr_pointer = 268435456

    def build_tree(start, end, flash, indent=0, stack=[]):
        prev = start
        empty = True
        found_oversized = False
        for element in flash:
            if element in stack:
                continue
            if element.start >= start and element.start < end:
                if prev != element.start and element.start > prev:
                    print_entry(C.GRAY, FlashRegion("free", prev, element.start), indent)
                stack.append(element)
                prev = element.end
                empty = False
                color = C.BLUE if element.container else C.CYAN
                if element.oversized(flash):
                    color = C.RED
                    found_oversized = True
                print_entry(color, element, indent)
                if element.container:
                    if build_tree(element.start, element.end, flash, indent + 1, stack):
                        found_oversized = True
        if not empty and prev < end:
            print_entry(C.GRAY, FlashRegion("free", prev, end), indent)
        return found_oversized

    def print_entry(color, element, indent=0):
        name = "  " * indent + element.name
        print("  {}{:<24}{} {}0x{:06x} - 0x{:06x}{}  {}{:>10}{}".format(
            color, name, C.END,
            C.GRAY, element.start, element.end, C.END,
            color, human_bytes(element.size()), C.END))

    def find_header_file(file):
        folders = ["src/", "include/", "lib/OGM-Common/include/",
                   projenv["PROJECT_LIBDEPS_DIR"] + "/" + env["PIOENV"] + "/OGM-Common/include/"]
        for folder in folders:
            if os.path.isfile(folder + file):
                return folder + file
        return file

    def firmware_size(env):
        size = 0
        sizetool = env.get("SIZETOOL")
        sysenv = os.environ.copy()
        sysenv["PATH"] = str(env["ENV"]["PATH"])
        result = exec_command([env.subst(sizetool), "-A", "-d", str(source[0])], env=sysenv)
        for search in [r"\.ARM\.exidx", r"\.ARM\.extab", r"\.rodata", r"\.text"]:
            m = re.search(search + r"\s+(\d+)\s+(\d+)", str(result))
            if m is not None:
                size += int(m.group(1)) + int(m.group(2))
                break
        m = re.search(r"\.data\s+(\d+)\s+(\d+)", str(result))
        if m is not None:
            size += int(m.group(1))
        if projenv["PIOPLATFORM"] == "raspberrypi":
            size -= flash_addr_pointer
        return size

    def get_knxprod_define_value(name):
        content = open(find_header_file("knxprod.h"), "r").read()
        m = re.search("#define " + name + " ([0-9]+)", content)
        return int(m.group(1)) if m else 0

    flash_elements = []
    flash_start = 0
    firmware_start = 0
    flash_end = 0
    firmware_end = firmware_size(env)

    if projenv["PIOPLATFORM"] == "raspberrypi":
        eeprom_start = env["PICO_EEPROM_START"] - flash_addr_pointer
        flash_end = eeprom_start + 4096
        if env["FS_START"] > 0 and env["FS_START"] != env["FS_END"]:
            filesystem_start = env["FS_START"] - flash_addr_pointer
            filesystem_end = env["FS_END"] - flash_addr_pointer
            flash_elements.append(FlashRegion("filesystem", filesystem_start, filesystem_end))

    if projenv["PIOPLATFORM"] == "atmelsam":
        flash_end = 0x40000

    eeprom_end = flash_end
    flash_elements.append(FlashRegion("FLASH", flash_start, flash_end, True))
    flash_elements.append(FlashRegion("firmware", firmware_start, firmware_end))
    if projenv["PIOPLATFORM"] != "atmelsam":
        flash_elements.append(FlashRegion("eeprom", eeprom_start, eeprom_end))

    defined_sizes = {}
    for x in projenv["CPPDEFINES"]:
        if type(x) is tuple:
            name = x[0]
            if x[0].endswith("FLASH_OFFSET") or x[0].endswith("FLASH_SIZE"):
                name = name.replace("_FLASH_OFFSET", "").replace("_FLASH_SIZE", "")
                if name not in defined_sizes:
                    defined_sizes[name] = {"offset": 0, "size": 0}
                if x[0].endswith("FLASH_OFFSET"):
                    defined_sizes[name]["offset"] = int(x[1], 16)
                if x[0].endswith("_FLASH_SIZE"):
                    defined_sizes[name]["size"] = int(x[1], 16)

    if projenv["PIOPLATFORM"] == "atmelsam" and defined_sizes["KNX"]["offset"] <= 0:
        defined_sizes["KNX"]["offset"] = system_end - defined_sizes["KNX"]["size"]

    # Estimated KNX flash usage: parameters + KO table + guessed GA/association tables + metadata.
    knx_parameter_size = get_knxprod_define_value("MAIN_ParameterSize")
    knx_ko_table_size = get_knxprod_define_value("MAIN_MaxKoNumber") * 2
    knx_ga_table_size = knx_ko_table_size * 2
    knx_association_table_size = knx_ko_table_size * 4
    knx_used = 100 + knx_parameter_size + knx_ko_table_size + knx_ga_table_size + knx_association_table_size

    for name, data in defined_sizes.items():
        if data["offset"] > 0 and data["size"] > 0:
            container = False
            if name == "KNX" and knx_used > 0:
                container = True
                flash_elements.append(FlashRegion("data*", data["offset"], data["offset"] + knx_used))
            flash_elements.append(FlashRegion(name, data["offset"], data["offset"] + data["size"], container))

    sorted_flash_elements = sorted(flash_elements, key=lambda element: (element.start, -element.size()))

    show_lib_sizes(env, firmware_end)

    section("Flash partitions", "(linker sizes + FLASH_OFFSET/SIZE defines)")
    found_oversized = build_tree(flash_start, flash_end, sorted_flash_elements, 1, [])
    if knx_used > 0:
        print("{}  * estimated KNX usage{}".format(C.GRAY, C.END))

    if found_oversized:
        warn("OVERSIZED", "a region does not fit its container -- see the red row above")
        sys.stdout.flush()
        sys.stderr.flush()
        time.sleep(1)  # PIO does not flush reliably
        sys.exit(1)

    # FW-update-over-KNX fit (RP2040/RP2350 only). KnxFileTransferClient gzips the firmware into
    # LittleFS and PicoOTA unpacks it at boot -> the COMPRESSED image must fit the filesystem. Guards
    # against firmware growth or a shrunk board_build.filesystem_size. Loud warning by default; opt-in
    # hard fail via -D OPENKNX_OTA_FS_ASSERT.
    if projenv["PIOPLATFORM"] == "raspberrypi" and env["FS_START"] > 0 and env["FS_START"] != env["FS_END"]:
        fs_size = env["FS_END"] - env["FS_START"]
        # Measured, not guessed. The fixed 0.65 was optimistic: a real image of this project compresses
        # to about 0.668, so the estimate came in ~19 KB LOW on a megabyte -- and that is exactly the
        # margin this check exists to judge. The binary is right there by the time this runs, so it is
        # gzipped for real; the ratio only stands in when it is not (a link-only run).
        OTA_COMPRESS_RATIO = 0.67
        FS_USABLE_FRACTION = 0.90
        est_compressed = int(firmware_end * OTA_COMPRESS_RATIO)
        est_measured = False
        try:
            import gzip as _gzip
            _bin = os.path.join(env.subst("$BUILD_DIR"), "firmware.bin")
            if os.path.isfile(_bin):
                with open(_bin, "rb") as _f:
                    est_compressed = len(_gzip.compress(_f.read(), 9))
                    est_measured = True
        except Exception as _e:
            print("{}    (could not measure the compressed size: {}){}".format(C.GRAY, _e, C.END))
        fs_usable = int(fs_size * FS_USABLE_FRACTION)
        opt_in_assert = any(
            (d == "OPENKNX_OTA_FS_ASSERT") or
            (isinstance(d, (tuple, list)) and len(d) > 0 and d[0] == "OPENKNX_OTA_FS_ASSERT")
            for d in projenv["CPPDEFINES"])

        # --- One table, both ways to update ---------------------------------------------------------
        # The two differ in what has to sit in the filesystem, and that is the whole story on RP: the
        # bootloader copies a FILE into the application area. A full update stages the COMPRESSED image
        # and the bootloader unpacks it; a delta stages the REBUILT image, uncompressed, because the
        # device produced it itself. So a device can be perfectly able to take a full update and still
        # have no room for a delta -- worth knowing here rather than in the field.
        BUS_SLOW, BUS_FAST = 480, 630  # B/s: OpenKNX IP-Interface/IPro .. MDT with its fast mode
        # Two different figures, and confusing them is what made an earlier version of this report promise
        # an update that then ran out of space on the device. What travels is the PACKED patch; what has to
        # lie next to the rebuilt image is the UNPACKED one, because that is the form the interpreter reads.
        # Both measured on real release-to-release pairs: 8 % packed, 13 % unpacked.
        PATCH_OVER_BUS = 0.08
        PATCH_ON_DISK = 0.13

        def bus_time(n):
            if n / BUS_SLOW < 90:
                return "{:.0f}-{:.0f} s".format(n / BUS_FAST, n / BUS_SLOW)
            fmt = "{:.1f}-{:.1f} min" if n / BUS_SLOW < 600 else "{:.0f}-{:.0f} min"
            return fmt.format(n / BUS_FAST / 60, n / BUS_SLOW / 60)

        def row(glyph, colour, label, over_bus, staged, note=""):
            print("  {}{}{} {:<26}{:>10}{:>11}{:>12}{}".format(
                colour, glyph, C.END, label,
                human_bytes(over_bus) if over_bus else "—",
                human_bytes(staged) if staged else "—",
                bus_time(over_bus) if over_bus else "—",
                "   {}{}{}".format(C.GRAY, note, C.END) if note else ""))

        # explicit -D, or implied by PROFILE_MANAGER (FileTransferConfig.h enables delta in that profile)
        delta_on = any(
            (d in ("OPENKNX_FTC_DELTA_UPDATE", "OPENKNX_FTC_PROFILE_MANAGER")) or
            (isinstance(d, (tuple, list)) and len(d) > 0 and d[0] in ("OPENKNX_FTC_DELTA_UPDATE", "OPENKNX_FTC_PROFILE_MANAGER"))
            for d in projenv["CPPDEFINES"])
        patch = int(firmware_end * PATCH_OVER_BUS)
        # The rebuilt image AND the unpacked patch exist at the same time -- the patch cannot be freed
        # while it is still being read.
        delta_needed = firmware_end + int(firmware_end * PATCH_ON_DISK)

        # Through section() so this block gets the same rule and spacing as every other one; the feature
        # name is coloured inside the title so it still leads the line.
        section("{}knxOTA{}  firmware update over the KNX bus".format(C.CYAN, C.END + C.BOLD),
                "what it costs to reach this device without touching it")
        print("{}    {:<26}{:>10}{:>11}{:>12}{}".format(C.GRAY, "", "over bus", "staged", "time", C.END))

        if est_compressed > fs_usable:
            row(G_FAIL, C.RED, "full image · gzip", est_compressed, est_compressed,
                "needs {} of {} usable".format(human_bytes(est_compressed), human_bytes(fs_usable)))
            if not est_measured:
                print("{}    (estimated from a ratio - the binary was not there to measure){}".format(C.GRAY, C.END))
        elif est_compressed > int(fs_size * 0.82):
            row("!", C.AMBER, "full image · gzip", est_compressed, est_compressed, "headroom is tight")
        else:
            row(G_OK, C.GREEN, "full image · gzip", est_compressed, est_compressed)

        if not delta_on:
            row("·", C.GRAY, "delta patch", None, None, "add -D OPENKNX_FTC_DELTA_UPDATE (or the MANAGER profile) to offer it")
        elif delta_needed > fs_size:
            row(G_FAIL, C.RED, "delta patch · typical", patch, delta_needed,
                "rebuilt image is staged uncompressed — no room next to it")
        elif delta_needed > int(fs_size * 0.95):
            row("!", C.AMBER, "delta patch · typical", patch, delta_needed, "headroom is tight")
        else:
            row(G_OK, C.GREEN, "delta patch · typical", patch, delta_needed)

        print("{}    firmware {} · filesystem {} · usable {} (x{:.2f}) · applied by picoOTA{}".format(
            C.GRAY, human_bytes(firmware_end), human_bytes(fs_size), human_bytes(fs_usable),
            FS_USABLE_FRACTION, C.END))
        if delta_on:
            print("{}    a normal release costs ~{} % of the image over the bus and ~{} % on the filesystem{}".format(
                C.GRAY, int(PATCH_OVER_BUS * 100), int(PATCH_ON_DISK * 100), C.END))
        # knxOTA is not the only way to reach a device without touching it. ArduinoOTA streams straight
        # into the application area and stages nothing at all, so a filesystem too small for knxOTA says
        # nothing about whether the device can be updated remotely -- worth stating, or the reader walks
        # away believing USB is the only option left.
        print("{}    other ways in: ArduinoOTA over the network (stages nothing) · USB (.uf2 via BOOTSEL){}".format(
            C.GRAY, C.END))

        if est_compressed > fs_usable:
            # Genuine miss. Does NOT fail the build by itself (other OAMs must not break); only the
            # opt-in OPENKNX_OTA_FS_ASSERT hard-fails.
            err("knxOTA will not fit", "use ArduinoOTA over the network or USB, or enlarge board_build.filesystem_size / shrink firmware (est. {} > usable {})".format(
                human_bytes(est_compressed), human_bytes(fs_usable)))
            if opt_in_assert:
                sys.stdout.flush()
                sys.stderr.flush()
                time.sleep(1)
                sys.exit(1)

    print()


if projenv["PIOPLATFORM"] != "espressif32":
    # Bare function -> SCons prints its own call line above the report. The block has a header of
    # its own, so the label is noise; an empty command string suppresses it.
    env.AddPostAction("checkprogsize", quiet_action(show_flash_partitioning))
