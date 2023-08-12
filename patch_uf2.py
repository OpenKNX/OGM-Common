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

def post_program_action(source, target, env):
    print()
    print("{}Patching {}.u2f for KnxUpdater:{}".format(console_color.YELLOW, source[0].get_path()[0:-4], console_color.END))
    content = open("include/knxprod.h", 'r').read()

    m = re.search("#define MAIN_OpenKnxId (0x)?([0-9A-Fa-f]{1,2})", content)
    if m is None:
        print("{}  {}{}".format(console_color.RED, "Error: OpenKnxId not readable", console_color.END))
        return
    elif m.group(1) == "0x":
        openknxid = int(m.group(2), 16)
    else:
        openknxid = int(m.group(2))

    m = re.search("#define MAIN_ApplicationNumber (0x)?([0-9A-Fa-f]{1,3})", content)
    if m is None:
        print("{}  {}{}".format(console_color.RED, "Error: ApplicationNumber not readable", console_color.END))
        return
    elif m.group(1) == "0x":
        application_number = int(m.group(2), 16)
    else:
        application_number = int(m.group(2))

    m = re.search("#define MAIN_ApplicationVersion (0x)?([0-9A-Fa-f]{1,3})", content)
    if m is None:
        print("{}  {}{}".format(console_color.RED, "Error: ApplicationVersion not readable", console_color.END))
        return
    elif m.group(1) == "0x":
        application_version = int(m.group(2), 16)
    else:
        application_version = int(m.group(2))

    content = open(env["PROJECT_SRC_DIR"] + "/main.cpp", 'r').read()

    m = re.search("const uint8_t firmwareRevision = ([0-9]+);", content)
    if m is None:
        print("{}  {}{}".format(console_color.RED, "Error: FirmwareRevision not readable", console_color.END))
        return
    else:
        firmware_revision = int(m.group(1))

    print("{}  OpenKnxId:          0x{} ({}){}".format(console_color.CYAN, format(openknxid, '02X'), openknxid, console_color.END))
    print("{}  ApplicationNumber:  0x{} ({}){}".format(console_color.CYAN, format(application_number, '02X'), application_number, console_color.END))
    print("{}  ApplicationVersion: 0x{} ({}){}".format(console_color.CYAN, format(application_version, '02X'), application_version, console_color.END))
    print("{}  FirmwareRevision:   0x{} ({}){}".format(console_color.CYAN, format(firmware_revision, '02X'), firmware_revision, console_color.END))

    barray = bytearray(open(source[0].get_path()[0:-4] + ".uf2", "rb").read())
    barray[9] = barray[9] | 0x80
    barray[288] = 8 #Tag Size
    barray[289] = 0x4B #Type
    barray[290] = 0x4E #Type
    barray[291] = 0x58 #Type
    barray[292] = openknxid #Data
    barray[293] = application_number #Data
    barray[294] = application_version #Data
    barray[295] = firmware_revision #Data
    open(source[0].get_path()[0:-4] + ".uf2","wb").write(barray)

    print("{}  Patching completed{}".format(console_color.GREEN, console_color.END))
    print()

env.AddPostAction("buildprog", post_program_action)