Import("env")
Import("projenv")
import re
import os
from platformio.proc import exec_command

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


def post_program_action(source, target, env):
    print("Changing ", source[0].get_path()[0:-4] + ".uf2")

    with open(env["PROJECT_LIBDEPS_DIR"] + '/' + env["PIOENV"] + "/OGM-Common/include/knxprod.h", 'r') as knxprod:
        content = knxprod.read(1000)

    m = re.search("#define MAIN_OpenKnxId 0x([0-9A-Fa-f]{1,2})", content)
    openknxid = m.group(1)
    m = re.search("#define MAIN_ApplicationNumber 0x([0-9A-Fa-f]{1,2})", content)
    if m is None:
        m = re.search("#define MAIN_ApplicationNumber ([0-9]{1,3})", content)
    appnumber = m.group(1)
    m = re.search("#define MAIN_ApplicationVersion 0x([0-9A-Fa-f]{1,2})", content)
    if m is None:
        m = re.search("#define MAIN_ApplicationVersion ([0-9]{1,3})", content)
    appversion = m.group(1)
    
    with open(env["PROJECT_SRC_DIR"] + "\\main.cpp", 'r') as knxprod:
        content = knxprod.read(-1)

    m = re.search("firmwareRevision = ([0-9]+);", content)
    apprevision = m.group(1)

    with open(source[0].get_path()[0:-4] + ".uf2", "rb") as orig_file:
        barray=bytearray(orig_file.read())
    barray[9] = barray[9] | 0x80
    barray[288] = 8 #Tag Size
    barray[289] = 0x4B #Type
    barray[290] = 0x4E #Type
    barray[291] = 0x58 #Type
    barray[292] = int(openknxid, 16) #Data
    barray[293] = int(appnumber, 16) #Data
    barray[294] = int(appversion, 16) #Data
    barray[295] = int(apprevision) #Data
    with open(source[0].get_path()[0:-4] + ".uf2","wb") as file: 
        file.write(barray)

def post_progsize(source, target, env):
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

    if not env.get("SIZECHECKCMD") and not env.get("SIZEPROGREGEXP"):
        _configure_defaults()
    output = _get_size_output()
    program_size = _calculate_size(output, env.get("SIZEPROGREGEXP"))

    flash_start = 0 # relative to zero   268435456 #0x10000000 got from uf2
    flash_end = flash_start + int(projenv["PICO_FLASH_LENGTH"])
    openknx_start = -1

    for x in projenv["CPPDEFINES"]:
        if type(x) is tuple:
            if x[0] == "KNX_FLASH_OFFSET":
                knx_start = int(x[1], 16)
            if x[0] == "KNX_FLASH_SIZE":
                knx_end = int(x[1], 16)
            if x[0] == "OPENKNX_FLASH_OFFSET":
                openknx_start = int(x[1], 16)
            if x[0] == "OPENKNX_FLASH_SIZE":
                openknx_end = int(x[1], 16)

    knx_start = flash_start + knx_start   
    knx_end = knx_start + knx_end
    if openknx_start != -1:
        openknx_start = flash_start + openknx_start   
        openknx_end = openknx_start + openknx_end

    print("Flash Usage")
    print("  - System      0x0 - " + hex(flash_end) + " (" + str(flash_end - flash_start) + " Bytes)")
    print("    - Firmware  0x0 - " + hex(program_size) + " (" + str(program_size) + " Bytes)")

    if knx_start - (flash_start + program_size) < 0:
        print(bcolors.FAIL + "    - Overlap   " + str(knx_start - (flash_start + program_size)) + " Bytes" + bcolors.ENDC)
    elif knx_start - (flash_start + program_size) > 0:
        print(bcolors.OKGREEN + "    - Free      " + str(knx_start - (flash_start + program_size)) + " Bytes" + bcolors.ENDC)

    print("    - KNX       " + hex(knx_start) + " - " + hex(knx_end) + " (" + str(knx_end - knx_start) + " Bytes)")

    if openknx_start != -1:
        if openknx_start - knx_end < 0:
            print(bcolors.FAIL + "    - Overlap   " + str(openknx_start - knx_end) + " Bytes" + bcolors.ENDC)
        elif openknx_start - knx_end > 0:
            print(bcolors.OKGREEN + "    - Free      " + str(openknx_start - knx_end) + " Bytes" + bcolors.ENDC)

        print("    - OpenKNX   " + hex(openknx_start) + " - " + hex(openknx_end) + " (" + str(openknx_end - openknx_start) + " Bytes)")

        if flash_end - openknx_end < 0:
            print(bcolors.FAIL + "    - Overlap   " + str(flash_end - openknx_end) + " Bytes" + bcolors.ENDC)
        elif flash_end - openknx_end:
            print(bcolors.OKGREEN + "    - Free      " + str(flash_end - openknx_end) + " Bytes" + bcolors.ENDC)
    else:
        if flash_end - knx_end < 0:
            print(bcolors.FAIL + "    - Overlap   " + str(flash_end - knx_end) + " Bytes" + bcolors.ENDC)
        elif flash_end - knx_end > 0:
            print(bcolors.OKGREEN + "    - Free      " + str(flash_end - knx_end) + " Bytes" + bcolors.ENDC)

    filesys_start = env["FS_START"] - 268435456
    filesys_end = env["FS_END"] - 268435456
    if filesys_end - filesys_start == 0:
        print("  - Filesystem  None")
    else:
        print("  - Filesystem  " + hex(filesys_start) + " - " + hex(filesys_end) + " (" + str(filesys_end - filesys_start) + ")")

    eeprom_start = env["PICO_EEPROM_START"] - 268435456
    print("  - EEPROM      " + hex(eeprom_start) + " - " + hex(eeprom_start + 4096) + " (4096 Bytes)")




env.AddPostAction("buildprog", post_program_action)
env.AddPostAction("checkprogsize", post_progsize)