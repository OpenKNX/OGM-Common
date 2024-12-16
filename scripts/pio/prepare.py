from platformio.builder.tools.piolib import ProjectAsLibBuilder, PackageItem, LibBuilderBase
from SCons.Script import ARGUMENTS  
from shlex import quote
Import("env", "projenv")
import pathlib
import os
import subprocess
import re

class console_color:
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    END = '\033[0m'

# from https://github.com/platformio/platformio-core/blob/develop/platformio/builder/tools/piolib.py
def _correct_found_libs(lib_builders):
    # build full dependency graph
    found_lbs = [lb for lb in lib_builders if lb.dependent]
    for lb in lib_builders:
        if lb in found_lbs:
            lb.search_deps_recursive(lb.get_search_files())
    for lb in lib_builders:
        for deplb in lb.depbuilders[:]:
            if deplb not in found_lbs:
                lb.depbuilders.remove(deplb)

print("Build Versions")

project = ProjectAsLibBuilder(env, "$PROJECT_DIR")

basepath = ""
if os.path.exists("lib/"):
  basepath = "lib"
else:
  basepath = ".pio/libdeps/" + env["PIOENV"]

# print(str(source[0]))
# print(env.Dictionary())
# print(projenv.Dictionary())


# rescan dependencies just like in py file above. otherwise dependenceis are empty
ldf_mode = LibBuilderBase.lib_ldf_mode.fget(project)
lib_builders = env.GetLibBuilders()
project.search_deps_recursive()
if ldf_mode.startswith("chain") and project.depbuilders:
    _correct_found_libs(lib_builders)

# for debugging
def _print_deps_tree(root, level=0):
    margin = "|   " * (level)
    for lb in root.depbuilders:
        title = "<%s>" % lb.name
        pkg = PackageItem(lb.path)
        if pkg.metadata:
            title += " %s" % pkg.metadata.version
        elif lb.version:
            title += " %s" % lb.version
        print("%s|-- %s" % (margin, title), end="")
        if int(ARGUMENTS.get("PIOVERBOSE", 0)):
            if pkg.metadata and pkg.metadata.spec.external:
                print(" [%s]" % pkg.metadata.spec.url, end="")
            print(" (", end="")
            print(lb.path, end="")
            print(")", end="")
        print("")
        if lb.depbuilders:
            _print_deps_tree(lb, level + 1)

# create a map of all used libraries and their version.
# the structure of the tree is not captured, just library names and versions. 
library_versions = dict()
def get_all_library_dependencies(root, level=0):
    global library_versions
    for lb in root.depbuilders:
        pkg = PackageItem(lb.path)
        lib_name = lb.name
        lib_version = pkg.metadata.version if pkg.metadata else lb.version
        library_versions[str(lib_name)] = str(lib_version)
        if lb.depbuilders:
            get_all_library_dependencies(lb, level + 1)

# print("PRINTING DEP TREE")
# _print_deps_tree(project)

def get_git_version(path):
  try:
    result = subprocess.run(["git", "rev-parse", "--short", "HEAD"], capture_output=True, text=True, cwd=path, check=True)
    return result.stdout.strip()
  except subprocess.CalledProcessError:
    return False


def get_ets_version(version_string):
  result = re.match(r"^(\d+)\.(\d+)\.(\d+)(\+.*)*$", version_string)
  if result:
    major =  int(result.group(1))
    minor =  int(result.group(2))

    if major > 15: return None
    if minor > 15: return None

    return (major << 4) + minor
  else:
    return None


get_all_library_dependencies(project)

print()
print("{}Read OpenKNX Module version and build defines:{}".format(console_color.YELLOW, console_color.END))

openknx_modules = {k: v for k, v in library_versions.items() if k.startswith("OGM") or k.startswith("OFM")}
# openknx_modules["nodir"] = None # to test missing directory

# get git versions
base_dir = pathlib.Path().resolve()
for name, lib_version in openknx_modules.items():
  try:
    git_version = get_git_version(base_dir / basepath / name)
    if git_version != None:
      if lib_version != None and lib_version != "None":
        openknx_modules[name] = lib_version.split("+")[0] + "+" + git_version
      else:
        openknx_modules[name] = git_version
  except NotADirectoryError:
    pass

# build defines
version_file = open("include/versions.h", "w")
version_file.write("#pragma once\n\n")
version_file.write("#define MAIN_Version \"{}\"\n".format(get_git_version(base_dir)))
version_file.write("#define KNX_Version \"{}\"\n".format(library_versions["knx"] + "+" + get_git_version(base_dir / basepath / "knx")))
# additional_defines = dict()
for name, version in openknx_modules.items():
  define_name = "MODULE_" + name.split("-")[1]
  version_file.write("#define {} \"{}\"\n".format(define_name + "_Version", version))

  ets = get_ets_version(version)
  if ets != None:
    version_file.write("#define {} {}\n".format(define_name + "_ETS", ets))
  
  print("{}  {}: {} ({}){}".format(console_color.CYAN, define_name, version, name, console_color.END))

version_file.close()
print()

# delete old file
if os.path.isfile("lib/OGM-Common/include/knxprod.h"):
    os.remove("lib/OGM-Common/include/knxprod.h")

if os.path.isfile("lib/OGM-Common/include/versions.h"):
    os.remove("lib/OGM-Common/include/versions.h")

if os.path.isfile("lib/OGM-Common/include/hardware.h"):
    os.remove("lib/OGM-Common/include/hardware.h")

# def make_macro_name(lib_name):
#     lib_name = lib_name.upper()
#     lib_name = lib_name.replace(" ", "_")
#     return lib_name

# # also add all individual library versions
# for lib, version in library_versions.items():
#     projenv.Append(CPPDEFINES=[
#      ("LIB_VERSION_%s" % make_macro_name(lib) , "\\\"" + version + "\\\"")
#     ])
#     print("LIB_VERSION_%s = %s" % (make_macro_name(lib), version))

# print(env.Dump())
# print(projenv.Dump())

#env.AddPreAction(target, callback)