from platformio.builder.tools.piolib import ProjectAsLibBuilder, PackageItem, LibBuilderBase
from SCons.Script import ARGUMENTS  
from shlex import quote
Import("env", "projenv")
import pathlib
import os
import subprocess
import re
import datetime
import json
import hashlib
import gzip
import shutil
from SCons.Script import Action

class console_color:
    BLUE = '\033[94m'
    DARKBLUE = '\033[34m'  # normales ANSI-Blau, dunkler als BLUE (94)
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

def add_local_lib_folder_versions(all_lib_builders):
  global library_versions
  local_lib_dir = (pathlib.Path().resolve() / "lib").resolve()

  for lb in all_lib_builders:
    try:
      lib_path = pathlib.Path(lb.path).resolve()
    except OSError:
      continue

    # Include every library that comes from the project's local lib folder.
    try:
      lib_path.relative_to(local_lib_dir)
    except ValueError:
      continue

    pkg = PackageItem(lb.path)
    lib_name = lb.name
    lib_version = pkg.metadata.version if pkg.metadata else lb.version
    library_versions[str(lib_name)] = str(lib_version)

def add_all_local_lib_dirs():
  global library_versions
  local_lib_dir = (pathlib.Path().resolve() / "lib").resolve()

  if not local_lib_dir.exists():
    return

  for lib_dir in local_lib_dir.iterdir():
    if not lib_dir.is_dir() or lib_dir.name.startswith("."):
      continue

    lib_name = lib_dir.name
    lib_version = None

    # Prefer metadata from library.json if available.
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
add_local_lib_folder_versions(lib_builders)
add_all_local_lib_dirs()

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

version_lines = []
version_lines.append("#pragma once\n\n")
version_lines.append("#define MAIN_Version \"{}\"\n".format(get_git_version(base_dir)))
version_lines.append("#define KNX_Version \"{}\"\n".format(library_versions["knx"] + "+" + get_git_version(base_dir / basepath / "knx")))
# additional_defines = dict()
for name, version in openknx_modules.items():
  define_name = "MODULE_" + name.split("-")[1]
  version_lines.append("#define {} \"{}\"\n".format(define_name + "_Version", version))
  result = re.match(r"^(\d+)\.(\d+)\.(\d+)(\D.*)?$", version)
  if result:
    version_lines.append("#define {}_Version_Major {}\n".format(define_name, int(result.group(1))))
    version_lines.append("#define {}_Version_Minor {}\n".format(define_name, int(result.group(2))))
    version_lines.append("#define {}_Version_Revision {}\n".format(define_name, int(result.group(3))))

  ets = get_ets_version(version)
  if ets != None:
    version_lines.append("#define {} {}\n".format(define_name + "_ETS", ets))

  print("{}  {}: {} ({}){}".format(console_color.CYAN, define_name, version, name, console_color.END))

stable_content = "".join(version_lines)

try:
  with open("include/versions.h", "r") as f:
    old_content = f.read()
except (OSError, IOError):
  old_content = ""

old_stable = ""
old_datetime = None
old_timestamp = None
for line in old_content.splitlines(keepends=True):
  m_dt = re.match(r"^#define BUILD_DATETIME \"(.*)\"\s*$", line)
  m_ts = re.match(r"^#define BUILD_TIMESTAMP (\d+)\s*$", line)
  if m_dt:
    old_datetime = m_dt.group(1)
  elif m_ts:
    old_timestamp = m_ts.group(1)
  else:
    old_stable += line

if old_datetime is not None and old_timestamp is not None and old_stable == stable_content:
  build_datetime = old_datetime
  build_timestamp = old_timestamp
else:
  now = datetime.datetime.now()
  build_datetime = now.strftime("%Y-%m-%d %H:%M:%S")
  build_timestamp = int(now.timestamp())

new_content = stable_content \
  + "#define BUILD_DATETIME \"{}\"\n".format(build_datetime) \
  + "#define BUILD_TIMESTAMP {}\n".format(build_timestamp)

if new_content != old_content:
  with open("include/versions.h", "w") as version_file:
    version_file.write(new_content)

print("{}  Build: {}{}".format(console_color.CYAN, build_datetime, console_color.END))
print()

# delete old file
if os.path.isfile("lib/OGM-Common/include/knxprod.h"):
    os.remove("lib/OGM-Common/include/knxprod.h")

if os.path.isfile("lib/OGM-Common/include/versions.h"):
    os.remove("lib/OGM-Common/include/versions.h")

if os.path.isfile("lib/OGM-Common/include/hardware.h"):
    os.remove("lib/OGM-Common/include/hardware.h")


# ── Web assets ────────────────────────────────────────────────────────────────
# web/assets/ (CSS/JS/SVG/images) from every built module + the project -> minified,
# gzip-compressed and embedded into include/webassets.h. Only built when a webserver is
# present (OPENKNX_WEBSERVER). A post-link nm report shows what actually shipped.
# See OFM-Network/README.md.
WEBASSET_MIME = {".css": "text/css", ".js": "text/javascript", ".svg": "image/svg+xml",
                 ".jpg": "image/jpeg", ".jpeg": "image/jpeg", ".png": "image/png"}
WEBASSET_FORMAT = "4"
WEBASSET_HEADER = pathlib.Path("include/webassets.h")


def _webasset_ident(rel):
    i = re.sub(r"[^0-9a-zA-Z_]", "_", rel)
    return "_" + i if i[:1].isdigit() else i


