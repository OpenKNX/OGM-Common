Import("env")
Import("projenv")
import re
import os
from platformio.proc import exec_command

def post_program_action(source, target, env):
    print("Changing ", source[0].get_path()[0:-4] + ".uf2")

    with open("lib/OGM-Common/include/knxprod.h", 'r') as knxprod:
        content = knxprod.read(1000)

    m = re.search("#define MAIN_OpenKnxId 0x([0-9A-Fa-f]{1,2})", content)
    openknxid = int(m.group(1), 16)
    #appnumber = 0
    #appversion = 0
    m = re.search("#define MAIN_ApplicationNumber 0x([0-9A-Fa-f]{1,2})", content)
    if m is None:
        m = re.search("#define MAIN_ApplicationNumber ([0-9]{1,3})", content)
        appnumber = int(m.group(1))
    else:
        appnumber = int(m.group(1), 16)

    m = re.search("#define MAIN_ApplicationVersion 0x([0-9A-Fa-f]{1,2})", content)
    if m is None:
        m = re.search("#define MAIN_ApplicationVersion ([0-9]{1,3})", content)
        appversion = int(m.group(1))
    else:
        appversion = int(m.group(1), 16)
    
    with open(env["PROJECT_SRC_DIR"] + "\\main.cpp", 'r') as knxprod:
        content = knxprod.read(-1)

    m = re.search("firmwareRevision = ([0-9]+);", content)
    apprevision = int(m.group(1))

    with open(source[0].get_path()[0:-4] + ".uf2", "rb") as orig_file:
        barray=bytearray(orig_file.read())
    barray[9] = barray[9] | 0x80
    barray[288] = 8 #Tag Size
    barray[289] = 0x4B #Type
    barray[290] = 0x4E #Type
    barray[291] = 0x58 #Type
    barray[292] = openknxid #Data
    barray[293] = appnumber #Data
    barray[294] = appversion #Data
    barray[295] = apprevision #Data
    with open(source[0].get_path()[0:-4] + ".uf2","wb") as file: 
        file.write(barray)

env.AddPostAction("buildprog", post_program_action)