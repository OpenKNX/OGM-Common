Import("env")
Import("projenv")
import os
import re
import sys
from platformio.proc import exec_command

sys.path.insert(0, next((p for p in ("lib/OGM-Common/scripts/pio", "scripts/pio")
                         if os.path.exists(os.path.join(p, "_pio_common.py"))), "."))
from _pio_common import C, section, ok, warn, identity_line, quiet_action


def _define(content, name, width):
    m = re.search(r"#define %s (0x)?([0-9A-Fa-f]{1,%d})" % (name, width), content)
    if m is None:
        return None
    return int(m.group(2), 16 if m.group(1) == "0x" else 10)


def post_program_action(source, target, env):
    base = source[0].get_path()[0:-4]
    uf2source = base + ".uf2"
    elfsource = base + ".elf"

    section("OpenKNX RP2040 identity stamp", "firmware.uf2")

    if not os.path.exists(uf2source):
        print("{}  rebuilding firmware.uf2 from firmware.elf{}".format(C.GRAY, C.END))
        exec_command('picotool uf2 convert -t elf "%s" "%s"' % (elfsource, uf2source), shell=True)

    content = open("include/knxprod.h", "r").read()
    openknxid = _define(content, "MAIN_OpenKnxId", 2)
    appnumber = _define(content, "MAIN_ApplicationNumber", 3)
    appversion = _define(content, "MAIN_ApplicationVersion", 3)

    # Firmware revision from knxprod.h; old style keeps the literal in main.cpp.
    m = re.search(r"#define MAIN_FirmwareRevision (\d{1,2})", content)
    if m is None:
        m = re.search(r"const uint8_t firmwareRevision = ([0-9]+);",
                      open(env["PROJECT_SRC_DIR"] + "/main.cpp", "r").read())
    revision = int(m.group(1)) if m else None

    if None in (openknxid, appnumber, appversion, revision):
        warn("identity not readable from knxprod.h", "UF2 left unstamped")
        return

    print(identity_line(openknxid, appnumber, appversion, revision, "tagged into the UF2 extension"))

    barray = bytearray(open(uf2source, "rb").read())
    barray[9] = barray[9] | 0x80
    barray[288] = 8       # tag size
    barray[289] = 0x4B    # 'K'
    barray[290] = 0x4E    # 'N'
    barray[291] = 0x58    # 'X'
    barray[292] = openknxid
    barray[293] = appnumber
    barray[294] = appversion
    barray[295] = revision
    open(uf2source, "wb").write(barray)

    ok("firmware.uf2", "KNX identity tag written")


env.AddPostAction("buildprog", quiet_action(post_program_action))
