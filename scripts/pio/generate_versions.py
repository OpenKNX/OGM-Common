#
# generate_versions.py — Lightweight versions.h generator
#
# Generates include/versions.h by scanning lib/ for OpenKNX modules
# (OGM-*, OFM-*) and the knx library.  Reads version numbers from
# each library's library.json and appends the short git hash.
#
# Unlike prepare.py this script does NOT use PlatformIO's library
# resolver (ProjectAsLibBuilder).  It only needs filesystem access
# and git, so it works for both normal Arduino builds AND custom-IDF
# builds where the library resolver is not available.
#
# When used together with prepare.py (normal Arduino envs), prepare.py
# runs AFTER this script and overwrites versions.h with the same data
# obtained from the full library resolver — no harm done.
#
# For IDF envs (where prepare.py is NOT in the extra_scripts list),
# this script is the sole source of versions.h.
#

Import("env")
import json
import os
import pathlib
import re
import subprocess

class console_color:
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    END = '\033[0m'


def get_git_version(path):
    """Return short git hash for *path*, or None on failure."""
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            capture_output=True, text=True, cwd=path, check=True,
        )
        return result.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None


def read_library_version(lib_path):
    """Read 'version' from library.json, return string or None."""
    json_path = os.path.join(lib_path, "library.json")
    if not os.path.isfile(json_path):
        return None
    try:
        with open(json_path, "r") as f:
            data = json.load(f)
        return data.get("version")
    except (json.JSONDecodeError, OSError):
        return None


def get_ets_version(version_string):
    """Encode major.minor into a single byte for ETS ApplicationVersion check."""
    m = re.match(r"^(\d+)\.(\d+)\.(\d+)(\+.*)*$", version_string)
    if m:
        major = int(m.group(1))
        minor = int(m.group(2))
        if major > 15 or minor > 15:
            return None
        return (major << 4) + minor
    return None


# ---------------------------------------------------------------------------
# Determine lib path (local lib/ or .pio/libdeps/<env>)
# ---------------------------------------------------------------------------
base_dir = pathlib.Path().resolve()

if os.path.isdir("lib"):
    basepath = "lib"
else:
    basepath = os.path.join(".pio", "libdeps", env["PIOENV"])

lib_dir = base_dir / basepath

# ---------------------------------------------------------------------------
# Discover OpenKNX modules (OGM-*, OFM-*) and knx
# ---------------------------------------------------------------------------
openknx_modules = {}  # name -> "version+githash"
knx_version = None

if os.path.isdir(lib_dir):
    for entry in sorted(os.listdir(lib_dir)):
        full = lib_dir / entry
        if not os.path.isdir(full):
            continue

        ver = read_library_version(full)
        git = get_git_version(full)

        if entry == "knx":
            knx_version = ver
            if git:
                knx_version = "{}+{}".format(ver, git) if ver else git
            continue

        if not (entry.startswith("OGM") or entry.startswith("OFM")):
            continue

        if ver and git:
            openknx_modules[entry] = "{}+{}".format(ver, git)
        elif ver:
            openknx_modules[entry] = ver
        elif git:
            openknx_modules[entry] = git

# ---------------------------------------------------------------------------
# Write include/versions.h
# ---------------------------------------------------------------------------
main_git = get_git_version(base_dir)

os.makedirs("include", exist_ok=True)
with open("include/versions.h", "w") as vf:
    vf.write("#pragma once\n\n")
    vf.write('#define MAIN_Version "{}"\n'.format(main_git or "unknown"))

    if knx_version:
        vf.write('#define KNX_Version "{}"\n'.format(knx_version))

    for name, version in openknx_modules.items():
        define_name = "MODULE_" + name.split("-")[1]
        vf.write('#define {} "{}"\n'.format(define_name + "_Version", version))

        m = re.match(r"^(\d+)\.(\d+)\.(\d+)(\D.*)?$", version)
        if m:
            vf.write("#define {}_Version_Major {}\n".format(define_name, int(m.group(1))))
            vf.write("#define {}_Version_Minor {}\n".format(define_name, int(m.group(2))))
            vf.write("#define {}_Version_Revision {}\n".format(define_name, int(m.group(3))))

        ets = get_ets_version(version)
        if ets is not None:
            vf.write("#define {} {}\n".format(define_name + "_ETS", ets))

# ---------------------------------------------------------------------------
# Console output
# ---------------------------------------------------------------------------
print()
print("{}generate_versions.py — OpenKNX Module versions:{}".format(
    console_color.YELLOW, console_color.END))

for name, version in openknx_modules.items():
    define_name = "MODULE_" + name.split("-")[1]
    print("{}  {}: {} ({}){}".format(console_color.CYAN, define_name, version, name, console_color.END))

print()

# ---------------------------------------------------------------------------
# Cleanup legacy files (same as prepare.py did)
# ---------------------------------------------------------------------------
for legacy in [
    "lib/OGM-Common/include/knxprod.h",
    "lib/OGM-Common/include/versions.h",
    "lib/OGM-Common/include/hardware.h",
]:
    if os.path.isfile(legacy):
        os.remove(legacy)
