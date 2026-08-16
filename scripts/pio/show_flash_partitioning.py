Import("env")
Import("projenv")
import os
import re
import sys
import time
from platformio.proc import exec_command

sys.path.insert(0, next((p for p in ("lib/OGM-Common/scripts/pio", "scripts/pio")
                         if os.path.exists(os.path.join(p, "_pio_common.py"))), "."))
from _pio_common import C, section, ok, warn, err, human_bytes


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
        OTA_COMPRESS_RATIO = 0.65
        FS_USABLE_FRACTION = 0.90
        est_compressed = int(firmware_end * OTA_COMPRESS_RATIO)
        fs_usable = int(fs_size * FS_USABLE_FRACTION)
        opt_in_assert = any(
            (d == "OPENKNX_OTA_FS_ASSERT") or
            (isinstance(d, (tuple, list)) and len(d) > 0 and d[0] == "OPENKNX_OTA_FS_ASSERT")
            for d in projenv["CPPDEFINES"])

        section("Update over the KNX bus (knxOTA)", "(the compressed image is staged in the filesystem first)")
        print("{}firmware {} · est. gzip {} (x{:.2f}) · filesystem {} · usable {} (x{:.2f}){}".format(
            C.GRAY, human_bytes(firmware_end), human_bytes(est_compressed), OTA_COMPRESS_RATIO,
            human_bytes(fs_size), human_bytes(fs_usable), FS_USABLE_FRACTION, C.END))

        if est_compressed > fs_usable:
            # Genuine miss: red so it stands out from the amber "tight" case. Does NOT fail the build by
            # itself (other OAMs must not break); only the opt-in OPENKNX_OTA_FS_ASSERT hard-fails.
            err("OTA will not fit", "flash over USB, or enlarge board_build.filesystem_size / shrink firmware (est. {} > usable {})".format(
                human_bytes(est_compressed), human_bytes(fs_usable)))
            if opt_in_assert:
                sys.stdout.flush()
                sys.stderr.flush()
                time.sleep(1)
                sys.exit(1)
        elif est_compressed > int(fs_size * 0.82):
            warn("OTA headroom tight", "est. compressed ~{} vs filesystem {}".format(
                human_bytes(est_compressed), human_bytes(fs_size)))
        else:
            ok("fits", "est. compressed ~{} in usable {}".format(human_bytes(est_compressed), human_bytes(fs_usable)))

    print()


if projenv["PIOPLATFORM"] != "espressif32":
    env.AddPostAction("checkprogsize", show_flash_partitioning)
