# ---------------------------------------------------------------------------
#  Pre-build: generate include/buildtime.h (BUILD_DATETIME / BUILD_TIMESTAMP).
#
#  Kept separate from versions.h: these change on EVERY build, and versions.h is
#  pulled in via defines.h by nearly every source file -- bundling them there
#  would force a full rebuild even when no module version changed.
# ---------------------------------------------------------------------------

import os
import sys
import datetime

sys.path.insert(0, next((p for p in ("lib/OGM-Common/scripts/pio", "scripts/pio")
                         if os.path.exists(os.path.join(p, "_pio_common.py"))), "."))
from _pio_common import C, step

step("Generate include/buildtime.h")

now = datetime.datetime.now()
build_datetime = now.strftime("%Y-%m-%d %H:%M:%S")
with open("include/buildtime.h", "w") as build_file:
    build_file.write("#pragma once\n\n")
    build_file.write("#define BUILD_DATETIME \"{}\"\n".format(build_datetime))
    build_file.write("#define BUILD_TIMESTAMP {}\n".format(int(now.timestamp())))

print("{}  Build: {}{}".format(C.GRAY, build_datetime, C.END))
