# ---------------------------------------------------------------------------
#  Pre-build: generate include/versions.h from the resolved library tree.
#
#  Load-time step (runs before the compile). buildtime.h and webassets.h are
#  separate scripts (prepare_buildtime.py / prepare_webassets.py) so each header
#  regenerates only on its own trigger. Shared style/resolver in _pio_common.py.
# ---------------------------------------------------------------------------

Import("env")
import os
import sys
import re
import json
import pathlib
import subprocess

sys.path.insert(0, next((p for p in ("lib/OGM-Common/scripts/pio", "scripts/pio")
                         if os.path.exists(os.path.join(p, "_pio_common.py"))), "."))
from _pio_common import C, step, resolve_project

from platformio.builder.tools.piolib import PackageItem

print()
print("{}Build Versions{}".format(C.BOLD, C.END))

project, lib_builders, basepath = resolve_project(env)

# A flat map {library name: version}; the tree structure is not kept, only names + versions.
library_versions = dict()


def get_all_library_dependencies(root):
    for lb in root.depbuilders:
        pkg = PackageItem(lb.path)
        library_versions[str(lb.name)] = str(pkg.metadata.version if pkg.metadata else lb.version)
        if lb.depbuilders:
            get_all_library_dependencies(lb)


def add_local_lib_folder_versions(all_lib_builders):
    local_lib_dir = (pathlib.Path().resolve() / "lib").resolve()
    for lb in all_lib_builders:
        try:
            lib_path = pathlib.Path(lb.path).resolve()
        except OSError:
            continue
        # Only libraries that come from the project's local lib/ folder.
        try:
            lib_path.relative_to(local_lib_dir)
        except ValueError:
            continue
        pkg = PackageItem(lb.path)
        library_versions[str(lb.name)] = str(pkg.metadata.version if pkg.metadata else lb.version)


def add_all_local_lib_dirs():
    local_lib_dir = (pathlib.Path().resolve() / "lib").resolve()
    if not local_lib_dir.exists():
        return
    for lib_dir in local_lib_dir.iterdir():
        if not lib_dir.is_dir() or lib_dir.name.startswith("."):
            continue
        lib_name = lib_dir.name
        lib_version = None
        # Prefer library.json metadata when present.
        library_json = lib_dir / "library.json"
        if library_json.exists():
            try:
                with open(library_json, "r") as handle:
                    manifest = json.load(handle)
                if manifest.get("name"):
                    lib_name = manifest.get("name")
                if manifest.get("version"):
                    lib_version = manifest.get("version")
            except (json.JSONDecodeError, OSError):
                pass
        if lib_version is None:
            pkg = PackageItem(str(lib_dir))
            if pkg.metadata and pkg.metadata.version:
                lib_version = pkg.metadata.version
        library_versions[str(lib_name)] = str(lib_version)


def get_git_version(path):
    try:
        result = subprocess.run(["git", "rev-parse", "--short", "HEAD"],
                                capture_output=True, text=True, cwd=path, check=True)
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return False


def get_ets_version(version_string):
    result = re.match(r"^(\d+)\.(\d+)\.(\d+)(\+.*)*$", version_string)
    if result:
        major = int(result.group(1))
        minor = int(result.group(2))
        if major > 15 or minor > 15:
            return None
        return (major << 4) + minor
    return None


get_all_library_dependencies(project)
add_local_lib_folder_versions(lib_builders)
add_all_local_lib_dirs()

step("Generate include/versions.h")

openknx_modules = {k: v for k, v in library_versions.items() if k.startswith("OGM") or k.startswith("OFM")}

# append the git short hash to each module version
base_dir = pathlib.Path().resolve()
for name, lib_version in openknx_modules.items():
    try:
        git_version = get_git_version(base_dir / basepath / name)
        if git_version is not None:
            if lib_version is not None and lib_version != "None":
                openknx_modules[name] = lib_version.split("+")[0] + "+" + git_version
            else:
                openknx_modules[name] = git_version
    except NotADirectoryError:
        pass

version_lines = ["#pragma once\n\n"]
version_lines.append("#define MAIN_Version \"{}\"\n".format(get_git_version(base_dir)))
version_lines.append("#define KNX_Version \"{}\"\n".format(
    library_versions["knx"] + "+" + get_git_version(base_dir / basepath / "knx")))
for name, version in openknx_modules.items():
    define_name = "MODULE_" + name.split("-")[1]
    version_lines.append("#define {} \"{}\"\n".format(define_name + "_Version", version))
    result = re.match(r"^(\d+)\.(\d+)\.(\d+)(\D.*)?$", version)
    if result:
        version_lines.append("#define {}_Version_Major {}\n".format(define_name, int(result.group(1))))
        version_lines.append("#define {}_Version_Minor {}\n".format(define_name, int(result.group(2))))
        version_lines.append("#define {}_Version_Revision {}\n".format(define_name, int(result.group(3))))
    ets = get_ets_version(version)
    if ets is not None:
        version_lines.append("#define {} {}\n".format(define_name + "_ETS", ets))
    print("{}  {}: {} ({}){}".format(C.CYAN, define_name, version, name, C.END))

# versions.h carries only the module version defines, so it changes only when a version actually
# changes -- no build timestamp here (that is buildtime.h, generated by prepare_buildtime.py).
with open("include/versions.h", "w") as version_file:
    version_file.write("".join(version_lines))

# Drop stale generated headers from the OGM-Common lib symlink so a rebuild cannot pick up old copies.
for stale in ("knxprod.h", "versions.h", "hardware.h"):
    path = "lib/OGM-Common/include/" + stale
    if os.path.isfile(path):
        os.remove(path)
