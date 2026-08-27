# ---------------------------------------------------------------------------
#  Shared helpers for the OpenKNX OGM-Common PlatformIO build scripts.
#
#  One console style (colours, rule, section headers, byte formatting) so every
#  pre-build (prepare*.py) and post-build (patch_*, create_esp32_image, ...)
#  script reads the same, plus the one-time library-dependency resolver used by
#  the version and webasset steps.
# ---------------------------------------------------------------------------

import io
import locale
import os
import re
import sys

from platformio.proc import exec_command


class C:
    END = "\033[0m"
    BOLD = "\033[1m"
    GRAY = "\033[90m"
    BLUE = "\033[94m"
    DARKBLUE = "\033[34m"
    CYAN = "\033[96m"
    GREEN = "\033[92m"
    AMBER = "\033[93m"
    RED = "\033[91m"


# --- Console encoding ---------------------------------------------------------------------------
# On Windows a REDIRECTED stdout falls back to the code page (cp1252/cp850), and the box/check glyphs
# are not in it -> UnicodeEncodeError, and the build dies on a REPORT. Ask for UTF-8 first; if that is
# refused, fall back to ASCII glyphs. A report must never be the reason a build fails.
def _stdout_utf8():
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        return True
    except Exception:
        pass
    # SCons replaces sys.stdout with an object of its own -- it has neither reconfigure nor encoding.
    # Then the system locale decides, not "ascii": otherwise macOS/Linux falls back for no reason.
    enc = getattr(sys.stdout, "encoding", None) or locale.getpreferredencoding(False) or "ascii"
    try:
        "─✔✘█·—".encode(enc)
        return True
    except Exception:
        pass
    try:                     # cannot switch: at least never abort with an error
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding=sys.stdout.encoding or "ascii",
                                      errors="replace", line_buffering=True)
    except Exception:
        pass
    return False


UNICODE_OK = _stdout_utf8()

# Glyphs in one place, so the ASCII fallback reaches every one of them.
G_RULE = "─" if UNICODE_OK else "-"
G_OK = "✔" if UNICODE_OK else "+"
G_FAIL = "✘" if UNICODE_OK else "x"
G_BAR = "█" if UNICODE_OK else "#"
G_DOT = "·" if UNICODE_OK else "."
G_DASH = "—" if UNICODE_OK else "--"

RULE = C.GRAY + G_RULE * 78 + C.END


def human_bytes(n):
    """Byte count -> 2-decimal MB / whole KB / raw B, matching the ESP image report."""
    if n >= 1024 * 1024:
        return "{:.2f} MB".format(n / (1024 * 1024))
    if n >= 1024:
        return "{:.0f} KB".format(n / 1024)
    return "{} B".format(n)


def quiet_action(fn):
    """Post-Action ohne SCons-Aufrufzeile: die Bloecke drucken ihre eigene Ueberschrift."""
    from SCons.Script import Action
    return Action(fn, strfunction=lambda *a, **k: "")


def section(title, note=""):
    """Rule + bold title (+ optional gray note) -- the header of a report block."""
    print()
    print(RULE)
    line = C.BOLD + title + C.END
    if note:
        line += "  " + C.GRAY + note + C.END
    print(line)


def step(title):
    """A single pre-build generation step, e.g. 'Generate include/versions.h'."""
    print(C.CYAN + title + C.END)


def ok(msg, note=""):
    print(C.GREEN + G_OK + C.END + " " + msg + (("  " + C.GRAY + note + C.END) if note else ""))


def warn(msg, note=""):
    print(C.AMBER + msg + C.END + (("  " + C.GRAY + note + C.END) if note else ""))


def err(msg, note=""):
    """A red error line -- for a genuine failure, but it does NOT stop the build by itself."""
    print(C.RED + msg + C.END + (("  " + C.GRAY + note + C.END) if note else ""))


def identity_line(openknxid, appnumber, appversion, revision, note=""):
    """The one-line OpenKNX identity, shared by the stampers and the ESP image report.

    appversion packs the version as major<<4 | minor; revision is the firmware revision byte.
    """
    line = "{}identity{}  OpenKNX 0x{:02X} · app {} · {}v{}.{}.{}{}".format(
        C.GRAY, C.END, openknxid, appnumber, C.BOLD, appversion >> 4, appversion & 0x0F, revision, C.END)
    if note:
        line += "   " + C.GRAY + note + C.END
    return line


# --- library dependency resolver -------------------------------------------
# Shared by prepare.py (versions.h) and prepare_webassets.py (walks the tree to
# find each module's web/assets/). Resolving the LDF tree is not cheap, so the
# result is cached for the lifetime of the build process -- the second caller
# reuses it, and either script still works when run on its own.
_project_cache = None


def resolve_project(env):
    """Return (project, lib_builders, basepath) for the current build, resolved once."""
    global _project_cache
    if _project_cache is not None:
        return _project_cache

    import os
    from platformio.builder.tools.piolib import ProjectAsLibBuilder, LibBuilderBase

    # from platformio-core piolib.py: prune deps that are not actually dependent
    def _correct_found_libs(lib_builders):
        found = [lb for lb in lib_builders if lb.dependent]
        for lb in lib_builders:
            if lb in found:
                lb.search_deps_recursive(lb.get_search_files())
        for lb in lib_builders:
            for dep in lb.depbuilders[:]:
                if dep not in found:
                    lb.depbuilders.remove(dep)

    project = ProjectAsLibBuilder(env, "$PROJECT_DIR")
    basepath = "lib" if os.path.exists("lib/") else ".pio/libdeps/" + env["PIOENV"]

    ldf_mode = LibBuilderBase.lib_ldf_mode.fget(project)
    lib_builders = env.GetLibBuilders()
    project.search_deps_recursive()
    if ldf_mode.startswith("chain") and project.depbuilders:
        _correct_found_libs(lib_builders)

    _project_cache = (project, lib_builders, basepath)
    return _project_cache


