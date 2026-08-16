# ---------------------------------------------------------------------------
#  Shared helpers for the OpenKNX OGM-Common PlatformIO build scripts.
#
#  One console style (colours, rule, section headers, byte formatting) so every
#  pre-build (prepare*.py) and post-build (patch_*, create_esp32_image, ...)
#  script reads the same, plus the one-time library-dependency resolver used by
#  the version and webasset steps.
# ---------------------------------------------------------------------------


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


RULE = C.GRAY + "─" * 78 + C.END


def human_bytes(n):
    """Byte count -> 2-decimal MB / whole KB / raw B, matching the ESP image report."""
    if n >= 1024 * 1024:
        return "{:.2f} MB".format(n / (1024 * 1024))
    if n >= 1024:
        return "{:.0f} KB".format(n / 1024)
    return "{} B".format(n)


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
    print(C.GREEN + "✔" + C.END + " " + msg + (("  " + C.GRAY + note + C.END) if note else ""))


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
