"""
Open ■
┬────┴  idf_webassets_guard
■ KNX   2026 OpenKNX - Erkan Çolak

PlatformIO extra_script: idf_webassets_guard.py
Stops a build that uses the [esp32idf] script list and defines OPENKNX_WEBSERVER.

The [esp32idf] list replaces the one from [ESP32] ("last definition wins") and leaves
prepare_webassets.py out, so include/webassets.h is not generated on this path: a product with
OPENKNX_WEBSERVER would fail on the missing header or link a stale one, and such a build is
stopped instead.

Registered without "pre:" on purpose: build_flags only become CPPDEFINES during the platform
build script, which runs after the pre: scripts — a pre: script finds no CPPDEFINES key at all
and the guard reads an empty list, so it never fires and the build then fails on the missing
include/webassets.h or links a stale one.
"""
Import("env")

class _C:
    YELLOW = '\033[93m'
    RED    = '\033[91m'
    GRAY   = '\033[90m'
    END    = '\033[0m'

_TAG = f"{_C.YELLOW}[webassets_guard]{_C.END}"

# Same reading as prepare_webassets.py: CPPDEFINES holds strings and (name, value) pairs.
_defines = set(str(d[0] if isinstance(d, (list, tuple)) else d) for d in env.get("CPPDEFINES", []))

if "OPENKNX_WEBSERVER" in _defines:
    print(f"{_TAG} {_C.RED}OPENKNX_WEBSERVER is defined, but this env uses the [esp32idf] script list.{_C.END}")
    print(f"{_TAG} {_C.RED}include/webassets.h is not generated there, so the webserver would ship without assets.{_C.END}")
    print(f"{_TAG} {_C.GRAY}    build this product without 'esp32idf' in extends, or drop OPENKNX_WEBSERVER from this env{_C.END}")
    env.Exit(1)
