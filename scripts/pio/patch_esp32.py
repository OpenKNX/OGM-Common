Import("env")
import re
import hashlib
import struct

# ---------------------------------------------------------------------------
#  OpenKNX ESP32 identity stamp
#
#  The RP2040/RP2350 build writes the four OpenKNX identity bytes into the UF2
#  extension tag (patch_uf2.py), which is what lets a firmware file say which
#  device it belongs to. An ESP32 build ships a plain .bin and had no such
#  marker, so a client could not tell an ESP image apart from any other -- and
#  a firmware update over the bus has no second chance to notice.
#
#  This writes the same four bytes into esp_app_desc_t.reserv1, eight reserved
#  bytes that sit unused at file offset 40. Because they live inside segment 0,
#  both integrity markers of the image have to be recomputed afterwards:
#
#    * the XOR checksum byte (seed 0xEF over all segment data), and
#    * the SHA-256 over the whole image, appended when hash_appended is set.
#
#  Both are what the bootloader and esp_ota_end verify, so a stamped image is
#  accepted exactly like an unstamped one -- and rejected outright if this
#  script ever got them wrong, which is the failure mode we want.
# ---------------------------------------------------------------------------


class C:
    CYAN = "\033[96m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    END = "\033[0m"


ESP_IMAGE_MAGIC = 0xE9
APP_DESC_OFF = 32
APP_DESC_MAGIC = 0xABCD5432
STAMP_OFF = 40          # esp_app_desc_t.reserv1[2]
CHECKSUM_SEED = 0xEF


def _define(content, name, width):
    m = re.search(r"#define %s (0x)?([0-9A-Fa-f]{1,%d})" % (name, width), content)
    if m is None:
        return None
    return int(m.group(2), 16 if m.group(1) == "0x" else 10)


def _recompute(data):
    """Return the image with a correct XOR checksum byte and, if present, SHA-256."""
    if data[0] != ESP_IMAGE_MAGIC:
        raise ValueError("not an ESP32 image (magic 0x%02X)" % data[0])
    segments = data[1]
    hash_appended = data[23] == 1

    off = 24
    xor = CHECKSUM_SEED
    for _ in range(segments):
        if off + 8 > len(data):
            raise ValueError("segment table runs past the end of the image")
        (_, seg_len) = struct.unpack("<II", data[off:off + 8])
        off += 8
        if off + seg_len > len(data):
            raise ValueError("a segment declares more data than the image holds")
        for b in data[off:off + seg_len]:
            xor ^= b
        off += seg_len

    cs_off = off + (15 - (off % 16))
    if cs_off >= len(data):
        raise ValueError("no room for the checksum byte")
    data[cs_off] = xor

    if hash_appended:
        if len(data) < 32:
            raise ValueError("image too small for an appended hash")
        data[-32:] = hashlib.sha256(bytes(data[:-32])).digest()
    return data


def post_program_action(source, target, env):
    path = target[0].get_abspath() if target else None
    if path is None or not path.endswith(".bin"):
        path = env.subst("$BUILD_DIR/${PROGNAME}.bin")

    try:
        content = open("include/knxprod.h", "r").read()
    except OSError:
        print("{}  knxprod.h not readable — image left unstamped{}".format(C.RED, C.END))
        return

    openknxid = _define(content, "MAIN_OpenKnxId", 2)
    appnumber = _define(content, "MAIN_ApplicationNumber", 3)
    appversion = _define(content, "MAIN_ApplicationVersion", 3)
    if openknxid is None or appnumber is None or appversion is None:
        print("{}  identity not readable from knxprod.h — image left unstamped{}".format(C.RED, C.END))
        return

    # knxprod.h FIRST, exactly like patch_uf2.py. Since OGM-Common 7.5 the literal in main.cpp lives in
    # the dead #else branch (FIRMWARE_REVISION is defined), so reading main.cpp alone stamps 0 while the
    # device reports the real revision — and knxOTA then calls identical firmware a downgrade.
    m = re.search(r"#define MAIN_FirmwareRevision (\d{1,3})", content)
    if m is None:
        m = re.search(r"const uint8_t firmwareRevision = ([0-9]+);",
                      open(env["PROJECT_SRC_DIR"] + "/main.cpp", "r").read())
    if m is None:
        print("{}  firmware revision not readable — image left unstamped{}".format(C.RED, C.END))
        return
    revision = int(m.group(1))
    if revision > 31:
        # The device packs the revision into 5 bits of PID_VERSION, so anything above 31 wraps there and
        # would never match the stamp. Better to say so at build time than to ship an uncomparable image.
        print("{}  firmware revision {} exceeds 31 and cannot be reported by the device{}".format(C.RED, revision, C.END))
        return

    try:
        data = bytearray(open(path, "rb").read())
    except OSError:
        print("{}  {} not readable — nothing stamped{}".format(C.RED, path, C.END))
        return

    if len(data) < APP_DESC_OFF + 16 or data[0] != ESP_IMAGE_MAGIC:
        print("{}  not an ESP32 application image — nothing stamped{}".format(C.RED, C.END))
        return
    if struct.unpack("<I", data[APP_DESC_OFF:APP_DESC_OFF + 4])[0] != APP_DESC_MAGIC:
        print("{}  no application descriptor — nothing stamped{}".format(C.RED, C.END))
        return

    data[STAMP_OFF + 0] = openknxid & 0xFF
    data[STAMP_OFF + 1] = appnumber & 0xFF
    data[STAMP_OFF + 2] = appversion & 0xFF
    data[STAMP_OFF + 3] = revision & 0xFF

    try:
        data = _recompute(data)
    except ValueError as err:
        print("{}  {} — image left UNCHANGED{}".format(C.RED, err, C.END))
        return

    open(path, "wb").write(bytes(data))

    # Deliberately silent on success: create_esp32_image.py reads these four bytes back out of the
    # finished binary and prints them with the flash report, so the build says it once and says it proven.


env.AddPostAction("buildprog", post_program_action)
