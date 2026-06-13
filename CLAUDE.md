# OGM-Common – Claude Instructions

## Project

**OGM-Common** is the **OpenKNX base infrastructure library** — not a standalone application. It provides the module system, KNX stack integration, logging, flash storage, LED management, time API, and console for all OpenKNX firmware projects. It has no own `platformio.ini`; it is always included as a dependency in parent OpenKNX device projects alongside one or more OFMs (OpenKNX Function Modules).

> **Not self-contained:** OGM-Common cannot be compiled or run on its own. It requires a concrete device project that provides `hardware.h`, `knxprod.h`, and a `main.cpp` with the device-specific setup. OFMs (e.g. OFM-Network at `../OFM-Network`) build on top of OGM-Common — they depend on it, not the other way around.

- **Language**: C++17, Arduino framework
- **Developer level**: Advanced — no basics on Arduino, RP2040, ESP32, or KNX needed. Get straight to the point.
- **Branch convention**: `v1` = stable release, `v1dev` = active development, feature branches merge into `v1dev`

---

## Hardware Platforms

| Platform | Notes |
|----------|-------|
| **RP2040** | Reference platform, full support including dual-core (`OPENKNX_DUALCORE`) |
| **RP2350** | Full support, includes optional PSRAM via `PICO_RP2350_PSRAM_CS` |
| **ESP32** | Experimental, PSRAM supported via `BOARD_HAS_PSRAM` |
| **SAMD21** | Obsolete, no new hardware — still supported in code |

- RP2040/RP2350: **arduino-pico** by Earle Philhower, lwIP single-thread (`NO_SYS=1`), optional dual-core
- Platform guards: `#ifdef ARDUINO_ARCH_ESP32` / `#ifdef ARDUINO_ARCH_RP2040` / `#ifdef ARDUINO_ARCH_RP2350`

---

## Architecture & Key Classes

### `OpenKNX::Common` (`src/OpenKNX/Common.h/.cpp`)
Central singleton (`openknx`). Owns the module registry, drives `init()` → `setup()` → `loop()` for all registered modules, handles flash read/write, startup delay, heartbeat, watchdog, and power-save (SAVE_INTERRUPT_PIN).

### `OpenKNX::Module` (`src/OpenKNX/Module.h/.cpp`)
Base class for all OFMs. Modules implement: `init()`, `setup()`, `loop()`, `processCommand()`, `readFlash()`, `writeFlash()`. OFMs register themselves via `openknx.addModule(...)`.

### `OpenKNX::Log::Logger` (`src/OpenKNX/Log/Logger.h/.cpp`)
Logging system. Ring buffer for webserver/console streaming:
- Entry format: `[uint32_t seq][uint16_t len][char text[len]]` = 6-byte header + payload
- Sentinel: remaining bytes zeroed when entry doesn't fit at end, tail resets to 0
- API: `openknx.logger.getLogEntryAfter(lastSeq, buf, size, &outSeq)`

### `OpenKNX::Flash::Driver` (`src/OpenKNX/Flash/Driver.h/.cpp`)
Persistent flash storage for module data. Each module gets a fixed region by registration order.

### `OpenKNX::Time::TimeProvider` (`src/OpenKNX/Time/TimeProvider.h/.cpp`)
Abstract base for time sources (KNX DPT19, NTP via OFM-Network, etc.). `TimeManager` arbitrates between providers.

### `OpenKNX::Facade` (`src/OpenKNX/Facade.h/.cpp`)
Public API surface — `openknx` is an instance of `Facade`.

---

## KNX Concepts & Integration

KNX parameters from `knxprod.h` via macros (e.g. `ParamBASE_*`, `KoBASE_*`). Definitions in `src/Common.share.xml`.

Required defines in `knxprod.h`:
```
MAIN_OpenKnxId
MAIN_ApplicationNumber
MAIN_ApplicationVersion
BASE_StartupDelayBase
ParamBASE_StartupDelayTimeMS
BASE_HeartbeatDelayBase
KoBASE_Heartbeat
ParamBASE_HeartbeatDelayTimeMS
```