def _webasset_min_css(t):
    t = re.sub(r"/\*.*?\*/", "", t, flags=re.DOTALL)
    t = re.sub(r"\s+", " ", t)
    t = re.sub(r"\s*([{}:;,])\s*", r"\1", t)
    return re.sub(r";}", "}", t).strip()


def _webasset_min_js(t):
    return "\n".join(s for s in (ln.strip() for ln in t.splitlines()) if s)


def _webasset_min_svg(t):
    t = re.sub(r"<!--.*?-->", "", t, flags=re.DOTALL)
    return re.sub(r">\s+<", "><", t).strip()


WEBASSET_MINIFY = {".css": _webasset_min_css, ".js": _webasset_min_js, ".svg": _webasset_min_svg}


def _webasset_collect():
    # Precise: walk the actual dependency tree (only modules really in the build) + the project.
    roots = []
    def walk(node):
        for lb in node.depbuilders:
            roots.append(pathlib.Path(lb.path))
            if lb.depbuilders:
                walk(lb)
    walk(project)
    roots.append(pathlib.Path(env.subst("$PROJECT_DIR")))

    seen, entries = set(), {}
    for base in roots:
        d = (base / "web" / "assets").resolve()
        if not d.is_dir() or d in seen:
            continue
        seen.add(d)
        for f in sorted(d.rglob("*")):
            ext = f.suffix.lower()
            if not f.is_file() or ext not in WEBASSET_MIME:
                continue
            ident = _webasset_ident(f.relative_to(d).as_posix())
            if ident in entries:
                raise SystemExit("{}webassets: duplicate identifier '{}': {} vs {}{}".format(
                    console_color.RED, ident, entries[ident], f, console_color.END))
            entries[ident] = (ext, f)
    return entries


def _webasset_generate():
    entries = _webasset_collect()
    if not entries:
        if WEBASSET_HEADER.is_file():
            WEBASSET_HEADER.unlink()
        return

    sig = hashlib.sha256(WEBASSET_FORMAT.encode())
    for ident, (ext, f) in entries.items():
        sig.update("{}\0{}\0".format(ident, ext).encode())
        sig.update(hashlib.sha256(f.read_bytes()).digest())
    marker = "// webassets-sig: " + sig.hexdigest()

    if WEBASSET_HEADER.is_file():
        try:
            if marker in WEBASSET_HEADER.read_text().splitlines()[:2]:
                return
        except OSError:
            pass

    head = ["#pragma once", marker, "// webassets-list:"]
    body = []
    for ident, (ext, f) in entries.items():
        raw = f.read_bytes()
        data = WEBASSET_MINIFY[ext](raw.decode()).encode() if ext in WEBASSET_MINIFY else raw
        gz = gzip.compress(data, compresslevel=9, mtime=0)
        head.append("//   E {} {}".format(ident, len(gz)))
        rows = ["0x{:02x}".format(b) for b in gz]
        arr = "\n".join("    " + ",".join(rows[i:i + 20]) + "," for i in range(0, len(rows), 20))
        body += ["    inline const uint8_t {}_gz[] = {{".format(ident), arr, "    };",
                 '    inline const char* const {}_mime = "{}";'.format(ident, WEBASSET_MIME[ext]), ""]

    head += ["// Auto-generated by OGM-Common/scripts/pio/prepare.py from web/assets/ -- do not edit.",
             "#include <cstddef>", "#include <cstdint>", "", "namespace WebAssets", "{"]
    WEBASSET_HEADER.parent.mkdir(parents=True, exist_ok=True)
    WEBASSET_HEADER.write_text("\n".join(head + body + ["} // namespace WebAssets", ""]))


def _webasset_nm():
    cc = env.subst("$CC")
    for c in (cc + "-nm", re.sub(r"(gcc|g\+\+|cc)$", "nm", cc), "nm"):
        hit = c if (os.path.isabs(c) and os.path.exists(c)) else shutil.which(c)
        if hit:
            return hit
    return None


def _webasset_report(target, source, env):  # SCons calls with keyword args -> names are fixed
    elf = next((str(n) for n in (*target, *source) if str(n).endswith(".elf")), None)
    manifest = {}
    try:
        for line in WEBASSET_HEADER.read_text().splitlines():
            if line.startswith("//   E "):
                ident, gz = line[7:].split()
                manifest[ident] = int(gz)
    except OSError:
        pass
    nm = _webasset_nm()
    if not elf or not manifest or not nm:
        return
    try:
        out = subprocess.run([nm, "-C", elf], capture_output=True, text=True).stdout
    except (OSError, subprocess.SubprocessError):
        return
    present = set(re.findall(r"WebAssets::(\w+)_gz\b", out))
    in_bin = 0
    for ident in sorted(manifest):
        gz = manifest[ident]
        if ident in present:
            in_bin += gz
            print("{}  {}: {} B gz -> in firmware{}".format(console_color.CYAN, ident, gz, console_color.END))
        else:
            print("{}  {}: dropped (unused, linker-removed){}".format(console_color.DARKBLUE, ident, console_color.END))
    print("{}  total in firmware: {}/{} assets, {} B gz{}".format(
        console_color.DARKBLUE, len(present & manifest.keys()), len(manifest), in_bin, console_color.END))


# Gate: only for products that actually run a webserver.
_webasset_defines = set(str(d[0] if isinstance(d, (list, tuple)) else d) for d in env.get("CPPDEFINES", []))
if "OPENKNX_WEBSERVER" in _webasset_defines:
    _webasset_generate()
    env.AddPostAction("checkprogsize", Action(_webasset_report, "Web assets: firmware report"))


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