Import("env")
Import("projenv")
import re
import os
from platformio.proc import exec_command

class console_color:
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    END = '\033[0m'

class OversizedError(Exception):
    pass

class FlashRegion:
    def __init__(self, name, start, end, container=False):
        self.name = name
        self.start = start
        self.end = end
        self.container = container
    def __str__(self):
        return f"FlashRegion(Name: {self.name}, Start: {self.start}, End: {self.end}, Container: {self.container})"

    def size(self):
        return self.end - self.start

    def oversized(self, flash):
        # Bin ich ein Container
        if (self.container):
            return False

        for element in flash:
            # Darf es nicht selber sein
            if (self == element):
                continue

            # Liegt das Element vor mir
            if(element.end <= self.start):
                continue

            # Liegt das Element hinter mir
            if(element.start >= self.end):
                continue

            # Passe ich in das Element rein
            if(element.container and element.start <= self.start and element.end >= self.end):
                continue

            return True

        return False

def show_flash_partitioning(source, target, env):

    # start of flash address pointer
    flash_addr_pointer = 268435456

    def build_tree(start, end, flash, indent = 0, stack = []):
        prev = start
        empty = True
        found_oversized = False

        for i, element in enumerate(flash):
            if (element in stack):
                continue

            if (element.start >= start and element.start < end):
                if (prev != element.start and element.start > prev):
                    print_entry(console_color.GREEN, FlashRegion('FREE', prev, element.start), indent)

                stack.append(element)
                prev = element.end
                empty = False

                color = console_color.CYAN
                if (element.container):
                    color = console_color.BLUE

                # Oversize
                if (element.oversized(flash)):
                    color = console_color.RED
                    found_oversized = True

                print_entry(color, element, indent)

                if (element.container):
                    if build_tree(element.start, element.end, flash, indent+1, stack):
                        found_oversized = True

        if (not empty and prev < end):
            print_entry(console_color.GREEN, FlashRegion('FREE', prev, end), indent)

        return found_oversized

    def build_entry(element, indent = 0):
        return (
            "{}{}{} - {} ({} Bytes)".format(
                "".rjust(indent*2, " "),
                (element.name + ":").ljust(30-indent*2, ' '),
                format(element.start, "#010x"),
                format(element.end, "#010x"),
                format(element.end-element.start, "#10")
            )
        )

    def print_entry(color, element, indent = 0):
        print("{}{}{}".format(color, build_entry(element, indent), console_color.END))

    def find_header_file(file):
        folders = [
            "src/",
            "include/",
            "lib/OGM-Common/include/",
            projenv["PROJECT_LIBDEPS_DIR"] + "/" + env["PIOENV"] + "/OGM-Common/include/"
        ]
        for folder in folders:
            if os.path.isfile(folder + file):
                return folder + file
            
        return file
                
    def firmware_size(env):
        size = 0
        sizetool = env.get("SIZETOOL")
        sysenv = os.environ.copy()
        sysenv["PATH"] = str(env["ENV"]["PATH"])
        result = exec_command([env.subst(sizetool), '-A', '-d', str(source[0])], env=sysenv)

        searches = ["\.ARM\.exidx", "\.ARM\.extab", "\.rodata", "\.text"]
        for search in searches:
            m = re.search(search + "\s+(\d+)\s+(\d+)", str(result))
            if m is not None:
                size += int(m.group(1))
                size += int(m.group(2))
                break

        m = re.search("\.data\s+(\d+)\s+(\d+)", str(result))
        if m is not None:
            size += int(m.group(1))
        
        if projenv['PIOPLATFORM'] == 'raspberrypi':
            size -= flash_addr_pointer

        return size

    def get_knxprod_define_value(name):
        content = open(find_header_file("knxprod.h"), 'r').read()
        m = re.search("#define " + name + " ([0-9]+)", content)
        if m is None:
            return 0
        return int(m.group(1))

    # print(str(source[0]))
    # print(env.Dictionary())
    # print(projenv.Dictionary())

    flash_elements = []

    flash_start = 0
    firmware_start = 0
    flash_end = 0

    firmware_end = firmware_size(env)
    if projenv['PIOPLATFORM'] == 'raspberrypi':
        eeprom_start = env["PICO_EEPROM_START"] - flash_addr_pointer
        flash_end = eeprom_start + 4096

        if env["FS_START"] > 0 and env["FS_START"] != env["FS_END"]:
            filesystem_start = env["FS_START"] - flash_addr_pointer
            filesystem_end = env["FS_END"] - flash_addr_pointer
            flash_elements.append(FlashRegion('FILESYSTEM', filesystem_start, filesystem_end, ))
        
    if projenv['PIOPLATFORM'] == 'atmelsam':
        flash_end = 0x40000

    eeprom_end = flash_end

    flash_elements.append(FlashRegion('FLASH', flash_start, flash_end, True))
    flash_elements.append(FlashRegion('FIRMWARE', firmware_start, firmware_end))
    if projenv['PIOPLATFORM'] != 'atmelsam':
        flash_elements.append(FlashRegion('EEPROM', eeprom_start, eeprom_end))

    defined_sizes = {}
    for x in projenv["CPPDEFINES"]:
        if type(x) is tuple:
            name = x[0]
            if (x[0].endswith("FLASH_OFFSET") or x[0].endswith("FLASH_SIZE")):
                name = name.replace("_FLASH_OFFSET", "")
                name = name.replace("_FLASH_SIZE", "")
                if(name not in defined_sizes):
                    defined_sizes[name] = { 'offset': 0, 'size': 0 }

                if(x[0].endswith("FLASH_OFFSET")):
                    defined_sizes[name]['offset'] = int(x[1], 16)

                if(x[0].endswith("_FLASH_SIZE")):
                    defined_sizes[name]['size'] = int(x[1], 16)

    if projenv['PIOPLATFORM'] == 'atmelsam' and defined_sizes['KNX']['offset'] <= 0:
        defined_sizes['KNX']['offset'] = system_end - defined_sizes['KNX']['size']

    # Schätzung der nutzung des knx speichers
    # Größe der Parameter
    knx_parameter_size = get_knxprod_define_value("MAIN_ParameterSize")
    # Größe der KO Tabelle
    knx_ko_table_size = get_knxprod_define_value("MAIN_MaxKoNumber") * 2
    # Größe der GA Tabelle geschätzt
    # Annahme, dass im Schnitt 2 GA mit einem KO verknüpft wird = get_knx_max_ko_number * 4 (Eintrag) * 2 (GAs)
    knx_ga_table_size = knx_ko_table_size * 4
    # Metadaten & etwas Overhead
    knx_meta = 100
    # Zusammen gerechnete Größe
    knx_used = knx_meta + knx_parameter_size + knx_ko_table_size + knx_ga_table_size

    for name, data in defined_sizes.items():
        if data['offset'] > 0 and data['size'] > 0:
            container = False
            if name == "KNX" and knx_used > 0:
                container = True
                flash_elements.append(FlashRegion("DATA*", data['offset'], data['offset'] + knx_used))
            flash_elements.append(FlashRegion(name, data['offset'], data['offset'] + data['size'], container))


    sorted_flash_elements = sorted(flash_elements, key=lambda element: (element.start, -element.size()))
    print("")
    stack = []
    print("{}Show flash partitioning:{}".format(console_color.YELLOW, console_color.END))
    found_oversized = build_tree(flash_start, flash_end, sorted_flash_elements, 1, stack)
    if (knx_used > 0):
        print("")
        print("* This value is an estimate")
    if found_oversized:
        print("")
        print("{} ERROR OVERSIZED {}".format(console_color.RED, console_color.END))
        raise OversizedError()
    print("")


if projenv['PIOPLATFORM'] != 'espressif32':
    env.AddPostAction("checkprogsize", show_flash_partitioning)