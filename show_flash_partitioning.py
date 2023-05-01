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

def show_flash_partitioning(source, target, env):
    def _configure_defaults():
        env.Replace(
            SIZECHECKCMD="$SIZETOOL -B -d $SOURCES",
            SIZEPROGREGEXP=r"^(\d+)\s+(\d+)\s+\d+\s",
            SIZEDATAREGEXP=r"^\d+\s+(\d+)\s+(\d+)\s+\d+",
        )

    def _get_size_output():
        cmd = env.get("SIZECHECKCMD")
        if not cmd:
            return None
        if not isinstance(cmd, list):
            cmd = cmd.split()
        cmd = [arg.replace("$SOURCES", str(source[0])) for arg in cmd if arg]
        sysenv = os.environ.copy()
        sysenv["PATH"] = str(env["ENV"]["PATH"])
        result = exec_command(env.subst(cmd), env=sysenv)
        if result["returncode"] != 0:
            return None
        return result["out"].strip()

    def _calculate_size(output, pattern):
        if not output or not pattern:
            return -1
        size = 0
        regexp = re.compile(pattern)
        for line in output.split("\n"):
            line = line.strip()
            if not line:
                continue
            match = regexp.search(line)
            if not match:
                continue
            size += sum(int(value) for value in match.groups())
        return size

    def oversized(ref_element, flash):
        # Bin ich ein Container
        if (ref_element['container']):
            return False

        for element in flash:
            # Darf es nicht selber sein
            if (ref_element == element):
                continue

            # Liegt das Element vor mir
            if(element['end'] <= ref_element['start']):
                continue

            # Liegt das Element hinter mir
            if(element['start'] >= ref_element['end']):
                continue

            # Passe ich in das Element rein
            if(element['container'] and element['start'] <= ref_element['start'] and element['end'] >= ref_element['end']):
                continue

            return True

        return False

    def buid_tree(start, end, flash, indent = 0, stack = []):
        prev = start
        empty = True

        for i, element in enumerate(flash):
            if (element in stack):
                continue

            if (element['start'] >= start and element['start'] < end):
                if (prev != element['start'] and element['start'] > prev):
                    print_entry(console_color.GREEN, { 'name': 'FREE', 'start': prev, 'end': element['start']}, indent)

                stack.append(element)
                prev = element['end']
                empty = False

                color = console_color.CYAN
                if (element['container']):
                    color = console_color.BLUE

                # Oversize
                if (oversized(element, flash)):
                    color = console_color.RED

                print_entry(color, element, indent)

                if (element['container']):
                    buid_tree(element['start'], element['end'], flash, indent+1, stack)

        if (not empty and prev < end):
            print_entry(console_color.GREEN, { 'name': 'FREE', 'start': prev, 'end': end}, indent)

    def build_entry(element, indent = 0):
        return (
            "{}{}{} - {} ({} Bytes)".format(
                "".rjust(indent*2, " "),
                (element['name'] + ":").ljust(20-indent*2, ' '),
                format(element['start'], "#010x"),
                format(element['end'], "#010x"),
                element['end']-element['start']
            )
        )

    def print_entry(color, element, indent = 0):
        print("{}{}{}".format(color, build_entry(element, indent), console_color.END))


    if not env.get("SIZECHECKCMD") and not env.get("SIZEPROGREGEXP"):
        _configure_defaults()

    # print(projenv.Dictionary())

    output = _get_size_output()

    flash_elements = []

    flash_start = 0
    system_start = 0
    firmware_start = 0
    flash_end = 0

    firmware_end = _calculate_size(output, env.get("SIZEPROGREGEXP")) 

    if projenv['BOARD'] == 'pico':
        eeprom_start = env["PICO_EEPROM_START"] - 268435456
        flash_end = eeprom_start + 4096

        if env["FS_START"] > 0:
            filesystem_start = env["FS_START"] - 268435456
            filesystem_end = env["FS_END"] - 268435456
            system_end = filesystem_start # overwrite new system end
            flash_elements.append({ 'name': 'FILESYSTEM', 'start': filesystem_start, 'end': filesystem_end, 'container': False })
        
    if projenv['PIOPLATFORM'] == 'atmelsam':
        flash_end = 0x40000
        eeprom_start = flash_end - 256
        system_end = eeprom_start

    eeprom_end = flash_end

    flash_elements.append({ 'name': 'FLASH',      'start': flash_start, 'end': flash_end, 'container': True })
    flash_elements.append({ 'name': 'FIRMWARE',   'start': firmware_start, 'end': firmware_end, 'container': False })
    flash_elements.append({ 'name': 'EEPROM',     'start': eeprom_start, 'end': eeprom_end, 'container': projenv['PIOPLATFORM'] == 'atmelsam' }) # Container because, when EEPROM not use you can use by other
    flash_elements.append({ 'name': 'SYSTEM',     'start': system_start, 'end': system_end, 'container': True })

    defined_sizes = {}
    for x in projenv["CPPDEFINES"]:
        if type(x) is tuple:
            name = x[0]
            if (x[0].endswith("FLASH_OFFSET") or x[0].endswith("FLASH_SIZE")):
                name = name.replace("_FLASH_OFFSET", "")
                name = name.replace("_FLASH_SIZE", "")
                if(not name in defined_sizes):
                    defined_sizes[name] = { 'offset': 0, 'size': 0 }
                
                if(x[0].endswith("FLASH_OFFSET")):
                    defined_sizes[name]['offset'] = int(x[1], 16)

                if(x[0].endswith("_FLASH_SIZE")):
                    defined_sizes[name]['size'] = int(x[1], 16)

    # hack for knx stack and samd - https://github.com/thelsing/knx/blob/master/src/samd_platform.cpp#L94
    if projenv['PIOPLATFORM'] == 'atmelsam':
        defined_sizes['KNX']['offset'] = system_end - defined_sizes['KNX']['size']

    for name, data in defined_sizes.items():
        if data['offset'] > 0 and data['size'] > 0:
            flash_elements.append({ 'name': name, 'start': data['offset'], 'end': data['offset'] + data['size'], 'container': False })


    sorted_flash_elements = sorted(flash_elements, key=lambda element: (element['start'], -element['end']-element['start']))
    print("")
    stack = []
    print("{}Show flash partitioning:{}".format(console_color.YELLOW, console_color.END))
    buid_tree(flash_start, flash_end, sorted_flash_elements, 1, stack)
    print("")

env.AddPostAction("checkprogsize", show_flash_partitioning)