**Key compile defines:**
```
OPENKNX_DUALCORE             – enable dual-core support (RP2040 only)
OPENKNX_WATCHDOG             – enable watchdog (releases only — breaks debugger)
OPENKNX_WATCHDOG_MAX_PERIOD  – watchdog timeout in seconds (default 16)
OPENKNX_RECOVERY_TIME        – ms to hold prog button for factory reset (default 6000, 0 = off)
SAVE_INTERRUPT_PIN           – falling edge triggers power-save actions
```

---

## Embedded Constraints

This code runs on microcontrollers with severe resource limits — treat every byte and cycle as precious:

- **RAM**: RP2040 has 264 KB total (shared with stack, heap, lwIP, KNX stack). ESP32 has ~320 KB free heap typical. RP2350 and ESP32 can use PSRAM via helper macros (`HS_MALLOC`, `PSRAM_DATA`, `PSRAM_CODE`). No dynamic allocation in hot paths.
- **Flash**: Use `const` for read-only data — the linker places `.rodata` in flash automatically. `PROGMEM` does not exist on ESP32/RP2040. Avoid duplicating string literals. On RP2350/ESP32, use `PSRAM_CODE` to move large functions to PSRAM and free flash.
- **No heap churn**: No `new`/`delete` or `std::string` construction in `loop()` — use fixed buffers, stack locals, or pre-allocated members.
- **No STL bloat**: Avoid `std::map`, `std::function`, `std::stringstream` — prefer arrays, raw function pointers, `snprintf`.
- **Stack depth**: RP2040 has a single stack (no RTOS by default). Keep recursion and large stack frames out of callbacks.
- **CPU**: Single-core RP2040 @ 125 MHz, no FPU on Cortex-M0+. Avoid float where integer math suffices.
- When in doubt: measure before adding, and prefer the smaller solution.

---

## Coding Conventions

- **No `delay()`** — everything non-blocking, state machines with `millis()`
- **Platform guards**: `#ifdef ARDUINO_ARCH_ESP32` / `#ifdef ARDUINO_ARCH_RP2040` / `#ifdef ARDUINO_ARCH_RP2350`
- **PSRAM**: For ESP32/RP2350, use `HS_MALLOC`/`HS_CALLOC`/`HS_REALLOC` and `ps_new()` for dynamic allocation. Use `PSRAM_DATA` for large static buffers and `PSRAM_CODE` for heavy functions. Defined in `src/OpenKNX/helper.h`.
- No external libraries in `library.json` — only Arduino framework builtins
- Comments in German or English (mixed OK, but consistent per file)

### Formatting (Allman style, `.clang-format`)
- **Always follow `.clang-format` exactly** — it is the authoritative style definition
- Curly braces always on their **own line** (classes, functions, `if`, `else`, `for`, `while`, `case`, `enum`, `struct`, `namespace`, `extern`)
- No column limit — no artificial line breaks
- 4 spaces indentation, no tabs
- `namespace` content indented, `case` labels indented
- `if` without braces only for a **single** statement (`OnlyFirstIf`) — no single-line `else`
- Preprocessor directives not indented
- Short functions, lambdas, enums, and `case` labels may stay on one line (as per `AllowShort*` rules)
- When writing new code, match the style of the surrounding file exactly

---

## References

- [README.md](README.md) — module overview and configuration reference
- [doc/Applikationsbeschreibung-Common.md](doc/Applikationsbeschreibung-Common.md) — full KNX application documentation (German)
- [CHANGELOG.md](CHANGELOG.md) — version history

---

## Documentation Maintenance

After **any code change**, update all affected READMEs in the same response:

| Change type | Files to update |
|-------------|----------------|
| Public API / new feature | `README.md` |
| KNX parameters, compile defines | `README.md` + `doc/Applikationsbeschreibung-Common.md` |
| Bug fix (no API change) | No README update required |

Rules:
- Keep READMEs in sync with the code — never leave them stale after a change
- Do not create new README files unless the user explicitly asks
- Do not add a "Changed in this session" or similar meta-section — just update the relevant content inline
