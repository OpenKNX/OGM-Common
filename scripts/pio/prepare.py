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


# ── Web assets ────────────────────────────────────────────────────────────
#
# Jedes eingebundene Modul (und das Projekt selbst) darf einen web/assets/
# Ordner mit sauber formatierten .css/.js/.svg/.jpg/.png-Dateien anlegen.
# Hier werden sie minifiziert + gzip-komprimiert in include/webassets.h
# geschrieben. Es wird ausschliesslich gzip ausgeliefert (keine Content-
# Negotiation) -- die Firmware-Seite muss "Content-Encoding: gzip" setzen.
#
# Bezeichner sind flach (kein Modul-Praefix) und werden aus dem relativen
# Pfad unter web/assets/ abgeleitet. Jede Kollision -- auch zwischen dem
# Projekt selbst und einem Modul -- ist ein Fehler, kein Override.

import gzip as _gzip

WEBASSET_MIME = {
    ".css": "text/css",
    ".js": "text/javascript",
    ".svg": "image/svg+xml",
    ".jpg": "image/jpeg",
    ".jpeg": "image/jpeg",
    ".png": "image/png",
}


def _webasset_identifier(rel_posix_path):
    ident = re.sub(r"[^0-9a-zA-Z_]", "_", rel_posix_path)
    if ident and ident[0].isdigit():
        ident = "_" + ident
    return ident


def _minify_css(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    text = re.sub(r"\s+", " ", text)
    text = re.sub(r"\s*([{}:;,])\s*", r"\1", text)
    text = re.sub(r";}", "}", text)
    return text.strip()


def _minify_js(text):
    # Nur Whitespace am Zeilenanfang/-ende und Leerzeilen entfernen. Keine
    # Kommentar-Entfernung: ein Regex kann '//' in einem String- oder
    # Regex-Literal nicht sicher von einem echten Kommentar unterscheiden.
    lines = [line.strip() for line in text.splitlines()]
    return "\n".join(line for line in lines if line)


def _minify_svg(text):
    text = re.sub(r"<!--.*?-->", "", text, flags=re.DOTALL)
    text = re.sub(r">\s+<", "><", text)
    return text.strip()


def _format_byte_array(data, width=20):
    hexed = ["0x{:02x}".format(b) for b in data]
    return "\n".join(
        "    " + ",".join(hexed[i:i + width]) + ","
        for i in range(0, len(hexed), width)
    )


def _get_library_roots(root):
    roots = []

    def walk(node):
        for lb in node.depbuilders:
            roots.append((lb.name, pathlib.Path(lb.path)))
            if lb.depbuilders:
                walk(lb)

    walk(root)
    return roots


def _collect_webassets(root):
    seen_dirs = set()
    seen_ident = {}  # identifier -> (owner label, source file)
    entries = []     # (identifier, extension, absolute file path)

    sources = _get_library_roots(root) + [("<project>", pathlib.Path(env.subst("$PROJECT_DIR")))]
    for owner, lib_path in sources:
        assets_dir = lib_path / "web" / "assets"
        if not assets_dir.is_dir():
            continue
        real = assets_dir.resolve()
        if real in seen_dirs:
            continue
        seen_dirs.add(real)

        for file_path in sorted(assets_dir.rglob("*")):
            if not file_path.is_file():
                continue
            ext = file_path.suffix.lower()
            if ext not in WEBASSET_MIME:
                continue

            rel = file_path.relative_to(assets_dir).as_posix()
            ident = _webasset_identifier(rel)

            if ident in seen_ident:
                other_owner, other_file = seen_ident[ident]
                print("{}Duplicate web asset identifier '{}':{}".format(
                    console_color.RED, ident, console_color.END))
                print("  {} ({})".format(file_path, owner))
                print("  {} ({})".format(other_file, other_owner))
                raise SystemExit(1)
            seen_ident[ident] = (owner, file_path)
            entries.append((ident, ext, file_path))

    return entries


def _generate_webassets_header():
    entries = _collect_webassets(project)
    target = pathlib.Path("include/webassets.h")

    if not entries:
        if target.is_file():
            target.unlink()
        return

    parts = [
        "#pragma once",
        "// Auto-generated by OGM-Common/scripts/pio/prepare.py from web/assets/",
        "// directories across all included modules -- do not edit by hand.",
        "#include <cstddef>",
        "#include <cstdint>",
        "",
        "namespace WebAssets",
        "{",
    ]

    total_raw = 0
    total_gz = 0
    minifiers = {".css": _minify_css, ".js": _minify_js, ".svg": _minify_svg}

    print("{}Web assets:{}".format(console_color.YELLOW, console_color.END))

    for ident, ext, file_path in entries:
        raw = file_path.read_bytes()
        if ext in minifiers:
            minified = minifiers[ext](raw.decode("utf-8")).encode("utf-8")
        else:
            minified = raw  # .jpg/.png -- binaer, kein Minifier sinnvoll

        compressed = _gzip.compress(minified, compresslevel=9, mtime=0)
        total_raw += len(raw)
        total_gz += len(compressed)

        pct = round(len(compressed) * 100 / len(raw)) if raw else 0
        print("{}  {}: {} -> {} bytes ({}%){}".format(
            console_color.CYAN, ident, len(raw), len(compressed), pct, console_color.END))

        # Keine separate _gz_len-Konstante: die Groesse ist ueber sizeof(x_gz) am
        # Aufruf bereits bekannt (Array mit fester Bound) -- ein Laengenfeld waere
        # nur eine redundante, von Hand nachzufuehrende Zahl. Wichtig: das ist NICHT
        # NUL-terminiert -- gzip-Bytes sind Binaerdaten und enthalten praktisch immer
        # ein 0x00 irgendwo im Stream; strlen() wuerde den Inhalt zufaellig abschneiden.
        parts.append("    inline const uint8_t {}_gz[] = {{".format(ident))
        parts.append(_format_byte_array(compressed))
        parts.append("    };")
        parts.append("    inline const char* const {}_mime = \"{}\";".format(ident, WEBASSET_MIME[ext]))
        parts.append("")

    parts.append("} // namespace WebAssets")
    parts.append("")

    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text("\n".join(parts), encoding="utf-8")

    total_pct = round(total_gz * 100 / total_raw) if total_raw else 0
    print("{}  Total: {} files, {} -> {} bytes ({}%){}".format(
        console_color.DARKBLUE, len(entries), total_raw, total_gz, total_pct, console_color.END))
    print()


_generate_webassets_header()


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