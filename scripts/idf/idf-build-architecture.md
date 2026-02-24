# IDF Build Architecture — OGM-Common

> **Audience:** Developers who want to understand why and how OGM-Common  
> uses custom PlatformIO pre-scripts, sdkconfig mechanisms, and clean hooks.

---

## Contents

1. [Why Custom Scripts?](#1-why-custom-scripts)
2. [How pioarduino Works by Default](#2-how-pioarduino-works-by-default)
3. [How We Trigger a Custom IDF Build](#3-how-we-trigger-a-custom-idf-build)
4. [File Overview](#4-file-overview)
5. [Build Flow Step by Step](#5-build-flow-step-by-step)
6. [Guard Flag: custom_idf_build](#6-guard-flag-custom_idf_build)
7. [sdkconfig Files and Their Roles](#7-sdkconfig-files-and-their-roles)
8. [Clean Behaviour](#8-clean-behaviour)
9. [Certificate / Binary Data (.S Files)](#9-certificate--binary-data-s-files)
10. [IDF Version Tagging (--wrap)](#10-idf-version-tagging---wrap)
11. [Runtime Verification (Console)](#11-runtime-verification-console)
12. [Setting Up a New Project](#12-setting-up-a-new-project)

---

## 1. Why Custom Scripts?

### The Problem with Pre-built IDF Libs

pioarduino ships **pre-built IDF libraries** for the `arduino` framework —  
a ready-made package `framework-arduinoespressif32-libs` (~200 MB, ~60 min build time).  
These libs are compiled with a fixed default configuration:  
WiFi on, Bluetooth on, full RF calibration, default CPU frequency.

If you pass `CONFIG_*` values (e.g. `CONFIG_ESP_WIFI_ENABLED=n`) **only as compiler flags**,  
they appear in `sdkconfig.h` — but the pre-built `.a` libraries were already compiled  
without that change. The result:

- `CONFIG_ESP_WIFI_ENABLED=n` shows up in `sdkconfig.h`
- `libnet80211.a`, `libesp_wifi.a`, etc. are **still linked** and export symbols
- Your code sees the macro and skips WiFi calls — but the Arduino WiFi wrapper  
  (`WiFiGeneric.cpp`) still calls e.g. `esp_netif_create_default_wifi_ap`
- Linker error: **symbol not found** (or worse: memory is still allocated for  
  the WiFi stack despite the macro saying otherwise)

> **Key insight:** `CONFIG_*` defines visible in application code come from  
> **pre-built** `sdkconfig.h` headers, NOT from a custom IDF build.  
> The custom-built `.a` libs contain the correct settings internally,  
> but the preprocessor defines at app level still reflect the stock config.  
> This is why runtime APIs are the only trustworthy way to verify settings.

### The Solution

Build the IDF libraries **from source** with our own `CONFIG_*` values.  
WiFi symbols are truly removed, memory is saved, BLE is correctly configured —  
all at the assembly level.

---

## 2. How pioarduino Works by Default

```
pio run
  └── arduino.py (SConscript)
        ├── Check: framework-arduinoespressif32-libs present?
        │     YES → use pre-built libs from package cache
        │     NO  → call_compile_libs() → espidf.py → build IDF from source
        ├── Check: custom_sdkconfig set directly in [env:*]?
        │     YES + framework-libs/sdkconfig missing → call_compile_libs()
        └── Compile Arduino sources + link against IDF libs
```

The critical check in `arduino.py` (pioarduino internals):

```python
flag_custom_sdkconfig = config.has_option(current_env_section, "custom_sdkconfig")
flag_any_custom_sdkconfig = exists(join(framework_libs_dir, "sdkconfig"))

if flag_custom_sdkconfig and not flag_any_custom_sdkconfig:
    call_compile_libs()   # → runs espidf.py
```

**Important:** `config.has_option()` checks **only the direct `[env:*]` section**,  
not inherited sections via `extends`. Therefore `custom_sdkconfig` must always be  
set directly in the `[env:*]` section.

---

## 3. How We Trigger a Custom IDF Build

### Two Separate Mechanisms

| Mechanism | Purpose | Reader |
|---|---|---|
| `custom_sdkconfig` directly in `[env:*]` | Triggers pioarduino's `call_compile_libs()` | pioarduino `arduino.py` |
| `custom_idf_build = true` directly in `[env:*]` | Activates our pre-scripts | `idf_generate_crt_asm.py`, `idf_clean_artifacts.py` |

This separation is intentional:
- pioarduino reads `custom_sdkconfig` via `config.has_option()` — does **not** follow `extends`
- Our scripts read `custom_idf_build` via `env.GetProjectOption()` — **does** follow `extends` chains!  
  Therefore `custom_idf_build` must also be placed directly in `[env:*]`, not only in an extended section.

---

## 4. File Overview

```
lib/OGM-Common/
├── platformio.esp32idf.ini              ← [esp32idf] section: registers extra_scripts
├── platformio.esp32idf.example.ini      ← Step-by-step usage example for new projects
└── scripts/
    ├── pio/
    │   ├── generate_versions.py         ← Pre-script: lightweight versions.h generator
    │   ├── create_esp32_image.py        ← Post-script: merge bootloader+partitions+app → factory.bin
    │   ├── prepare.py                   ← (Arduino only) versions.h via PlatformIO library resolver
    │   ├── patch_uf2.py                 ← UF2 patcher
    │   └── show_flash_partitioning.py   ← Flash partition info
    └── idf/
        ├── idf_generate_crt_asm.py      ← Pre-script: sdkconfig, .S files, --wrap IDF version
        ├── idf_clean_artifacts.py       ← Pre-script: remove IDF artifacts on clean
        ├── idf_setup_components.py      ← Helper: set up managed_components
        ├── idf-build-architecture.md    ← This document
        └── certs/
            ├── .known_entries           ← Registry of cert/binary files and their types
            ├── rmaker_mqtt_server.crt   ← Amazon Root CA 1 (for MQTT)
            ├── rmaker_claim_service_server.crt ← Amazon Root CA 1 (for claim service)
            ├── rmaker_ota_server.crt    ← Certificate chain (DigiCert + Starfield Root)
            └── testfs.bin               ← Pre-formatted empty LittleFS image (test fixture)
```

### `platformio.esp32idf.ini`

Defines the `[esp32idf]` section inherited by every project via `extends = esp32idf`  
(or via `[custom]`):

```ini
[esp32idf]
extra_scripts =
  lib/OGM-Common/scripts/pio/generate_versions.py
  lib/OGM-Common/scripts/pio/create_esp32_image.py
  pre:lib/OGM-Common/scripts/idf/idf_generate_crt_asm.py
  pre:lib/OGM-Common/scripts/idf/idf_clean_artifacts.py
```

IDF scripts are registered as `pre:` — they run **before** the actual build.
`generate_versions.py` and `create_esp32_image.py` run as default (post) scripts.
`idf_clean_artifacts.py` internally checks `env.GetOption("clean")` and does
nothing outside of `--target clean`.

> **"Last wins" strategy:** PlatformIO's `extends` does NOT merge `extra_scripts` —
> the last definition wins. By placing `esp32idf` at the **end** of an env's `extends`
> list, its `extra_scripts` shadow those from `[ESP32]`. This replaces Arduino-only
> scripts (e.g. `prepare.py`) with IDF-compatible alternatives (e.g. `generate_versions.py`).

## 5. Build Flow Step by Step

### First Build (no IDF libs in cache)

```
pio run --environment release_..._LOW_POWER_CONSUMPTION
│
├─ [PlatformIO] Reads platformio.ini + extra_configs
│    [env:*] has custom_idf_build=true + custom_sdkconfig=...
│
├─ [pre-script] idf_generate_crt_asm.py
│    ├── Guard: custom_idf_build=true? → YES, proceed
│    ├── Run idf_setup_components.py (set up managed_components)
│    ├── _apply_custom_sdkconfig():
│    │    ├── Write CONFIG_* block to sdkconfig.<envname> (new/changed)
│    │    └── Delete sdkconfig (cache), BUILD_DIR/sdkconfig, BUILD_DIR/CMakeCache.txt
│    ├── Generate .S files for certificates (from components/certs/)
│    └── Generate __wrap_esp_get_idf_version() + add -Wl,--wrap (see §10)
│
├─ [pioarduino] arduino.py
│    ├── config.has_option("custom_sdkconfig") → TRUE (directly in [env:*])
│    ├── exists(framework-arduinoespressif32-libs/sdkconfig) → FALSE (first build)
│    ├── → call_compile_libs() → SConscript("espidf.py")
│    │
│    └─ [pioarduino] espidf.py (HandleArduinoIDFsettings)
│         ├── Read base sdkconfig from framework-arduinoespressif32-libs/{mcu}/sdkconfig
│         ├── Merge our CONFIG_* from custom_sdkconfig on top
│         ├── Write TASMOTA hash header + merged config → sdkconfig.defaults
│         ├── cmake configure (reads sdkconfig.defaults + sdkconfig.<envname>)
│         └── Compile all IDF components (~30–60 min)
│              → Write result to framework-arduinoespressif32-libs/
│              → Create framework-arduinoespressif32-libs/sdkconfig (hash marker)
│
└─ [pioarduino] Compile Arduino sources + link
     → firmware.elf / firmware.bin
```

### Follow-up Builds (IDF libs already cached)

```
pio run
│
├─ [pre-script] idf_generate_crt_asm.py
│    ├── custom_sdkconfig unchanged? → sdkconfig.<envname> identical → changed=False
│    └── No cache invalidation needed
│
├─ [pioarduino] arduino.py
│    ├── config.has_option("custom_sdkconfig") → TRUE
│    ├── exists(framework-arduinoespressif32-libs/sdkconfig) → TRUE
│    │    → NO call_compile_libs() (libs already present)
│    └── TASMOTA hash in sdkconfig.defaults == current hash?
│         YES → normal Arduino compilation (~37 sec)
│         NO  → recompile libs (config has changed)
│
└─ [pioarduino] Only compile changed sources + link
```

---

## 6. Guard Flag: `custom_idf_build`

```ini
; Must be directly in [env:*] — NOT only in an extended section!
custom_idf_build = true
```

Why a separate flag instead of using `custom_sdkconfig` as guard:

- `custom_sdkconfig` can be inherited via `extends` (e.g. `[sdkcfg_low_power]`)
- `env.GetProjectOption("custom_sdkconfig")` follows the `extends` chain → would  
  trigger for pure Arduino envs too (Adafruit Feather, etc.)
- `custom_idf_build = true` makes the intent **explicit and unambiguous**
- Envs without `custom_idf_build` (= all standard Arduino envs) are completely  
  ignored by our scripts — no side effects

---

## 7. sdkconfig Files and Their Roles

| File | Owner | Purpose | Delete on clean? |
|---|---|---|---|
| `sdkconfig.defaults` | **pioarduino** | TASMOTA hash + merged CONFIG_* for IDF cmake | **YES** — triggers full IDF rebuild (~60 min) |
| `sdkconfig.<envname>` | **Us** | cmake staleness trigger (is_cmake_reconfigure_required) | Yes (regenerated from INI values) |
| `sdkconfig` (no suffix) | IDF | Merged cache after cmake configure | **YES** — regenerated |
| `BUILD_DIR/sdkconfig` | IDF | Copy in build directory | **YES** |
| `BUILD_DIR/CMakeCache.txt` | CMake | Configure cache | Yes (on config change) |

### Why `sdkconfig.defaults` matters

pioarduino writes this:
```
# TASMOTA__<sha256-hash-of-config>
CONFIG_ESP_WIFI_ENABLED=n
CONFIG_BT_ENABLED=y
...
```

This hash is the **rebuild trigger**: on every build pioarduino checks if the hash of
current `custom_sdkconfig` values matches the stored one.
If the file is missing or the header is gone → pioarduino triggers a full
IDF reinstall (~60 min).

> **Note:** `idf_clean_artifacts.py` **does** delete `sdkconfig.defaults` on
> `--target clean`. This is intentional: a full clean should start from scratch.
> The next build will regenerate it and recompile the IDF libs.

### Why we write `sdkconfig.<envname>` (not `sdkconfig.defaults`)

`espidf.py` checks in `is_cmake_reconfigure_required()` whether `sdkconfig.<envname>`  
is newer than `CMakeCache.txt`. If yes → cmake reconfigure.  
This ensures our config changes propagate even between lib-recompile runs.

Writing to `sdkconfig.defaults` would destroy the TASMOTA header → full reinstall.

---

## 8. Clean Behaviour

`idf_clean_artifacts.py` runs on `pio run --target clean` and deletes:

| Path | Reason |
|---|---|
| `sdkconfig` (project root) | IDF merged cache — regenerated on next build |
| `sdkconfig.defaults` | TASMOTA hash header — triggers full IDF lib rebuild |
| `sdkconfig.<envname>` | Our generated CONFIG_* file — regenerated from INI values |
| `managed_components/` | IDF Component Manager output — re-fetched on next build |
| `idf_component.yml` | Generated by `idf_setup_components.py` |
| `.dummy` | pioarduino CMakeLists stub |
| `dependencies.lock` | IDF Component Manager lock — regenerated on next build |
| `CMakeLists.txt` | pioarduino regenerates this during build |
| `src/CMakeLists.txt` | pioarduino IDF app component registration |
| `components/*/` (except `certs/`) | Generated by `idf_setup_components.py` |

**Not deleted:**
- `components/certs/` → committed certificate files, always available
- `framework-arduinoespressif32-libs/` → custom-built IDF libs (expensive, ~60 min;
  only rebuilt when `sdkconfig.defaults` is missing or hash mismatch)

---

## 9. Certificate / Binary Data (.S Files)

ESP-IDF supports `target_add_binary_data()` in CMakeLists.txt to embed files  
(e.g. TLS certificates) directly into the firmware binary.

### Problem with PlatformIO

PlatformIO builds run through SCons, not CMake directly. The `.S` assembly files  
(with `.incbin`) auto-generated by ESP-IDF are gone after a `clean` — but PlatformIO  
doesn't know it needs to regenerate them.

### Our Solution

`idf_generate_crt_asm.py` generates the `.S` files **itself** as a pre-build step:

```
managed_components/<component>/CMakeLists.txt
  └── target_add_binary_data("cert.pem" TEXT)
        └── idf_generate_crt_asm.py
              ├── Copy cert.pem → components/certs/cert.pem  (committed to git)
              ├── Create .pio/build/<env>/cert.pem.S
              └── .incbin always points to components/certs/cert.pem
```

`components/certs/` is committed to git → even after `clean` (when `managed_components/`  
is gone), the `.S` files can be regenerated from the local cache.

### Certificate Details

| File | Content |
|---|---|
| `rmaker_mqtt_server.crt` | Amazon Root CA 1 (for ESP RainMaker MQTT) |
| `rmaker_claim_service_server.crt` | Amazon Root CA 1 (for claim service TLS) |
| `rmaker_ota_server.crt` | Certificate chain: DigiCert Baltimore CA-2 G2 (intermediate) + Starfield Root CA G2 |
| `testfs.bin` | Pre-formatted empty LittleFS image (test fixture for esp_littlefs component) |

---

## 10. IDF Version Tagging (--wrap)

To prove at runtime that the firmware uses a **custom-built IDF** (not pre-built libs),  
we override `esp_get_idf_version()` via the linker `--wrap` mechanism.

### How it works

1. `idf_generate_crt_asm.py` reads the IDF version from  
   `esp_idf_version.h` inside the `framework-espidf` package  
   (via `env.PioPlatform().get_package_dir("framework-espidf")`)
2. Generates a C file with a wrapper function:
   ```c
   const char* __wrap_esp_get_idf_version(void) { return "OpenKNX-IDF-v5.5.2"; }
   ```
3. Adds linker flag: `-Wl,--wrap=esp_get_idf_version`
4. Result: **every** call to `esp_get_idf_version()` (including third-party libs)  
   now returns `"OpenKNX-IDF-v5.5.2"` instead of the stock version string.

### Why --wrap instead of a #define?

- `#define` / `CPPDEFINES` only affect code that includes our header — third-party  
  libs still call the original function
- `--wrap` works at the **linker level** — transparent, no headers, no macros,  
  covers the entire binary
- Clean separation: the version string tells you exactly whether you're running  
  custom-built IDF or stock pre-built libs

---

## 11. Runtime Verification (Console)

The `info` command in the serial console shows an "IDF Platform" block  
(guarded by `OPENKNX_IDF_INFO` + `ARDUINO_ARCH_ESP32`):

```
IDF Platform
IDF:                     OpenKNX-IDF-v5.5.2 (Custom Build)
Chip:                    2 cores, rev 0.1
Flash:                   8 MB
CPU-Freq:                80 MHz
PM:                      Enabled (max 80 / min 80 MHz)
WiFi-TX-Power:           N/A (WiFi not started)
WiFi-PS:                 N/A (WiFi not started)
```

### What is shown and why

| Field | Source | Notes |
|---|---|---|
| IDF | `esp_get_idf_version()` | `"OpenKNX-IDF-"` prefix → "Custom Build", else "Pre-built" |
| Chip | `esp_chip_info()` | Runtime API — cores, silicon revision |
| Flash | `ESP.getFlashChipSize()` | Arduino runtime API |
| CPU-Freq | `getCpuFrequencyMhz()` | Actual runtime frequency |
| PM | `esp_pm_get_configuration()` | Runtime PM config (max/min MHz) |
| WiFi-TX-Power | `esp_wifi_get_max_tx_power()` | Shows "N/A" if WiFi not started yet |
| WiFi-PS | `esp_wifi_get_ps()` | Shows "N/A" if WiFi not started yet |

### Why no CONFIG_* values?

`CONFIG_*` defines in application code come from the **pre-built** `sdkconfig.h`  
headers, NOT from the custom IDF build. For example:
- `CONFIG_ESP_PHY_MAX_TX_POWER` shows `20` (pre-built default) instead of `8` (our custom value)
- `CONFIG_ESP_BROWNOUT_DET_LVL` shows `7` instead of `6`

The custom-built `.a` libraries contain the correct values internally,  
but the preprocessor defines visible to application code reflect the stock config.  
**Only runtime APIs provide trustworthy values.**

---

## 12. Setting Up a New Project

### Minimal Configuration

```ini
; platformio.ini
[platformio]
extra_configs =
  lib/OGM-Common/platformio.base.ini
  lib/OGM-Common/platformio.esp32.ini
  lib/OGM-Common/platformio.esp32idf.ini   ; ← provides [esp32idf] section
  platformio.custom.ini

; platformio.custom.ini
[sdkcfg_my_config]
custom_sdkconfig =
  CONFIG_BT_ENABLED=y
  CONFIG_BT_BLE_ENABLED=y
  CONFIG_BT_NIMBLE_ENABLED=y
  ...

; Standard Arduino env — no IDF, uses pre-built libs:
[env:my_release_arduino]
extends = custom_release_ESP32, ESP32_8MB, board_MY_BOARD
board = seeed_xiao_esp32s3
framework = arduino

; Custom IDF env — esp32idf at END of extends (shadows [ESP32]'s extra_scripts):
[env:my_release_idf]
extends = ESP32_8MB, board_MY_BOARD, custom_release_ESP32, sdkcfg_my_config, esp32idf
board = seeed_xiao_esp32s3
framework = arduino
; BOTH flags must be directly here (not only in extends):
custom_idf_build = true
custom_sdkconfig = ${sdkcfg_my_config.custom_sdkconfig}
```

> **Key:** `esp32idf` must be the **last** entry in `extends` so its `extra_scripts`
> shadow those from `[ESP32]` ("last wins"). This replaces `prepare.py` with
> `generate_versions.py` (IDF-compatible) and keeps `create_esp32_image.py` for
> factory.bin generation.

### Checklist

- [ ] `platformio.esp32idf.ini` listed in `extra_configs`
- [ ] `esp32idf` at the **end** of the `extends` list in the `[env:*]` section
- [ ] `custom_idf_build = true` **directly** in the `[env:*]` section
- [ ] `custom_sdkconfig = ...` **directly** in the `[env:*]` section (not only via extends)
- [ ] `sdkconfig` and `sdkconfig.*` are in `.gitignore` (all variants are generated)
- [ ] `managed_components/` is in `.gitignore`
- [ ] `components/certs/` is committed to git (certificate files)