def _lib_sizes(env):
    """
    What each of OUR libraries contributed, from the object files the build just produced.

    An UPPER BOUND, and printed as one: the linker drops unused sections afterwards
    (-ffunction-sections + --gc-sections), so a library that offers much and is used little shows
    larger here than it ends up. Good for comparing libraries and for noticing one of them growing
    between releases -- which is what it gets read for.

    Everything under lib<hash>/ is a library this project pulls in; the Arduino core and the SDK come
    from prebuilt archives instead, so they are reported as the REMAINDER of the binary rather than
    left out. Because our figures are upper bounds, that remainder is a lower bound.

    The toolchain PATH has to be handed to exec_command explicitly -- without it every call fails,
    every size stays zero, and the whole section silently prints nothing. That is exactly what it did.
    """
    build_dir = env.subst("$BUILD_DIR")
    sizetool = env.subst("$SIZETOOL") or "size"
    # The product's own sources are not "(this product)" -- the project has a name, and it is the one
    # a reader recognises next to OFM-Network and OGM-Common.
    product = os.path.basename(os.path.normpath(env.subst("$PROJECT_DIR"))) or "(this product)"
    sysenv = os.environ.copy()
    try:
        sysenv["PATH"] = str(env["ENV"]["PATH"])
    except Exception:
        pass

    groups = {}
    for root, _dirs, files in os.walk(build_dir):
        objs = [os.path.join(root, f) for f in files if f.endswith(".o")]
        if not objs:
            continue
        m = re.search(r"[/\\]lib[0-9a-f]+[/\\]([^/\\]+)", root)
        if m:
            name = m.group(1)
        elif root.endswith(os.sep + "src") or (os.sep + "src" + os.sep) in root:
            name = product
        else:
            continue                       # framework leftovers: counted in the remainder instead
        groups.setdefault(name, []).extend(objs)

    out = []
    for name, objs in groups.items():
        text = data = bss = 0
        for i in range(0, len(objs), 100):          # one call per batch, not one per object file
            res = exec_command([sizetool] + objs[i:i + 100], env=sysenv)
            # exec_command returns a dict here; older call sites read it as text. Take it as text
            # either way rather than unpack a shape that may not hold.
            for line in str(res.get("out", res) if isinstance(res, dict) else res).splitlines():
                parts = line.split()
                if len(parts) < 4 or not parts[0].isdigit():
                    continue
                text += int(parts[0]); data += int(parts[1]); bss += int(parts[2])
        if text + data > 0:
            ours = name == product or name.startswith(("OFM-", "OGM-", "OAM-", "knx", "TPUart"))
            out.append((text + data, bss, name, ours))
    out.sort(reverse=True)
    return out


def show_lib_sizes(env, binary_size=0):
    """
    Print the per-library table. Lives here because BOTH reports want it: the RP one in
    show_flash_partitioning.py and the ESP one in create_esp32_image.py, which are separate scripts
    hooked on different platforms. It was in the RP one alone, which is why it never appeared on ESP.
    """
    try:
        libs = _lib_sizes(env)
    except Exception as _e:
        print("{}  (library sizes unavailable: {}){}".format(C.GRAY, _e, C.END))
        return
    if not libs:
        return
    section("Libraries", "(compiled size - an upper bound, before the linker discards)")
    print("{}    {:<30}{:>10}{:>11}{}".format(C.GRAY, "", "flash", "ram", C.END))
    shown = [x for x in libs if x[0] >= 2048][:14]
    for flash, bss, name, ours in shown:
        # Pad first, colour second: an escape sequence counts as characters to format() and would
        # push every column of every coloured row out of line.
        nm = "{:<30}".format(name[:30])
        fl = "{:>10}".format(human_bytes(flash))
        rm = "{:>11}".format(human_bytes(bss))
        big = flash >= 100 * 1024
        print("    {}{}{}{}{}{}{}{}".format(
            C.CYAN if ours else "", nm, C.END if ours else "",
            C.AMBER if big else "", fl, C.END if big else "",
            C.GRAY + rm, C.END))
    small = [x for x in libs if x not in shown]
    if small:
        print("{}    {:<30}{:>10}{:>11}   {} more{}".format(
            C.GRAY, "(smaller ones)", human_bytes(sum(x[0] for x in small)),
            human_bytes(sum(x[1] for x in small)), len(small), C.END))
    # No "core + SDK" row: ours are upper bounds, so a remainder computed from them is meaningless.
    if binary_size:
        # Same emphasis as in the web-asset block: the total carries the statement.
        print("{}{:<30}{:>10}{}".format(
            "    " + C.GRAY + G_RULE * 47 + C.END + "\n    " + C.BOLD, "binary (text + data)",
            human_bytes(binary_size), C.END))
    print("{}    upper bounds: the linker drops unused sections afterwards, and the Arduino core{}"
          .format(C.GRAY, C.END))
    print("{}    and the SDK link from prebuilt archives, so they are not among these rows{}"
          .format(C.GRAY, C.END))
