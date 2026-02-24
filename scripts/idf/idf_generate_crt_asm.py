"""
Open ■
┬────┴  pre_generate_crt_asm
■ KNX   2026 OpenKNX - Erkan Çolak

PlatformIO pre-script:
1. Generates .S assembly files for target_add_binary_data() entries from managed_components/.
   - Cert/binary source files are stored in components/certs/ (committed to git).
   - .incbin ALWAYS points to components/certs/<file> -- always available, no cache needed.
   - managed_components/ present: update components/certs/ + generate .S files.
   - managed_components/ missing (after clean): use components/certs/ directly.
   - No auto-rebuild, no temporary cache needed.
2. Patches CCFLAGS globally with -Wno-discarded-qualifiers if rainmaker certs are present.
"""

import os
import re
import shutil
import subprocess
import sys

Import("env")  # noqa: F821  -- PlatformIO SCons environment injection

# Guard: only run for IDF environments that build custom IDF libs.
# Detection marker: custom_idf_build = true must be set DIRECTLY in [env:*].
# Envs without this flag (e.g. Adafruit Arduino envs) are skipped.
try:
    flag = env.GetProjectOption("custom_idf_build", default=None)
    if not flag or str(flag).strip().lower() not in ("1", "true", "yes"):
        Return()
except Exception:
    Return()

# During clean runs do nothing -- clean_idf_artifacts.py handles cleanup
if env.GetOption("clean"):
    Return()

build_dir    = env.subst("$BUILD_DIR")
project_dir  = env.subst("$PROJECT_DIR")
managed_dir  = os.path.join(project_dir, "managed_components")

# components/certs/ is committed to git -- always available, survives everything
certs_dir    = os.path.join(project_dir, "components", "certs")

# Build dir may not exist yet on the very first build
os.makedirs(build_dir, exist_ok=True)
os.makedirs(certs_dir, exist_ok=True)

