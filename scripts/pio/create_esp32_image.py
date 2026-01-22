# ---------------------------------------------------------------------------
# OpenKNX ESP32 Combined Binary Generator (Console Edition)
# ---------------------------------------------------------------------------

Import("env")
env = DefaultEnvironment()
platform = env.PioPlatform()

import sys, subprocess, shutil
from os.path import join

# ANSI Colors
class C:
    END = "\033[0m"
    BOLD = "\033[1m"
    GRAY = "\033[90m"
    BLUE = "\033[94m"
    GREEN = "\033[92m"

LINE = f"{C.GRAY}{'─'*70}{C.END}"

# ---------------------------------------------------------------------------
# Flash Layout Visualization
# ---------------------------------------------------------------------------
def print_flash_layout(sections, firmware_path, total_size=8 * 1024 * 1024):
    width = max(shutil.get_terminal_size((100, 20)).columns - 20, 100)

    parsed = []
    for off_str, path in sections:
        try:
            parsed.append((int(off_str, 16), path))
        except ValueError:
            pass

    if not any(off == 0x10000 for off, _ in parsed):
        parsed.append((0x10000, firmware_path))

    parsed.sort(key=lambda x: x[0])

    # Tabelle
    print(f"{C.GREEN}Flash Layout ({total_size//1024//1024} MB total){C.END}")
    print(f"{'#':<4}{'Offset':<12}{'Size':<10}{'Name':<20}{'Path'}")
    for idx, (off, path) in enumerate(parsed, start=1):
        next_off = parsed[idx][0] if idx < len(parsed) else total_size
        size = next_off - off
        size_str = f"{size//1024} KB" if size < 1024*1024 else f"{size//1024//1024} MB"
        color = C.BLUE if idx % 2 else C.GREEN
        name = path.split("/")[-1]
        print(f"{color}{idx:<4}{hex(off):<12}{size_str:<10}{name:<20}{C.END}{path}")

    print(C.GRAY + "─" * width + C.END)

    # Proportionaler Balken
    bar = []
    for idx, (off, _) in enumerate(parsed, start=1):
        next_off = parsed[idx][0] if idx < len(parsed) else total_size
        start = int(off / total_size * width)
        end = int(next_off / total_size * width)
        seg_width = max(end - start, 1)
        color = C.BLUE if idx % 2 else C.GREEN
        bar.append(color + "█" * seg_width + C.END)
    print(f"{C.GRAY}0x000000{C.END} |{''.join(bar)}| {C.GRAY}{hex(total_size)}{C.END}")
    print(C.GRAY + "─" * width + C.END)

    # Schematischer Balken
    #print(f"{C.BOLD}Schematic (equal segments):{C.END}")
    #seg_width = width // len(parsed)
    #schematic_bar = []
    #label_line = []
    #offset_line = []
    #for idx, (off, _) in enumerate(parsed, start=1):
    #    color = C.BLUE if idx % 2 else C.GREEN
    #    schematic_bar.append(color + "█" * seg_width + C.END)
    #    label_line.append(f"[{idx}]".center(seg_width))
    #    offset_line.append(hex(off).center(seg_width))
    #print(f"0x000000 |" + "|".join(schematic_bar) + f"| {hex(total_size)}")
    #print("          " + "".join(label_line))
    #print("          " + "".join(offset_line))
    #print(C.GRAY + "─" * width + C.END)

# ---------------------------------------------------------------------------
# Ensure esptool
# ---------------------------------------------------------------------------
try:
    sys.path.append(join(platform.get_package_dir("tool-esptoolpy")))
    import esptool
except Exception:
    print("Installing esptool...")
    subprocess.run([sys.executable, "-m", "pip", "install", "--upgrade", "esptool"])
    import esptool

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def esp32_create_combined_bin(source, target, env):
    print(f"\n{LINE}")
    print(f"{C.BOLD}Creating OpenKNX ESP32 Combined Firmware{C.END}")
    print(LINE)

    build_dir = env.subst("$BUILD_DIR")
    prog_name = env.subst("${PROGNAME}")
    new_file = f"{build_dir}/{prog_name}.factory.bin"
    fw = f"{build_dir}/{prog_name}.bin"

    chip = env.get("BOARD_MCU")
    flash_size = env.BoardConfig().get("upload.flash_size")
    flash_freq = env.BoardConfig().get("build.f_flash", "40m").replace("000000L", "m")
    flash_mode = env.BoardConfig().get("build.flash_mode", "dio")
    memory_type = env.BoardConfig().get("build.arduino.memory_type", "qio_qspi")

    if flash_mode in ["qio", "qout"]: flash_mode = "dio"
    if memory_type in ["opi_opi", "opi_qspi"]: flash_mode = "dout"

    cmd = [
        "esptool", "--chip", chip, "merge-bin", "-o", new_file,
        "--flash-mode", flash_mode, "--flash-freq", flash_freq, "--flash-size", flash_size
    ]

    print(f"{C.BOLD}Firmware Info{C.END}: {C.BLUE}{chip}{C.END} | Mode:{C.GREEN} {flash_mode}{C.END} | Size:{C.BLUE} {flash_size}{C.END} | Freq:{C.GREEN} {flash_freq}{C.END}")
    print(LINE)

    for offset, image in env["FLASH_EXTRA_IMAGES"]:
        cmd += [offset, image]
    cmd += [hex(0x10000), fw]

    # Visualisierung mit beiden Balken
    print_flash_layout(env["FLASH_EXTRA_IMAGES"], fw, 8 * 1024 * 1024)

    print(f"{C.BOLD}Esptool-Args:{C.END}\n'{C.GRAY}{' '.join(cmd)}'{C.END}")
    print(LINE)
    subprocess.run(cmd, check=True)

    print(f"\n{C.GREEN}✔ Combined binary created:{C.END} {new_file}\n{LINE}\n")

env.AddPostAction("buildprog", esp32_create_combined_bin)