# Run idf_setup_components.py -- it lives alongside this script in OGM-Common.
# __file__ and sys.argv[0] are unreliable in SCons context, so the path is
# reconstructed from project_dir (always correct via env.subst("$PROJECT_DIR")).
setup_script = os.path.join(project_dir, "lib", "OGM-Common", "scripts", "idf", "idf_setup_components.py")
print(f"[pre_generate] Looking for idf_setup_components.py at: {setup_script}")
if os.path.isfile(setup_script):
    print(f"[pre_generate] Running idf_setup_components.py ...")
    result = subprocess.run([sys.executable, setup_script, project_dir], capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[pre_generate] ERROR: idf_setup_components.py failed with exit code {result.returncode}")
        print(f"stdout:\n{result.stdout}")
        print(f"stderr:\n{result.stderr}")
else:
    print(f"[pre_generate] WARNING: idf_setup_components.py not found -- skipping component setup")

# --------------------------------------------------------------------------- #
# Apply custom_sdkconfig overrides to sdkconfig.<envname>
# --------------------------------------------------------------------------- #
# pioarduino's arduino.py / espidf.py detects "custom_sdkconfig" directly in
# the [env:*] section (via config.has_option), merges the values with the base
# sdkconfig from framework-arduinoespressif32-libs, and writes the result to
# sdkconfig.defaults with a TASMOTA hash header -- then triggers a full IDF
# lib recompile.  pioarduino OWNS sdkconfig.defaults; we must not touch it.
#
# We only maintain sdkconfig.<envname> here.  espidf.py uses that file as a
# staleness trigger in is_cmake_reconfigure_required(): if sdkconfig.<envname>
# is newer than CMakeCache.txt, cmake reconfigures.  That ensures our config
# changes always propagate even between lib-recompile runs.
# --------------------------------------------------------------------------- #
_SDK_MARKER_BEGIN = "# BEGIN OpenKNX custom_sdkconfig (auto-generated -- do not edit)"
_SDK_MARKER_END   = "# END OpenKNX custom_sdkconfig"

def _apply_custom_sdkconfig_to_file(sdkconfig_path, config_lines, env_label):
    """Append/replace the OpenKNX custom_sdkconfig marker block in one sdkconfig file.
    Returns True only when the file content actually changed (triggers cache invalidation).
    """
    if not os.path.isfile(sdkconfig_path):
        # File was deleted (e.g. by a previous clean) -- create it so our
        # overrides are still written.  ESP-IDF uses Kconfig built-in defaults
        # for anything not specified in the file.
        print(f"[pre_generate] (sdkconfig) {env_label}: file missing, creating: {os.path.basename(sdkconfig_path)}")
        original = ""
    else:
        with open(sdkconfig_path, "r", encoding="utf-8") as f:
            original = f.read()

    # Remove existing marker block (idempotent)
    content = re.sub(
        r"\n?" + re.escape(_SDK_MARKER_BEGIN) + r".*?" + re.escape(_SDK_MARKER_END) + r"\n?",
        "",
        original,
        flags=re.DOTALL,
    )

    block = (
        "\n" + _SDK_MARKER_BEGIN + "\n"
        + "\n".join(config_lines) + "\n"
        + _SDK_MARKER_END + "\n"
    )
    new_content = content.rstrip("\n") + "\n" + block

    # Only write + report change if content actually differs
    if new_content == original:
        print(f"[pre_generate] (sdkconfig) {env_label}: no changes → {os.path.basename(sdkconfig_path)}")
        return False

    with open(sdkconfig_path, "w", encoding="utf-8") as f:
        f.write(new_content)

    print(f"[pre_generate] (sdkconfig) {env_label}: applied {len(config_lines)} override(s) → {os.path.basename(sdkconfig_path)}")
    return True

def _apply_custom_sdkconfig(env, project_dir):
    """Read custom_sdkconfig from PlatformIO env and write overrides to sdkconfig files."""
    try:
        raw = env.GetProjectOption("custom_sdkconfig")
    except Exception:
        return  # No custom_sdkconfig defined -- nothing to do

    # Parse non-empty, non-comment lines
    config_lines = [
        line.strip()
        for line in (raw or "").splitlines()
        if line.strip() and not line.strip().startswith("#")
    ]
    if not config_lines:
        print("[pre_generate] (sdkconfig) custom_sdkconfig is empty -- no overrides written")
        return

    env_name = env.subst("$PIOENV")
    print(f"[pre_generate] (sdkconfig) applying {len(config_lines)} custom_sdkconfig override(s) for env '{env_name}' ...")

    changed = False
    # sdkconfig.defaults: pioarduino's arduino.py / espidf.py besitzt diese Datei!
    # Es schreibt dort einen TASMOTA-Hash-Header + gemergten Config-Inhalt.
    # Würden wir hier rein schreiben, zerstören wir den TASMOTA-Header und
    # pioarduino would trigger a full framework reinstall on every build.
    # → sdkconfig.defaults is NOT touched by us.
    #
    # sdkconfig.{env_name}: unsere Datei für cmake-Staleness-Check (is_cmake_reconfigure_required)
    changed |= _apply_custom_sdkconfig_to_file(
        os.path.join(project_dir, f"sdkconfig.{env_name}"),
        config_lines,
        f"sdkconfig.{env_name}",
    )

    # Invalidate ESP-IDF sdkconfig cache so the next build re-reads sdkconfig.*
    # files.  ESP-IDF caches the merged config in <BUILD_DIR>/sdkconfig after the
    # first configure run and ignores sdkconfig.defaults on subsequent builds
    # unless the cache is removed.  CMakeCache.txt is also removed to force a
    # full re-configure so all CONFIG_* values propagate correctly into components.
    if changed:
        build_dir_local = env.subst("$BUILD_DIR")
        invalidated = []

        # ESP-IDF writes the merged sdkconfig cache both to PROJECT_DIR/sdkconfig
        # (project root, primary cache) and optionally to BUILD_DIR/sdkconfig.
        # Both must be removed to force a full re-configure.
        for cache_path in [
            os.path.join(project_dir, "sdkconfig"),          # primary IDF cache
            os.path.join(build_dir_local, "sdkconfig"),      # pio build dir copy
            os.path.join(build_dir_local, "CMakeCache.txt"), # CMake configure cache
        ]:
            if os.path.isfile(cache_path):
                os.remove(cache_path)
                invalidated.append(os.path.relpath(cache_path, project_dir))

        if invalidated:
            print(f"[pre_generate] (sdkconfig) ESP-IDF cache invalidated: {', '.join(invalidated)} → re-configure forced")
        else:
            print("[pre_generate] (sdkconfig) no ESP-IDF cache found (first build or already clean)")

_apply_custom_sdkconfig(env, project_dir)

# --------------------------------------------------------------------------- #
# Helper functions
# --------------------------------------------------------------------------- #
binary_data_pattern = re.compile(
    r'target_add_binary_data\s*\(\s*\S+\s+"([^"]+)"\s+(TEXT|BINARY)\s*\)',
    re.MULTILINE,
)


def _symbol_name(filepath):
    safe = re.sub(r"[^a-zA-Z0-9]", "_", os.path.basename(filepath))
    return "_binary_" + safe


def _write_asm(incbin_path, dest_s, file_type="TEXT"):
    """Generate a .S assembly file; incbin_path points to components/certs/<file>."""
    var = _symbol_name(incbin_path)
    os.makedirs(os.path.dirname(dest_s), exist_ok=True)
    with open(dest_s, "w") as f:
        f.write("    .section .rodata\n")
        f.write(f"    .global {var}_start\n")
        f.write(f"    .global {var}_end\n")
        f.write(f"{var}_start:\n")
        f.write(f'    .incbin "{incbin_path}"\n')
        f.write(f"{var}_end:\n")
        if file_type == "TEXT":
            f.write(".byte 0\n")


# --------------------------------------------------------------------------- #
# Known entries file -- persists filename:type mappings alongside the certs
# --------------------------------------------------------------------------- #
KNOWN_ENTRIES_FILE = os.path.join(certs_dir, ".known_entries")


def _load_known_entries():
    entries = {}
    if os.path.isfile(KNOWN_ENTRIES_FILE):
        with open(KNOWN_ENTRIES_FILE) as f:
            for line in f:
                line = line.strip()
                if line and ":" in line:
                    name, ftype = line.split(":", 1)
                    entries[name.strip()] = ftype.strip()
    return entries


def _save_known_entries(entries):
    with open(KNOWN_ENTRIES_FILE, "w") as f:
        for name, ftype in sorted(entries.items()):
            f.write(f"{name}:{ftype}\n")


# --------------------------------------------------------------------------- #
# 1) managed_components/ present -> update components/certs/, generate .S files
# --------------------------------------------------------------------------- #
if os.path.isdir(managed_dir):
    known_entries = _load_known_entries()

    for root, _dirs, files in os.walk(managed_dir):
        if "CMakeLists.txt" not in files:
            continue
        cml_path = os.path.join(root, "CMakeLists.txt")
        try:
            with open(cml_path, encoding="utf-8") as fh:
                cmake_content = fh.read()
        except OSError:
            continue

        for m in binary_data_pattern.finditer(cmake_content):
            rel_path  = m.group(1)
            file_type = m.group(2)

            if rel_path.startswith("${"):
                print(f"[pre_generate] Skipping cmake variable path: {rel_path}")
                continue

            src_file = os.path.abspath(os.path.join(root, rel_path))
            if not os.path.isfile(src_file):
                print(f"[pre_generate] WARNING: not found: {src_file}")
                continue

            fname      = os.path.basename(src_file)
            cert_file  = os.path.join(certs_dir, fname)

            # Update components/certs/ from managed_components/ (commit if changed)
            shutil.copy2(src_file, cert_file)
            known_entries[fname] = file_type

            # Generate .S file (.incbin -> components/certs/<file>)
            s_name = fname + ".S"
            dest_s = os.path.join(build_dir, s_name)
            if not os.path.isfile(dest_s):
                print(f"[pre_generate] Generating: {s_name}")
                _write_asm(cert_file, dest_s, file_type)
            else:
                print(f"[pre_generate] Already exists: {s_name}")

    _save_known_entries(known_entries)

# --------------------------------------------------------------------------- #
# 2) managed_components/ missing (after pio clean) -> use components/certs/
# --------------------------------------------------------------------------- #
else:
    known_entries = _load_known_entries()
    if not known_entries:
        print("[pre_generate] WARNING: components/certs/.known_entries is empty or missing.")
        print("[pre_generate]   Run a build once with managed_components/ present to populate it.")
    else:
        for fname, file_type in known_entries.items():
            cert_file = os.path.join(certs_dir, fname)
            if not os.path.isfile(cert_file):
                print(f"[pre_generate] WARNING: {fname} not found in components/certs/ -- skipping")
                continue
            s_name = fname + ".S"
            dest_s = os.path.join(build_dir, s_name)
            if not os.path.isfile(dest_s):
                print(f"[pre_generate] Generating (from components/certs/): {s_name}")
                _write_asm(cert_file, dest_s, file_type)
            else:
                print(f"[pre_generate] Already exists: {s_name}")


# --------------------------------------------------------------------------- #
# 3) Add -Wno-discarded-qualifiers globally if rainmaker certs are present
#    in components/certs/ (they are committed, so always true if project uses them).
# --------------------------------------------------------------------------- #
RAINMAKER_CERT = os.path.join(certs_dir, "rmaker_mqtt_server.crt")

if os.path.isfile(RAINMAKER_CERT):
    if "-Wno-discarded-qualifiers" not in str(env.get("CFLAGS", [])):
        env.Append(CFLAGS=["-Wno-discarded-qualifiers"])
    if "-Wno-error=discarded-qualifiers" not in str(env.get("CXXFLAGS", [])):
        env.Append(CXXFLAGS=["-Wno-error=discarded-qualifiers"])
    print("[pre_generate] Fixed const-qualifier flags for espressif__esp_rainmaker")

# --------------------------------------------------------------------------- #
# 4) Override esp_get_idf_version() via linker --wrap
#    Generates a small C file with __wrap_esp_get_idf_version() that returns
#    "OpenKNX-IDF-vX.Y.Z".  The linker flag -Wl,--wrap=esp_get_idf_version
#    redirects ALL calls to esp_get_idf_version() to our wrapper -- clean,
#    no macros, no headers, works everywhere including third-party libs.
#    Version is read from esp_idf_version.h in the framework-espidf package.
# --------------------------------------------------------------------------- #
def _read_idf_version(env):
    """Read IDF version from esp_idf_version.h.

    Search order:
      1. framework-espidf PlatformIO package (reliable, present before compilation)
      2. $IDF_PATH env/SCons variable (set later by pioarduino, backup)
    """
    def _parse_ver_h(path):
        major = minor = patch = None
        try:
            with open(path, encoding="utf-8") as f:
                for line in f:
                    if "#define ESP_IDF_VERSION_MAJOR" in line:
                        major = line.split()[-1].strip()
                    elif "#define ESP_IDF_VERSION_MINOR" in line:
                        minor = line.split()[-1].strip()
                    elif "#define ESP_IDF_VERSION_PATCH" in line:
                        patch = line.split()[-1].strip()
            if major and minor and patch:
                return f"{major}.{minor}.{patch}"
        except Exception:
            pass
        return None

    # 1) Via PlatformIO package manager -- most reliable in pre-scripts
    try:
        pkg_dir = env.PioPlatform().get_package_dir("framework-espidf")
        if pkg_dir:
            ver_h = os.path.join(pkg_dir, "components", "esp_common", "include", "esp_idf_version.h")
            if os.path.isfile(ver_h):
                v = _parse_ver_h(ver_h)
                if v:
                    return v
    except Exception:
        pass

    # 2) Fallback: $IDF_PATH SCons / OS env variable
    for idf_path in [env.subst("$IDF_PATH"), os.environ.get("IDF_PATH", "")]:
        if idf_path and idf_path != "$IDF_PATH":
            ver_h = os.path.join(idf_path, "components", "esp_common", "include", "esp_idf_version.h")
            if os.path.isfile(ver_h):
                v = _parse_ver_h(ver_h)
                if v:
                    return v

    return None

idf_version = _read_idf_version(env)
if idf_version:
    idf_ver_string = f"OpenKNX-IDF-v{idf_version}"
    print(f"[pre_generate] esp_get_idf_version() → {idf_ver_string}")

    # Generate the --wrap override source file
    gen_dir = os.path.join(env.subst("$BUILD_DIR"), "openknx_idf_wrap")
    os.makedirs(gen_dir, exist_ok=True)
    gen_c = os.path.join(gen_dir, "openknx_idf_version_wrap.c")
    new_content = (
        '// Auto-generated by idf_generate_crt_asm.py -- do not edit\n'
        '// Overrides esp_get_idf_version() via linker --wrap\n'
        f'const char* __wrap_esp_get_idf_version(void) {{ return "{idf_ver_string}"; }}\n'
    )
    existing_content = ""
    if os.path.isfile(gen_c):
        try:
            with open(gen_c, encoding="utf-8") as _f:
                existing_content = _f.read()
        except Exception:
            pass
    if existing_content != new_content:
        with open(gen_c, "w", encoding="utf-8") as _f:
            _f.write(new_content)
        print(f"[pre_generate] Written: {gen_c}")

    # Compile the wrapper and link it; --wrap redirects all calls
    env.BuildSources(os.path.join("$BUILD_DIR", "openknx_idf_wrap_obj"), gen_dir)
    env.Append(LINKFLAGS=["-Wl,--wrap=esp_get_idf_version"])
else:
    print("[pre_generate] WARNING: could not determine IDF version -- esp_get_idf_version() not wrapped")
