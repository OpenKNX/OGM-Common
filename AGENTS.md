# OGM-Common – Agent Instructions

## Project

**OGM-Common** is the **OpenKNX base infrastructure** — not a standalone
application. It provides the module system, KNX stack integration, logging,
flash storage, LED management, time API, and console for all OpenKNX
firmware projects. It has no `platformio.ini` of its own; it is always
pulled in as a dependency into a concrete device project together with one
or more OFMs (OpenKNX Function Modules).

> **Not self-contained:** OGM-Common cannot be compiled on its own. It
> needs a concrete device project that provides `hardware.h`, `knxprod.h`,
> `versions.h`, `version_main.h`, and a `main.cpp` with the device-specific setup. OFMs
> (e.g. OFM-Network under `../OFM-Network`) build on top of OGM-Common —
> not the other way around.

- **Language**: C++ (Arduino framework, no explicit `-std=` flag — the
  core's default applies, typically `gnu++17`)
- **Experience level**: Advanced — no need to explain Arduino, RP2040,
  ESP32, or KNX basics, get straight to the point.
- **Branch convention**: `v1` = stable release, `v1dev` = active
  development, feature branches get merged into `v1dev`.
- **Dependencies** (`library.json`): `khoih-prog/TimerInterrupt_Generic`,
  `robtillaart/ANSI`, `RTTStream` (github koendv) — so not entirely free
  of external libraries.

## Hardware Platforms

| Platform | Notes |
|----------|-------|
| **RP2040** | Reference platform, fully supported, dual-core via `OPENKNX_DUALCORE` |
| **RP2350** | Runs through the RP2040/arduino-pico toolchain branch; its own PSRAM via `RP2350_PSRAM_CS`, but no dedicated `ARDUINO_ARCH_RP2350` — arduino-pico compiles RP2350 under `ARDUINO_ARCH_RP2040` |
| **ESP32** | Fully supported (no longer experimental), PSRAM via `BOARD_HAS_PSRAM`, dual-core also supported (not just RP2040) |
| **SAMD21** | Deprecated, code paths still exist (`#ifdef ARDUINO_ARCH_SAMD`) but probably no longer work reliably — no new hardware, not actively maintained |

- Platform switches in code: `#ifdef ARDUINO_ARCH_ESP32` /
  `#ifdef ARDUINO_ARCH_RP2040` / `#ifdef ARDUINO_ARCH_SAMD` — **there is
  no `ARDUINO_ARCH_RP2350`**, RP2350-specific code goes through the
  core's own macros like `RP2350_PSRAM_CS` — that exact name, as set by
  arduino-pico in `boards.txt`.

## Architecture & Core Classes

### `OpenKNX::Common` (`src/OpenKNX/Common.h/.cpp`)
Central singleton (`openknx.common`, usually accessed through the facade
shorthand). Manages the module registry (`OPENKNX_MAX_MODULES`, default 9),
drives `init()` → `setup()`/`setup(bool configured)` →
`loop()`/`loop(bool configured)` for all modules (plus `setup1()`/
`loop1()` under `OPENKNX_DUALCORE`), triggers flash load/save (the actual
logic lives in `Flash::Default`), startup delay (only active when
`BASE_StartupDelayBase` is defined), heartbeat (only active when
`BASE_HeartbeatDelayBase` is defined), watchdog control (actual class:
`OpenKNX::Watchdog`, `openknx.watchdog`), and power-save via
`SAVE_INTERRUPT_PIN`.

Other important Common API: `restart()`, `freeLoopTime()`/
`freeLoopIterate()` (cooperative multitasking — OFMs should use this in
`loop()` instead of processing everything in one go), heap/stack stats.

### `OpenKNX::Base` (`src/OpenKNX/Base.h`) and `OpenKNX::Module` (`src/OpenKNX/Module.h`)
`Module` inherits from `Base`. Together they form the base class for all
OFMs — the methods are split across both, not just `Module`:

- `Base`: `name()` (pure virtual — **must** be implemented),
  `init()`, `setup()`/`setup(bool configured)`, `loop()`/
  `loop(bool configured)`, `setup1()`/`loop1()` (`OPENKNX_DUALCORE` only),
  `processInputKo()`, `processFunctionProperty[State]()`.
- `Module`: `version()` (pure virtual — **must** be implemented),
  `flashSize()`, `writeFlash()`, `readFlash(...)`,
  `processAfterStartupDelay()`, `processBeforeRestart()`,
  `processBeforeTablesUnload()`, `savePower()`, `restorePower()`,
  `processCommand(const std::string cmd, bool diagnoseKo)`,
  `showHelp()`, `showInformations()`.

Modules register via `openknx.addModule(uint8_t id, Module& module)`.

For channels within a module there is additionally `OpenKNX::Channel`
(`src/OpenKNX/Channel.h`) — the usual pattern in OFMs with several
same-kind channels (see also the channel-selection pattern in
`.claude/agents/openknx-channelselect.md`).

### `OpenKNX::Log::Logger` (`src/OpenKNX/Log/Logger.h/.cpp`)
More important than the internal ring buffer is the log API itself:
`logInfoP`/`logErrorP`/`logWarningP`/`logDebugP`/`logTraceP` (plus
`logHex*` variants), `logIndentUp()`/`logIndentDown()`. Messages are
bounded by `OPENKNX_MAX_LOG_PREFIX_LENGTH` (23) and
`OPENKNX_MAX_LOG_MESSAGE_LENGTH` (200) respectively.

Trace filters (`OPENKNX_TRACE`) have a new syntax: a single,
semicolon-separated list of filters instead of the old
`OPENKNX_TRACE1`..`OPENKNX_TRACE5`. Format `PREFIX<SUB>` with `*` as a
wildcard, ranges (`1-19`), and lists (`4,5,7`) inside `<...>`.

When `OPENKNX_WEBCONSOLE` is active, there is additionally a plain byte
ring buffer (`OPENKNX_WEBCONSOLE_BUFSIZE`, default 4096) for the web
console, accessible via `ringBuf()`/`ringWritePos()` — no structured
frame format, no sequence numbers.

### `OpenKNX::Flash::Default` (`src/OpenKNX/Flash/Default.h/.cpp`, `openknx.flash`)
This is the class modules actually interact with (`readXXX`/`writeXXX`).
It hands each module a fixed region in registration order, based on
`flashSize()`.

### `OpenKNX::Flash::Driver` (`src/OpenKNX/Flash/Driver.h/.cpp`)
Lower level: plain access to a flash region (read, write, erase, buffer
per sector). Has no notion of modules. Two instances:
`openknx.openknxFlash` and `openknx.knxFlash`.

### `OpenKNX::Time::TimeProvider` (`src/OpenKNX/Time/TimeProvider.h/.cpp`) and `TimeManager`
Abstract base for time sources (KNX DPT19 via `TimeProviderKnx`, NTP via
OFM-Network). `TimeManager` manages **exactly one** active provider
(`setTimeProvider()`) — no arbitration between multiple sources. More
relevant for OFMs is usually the event system
(`TimeChangedEvents`/`TimeChangeCallback`), used to react to time changes
instead of polling yourself.

### `OpenKNX::Facade` (`src/OpenKNX/Facade.h/.cpp`)
Public API surface — `openknx` is an instance of it. Besides the classes
mentioned above, this is also where `console`, `hardware`, `leds`
(`Led::Manager`), `ledFunctions` (`Led::FunctionManager`), `gpio`
(`GPIO::Manager`), `sun`, `calendar`, `progButton`/`func1..3Button`, and
`modules` (the registry itself) hang off.

### `OpenKNX::Charset` (`src/OpenKNX/Charset.hpp`)
Header-only, lossy-aware re-encoding between the firmware/KNX-bus encoding
(ISO-8859-15 — DPT16 strings, FAT32/LittleFS filenames) and UTF-8, which
is effectively unavoidable at the browser boundary (WebSocket text frames,
`fetch()`, HTML). Two functions: `encodeUtf8()` (ISO→UTF-8, can never
fail — every byte is a valid Unicode code point) and `decodeUtf8()`
(UTF-8→ISO, returns `bool` — `false` as soon as at least one character has
no Latin-15 equivalent and was replaced with `?`). Both are exact inverses
of each other for arbitrary byte sequences, not just for "genuine"
ISO-8859-15 text.

The entire file content sits behind `#ifdef OPENKNX_CHARSET` — without
that define the include is a no-op, no `.cpp` needed. OGM-Common itself
does not set this define; currently `OFM-Network` derives it automatically
from `OPENKNX_WEBSERVER` (`Network/Module.h`). For the background on why
this is needed (browsers force UTF-8 in several places with no override)
and the concrete usage sites: `../OFM-Network/AGENTS.md` (section
"Character Encoding: why everything is UTF-8") and
`../OFM-Network/README.Webserver.md` (section "Character Encoding").

### Other Subsystems (not documented in detail, but present)
`src/OpenKNX/Console.h/.cpp` (command console), `src/OpenKNX/Led/*`
(see `README_LED.md`), `src/OpenKNX/GPIO/*` (incl. PCA9554/PCA9557/
TCA6408/TCA9555 expanders), `src/OpenKNX/Button.h`,
`src/OpenKNX/Watchdog.h`, `src/OpenKNX/TimerInterrupt.h`,
`src/OpenKNX/Stat/*`, `src/OpenKNX/Sun/*`.

## KNX Concepts & Integration

KNX parameters come through macros from `knxprod.h` (e.g. `ParamBASE_*`,
`KoBASE_*`). Defined in `src/Common.share.xml` (see also
`src/Common.Router.share.xml`, `src/InfoLed.part.xml`).

**Mandatory** (otherwise the build fails, or the application is
non-functional): `MAIN_OpenKnxId`, `MAIN_ApplicationNumber`,
`MAIN_ApplicationVersion`.

**Optional, guarded by `#ifdef`** (each enables one feature):
`BASE_StartupDelayBase` + `ParamBASE_StartupDelayTimeMS` (startup delay),
`BASE_HeartbeatDelayBase` + `KoBASE_Heartbeat` +
`ParamBASE_HeartbeatDelayTimeMS` (heartbeat), `MAIN_OrderNumber`,
`BASE_PeriodicSave`, `BASE_KoManualSave`, `ParamBASE_Latitude`
(enables sun-position calculation).

**Enforced by the compiler via `#error` in `src/OpenKNX.h`:**
`-D SMALL_GROUPOBJECT` must be set, the target architecture must be
SAMD/RP2040/ESP32, `OPENKNX_FLASH_OFFSET`/`_SIZE` and
`KNX_FLASH_OFFSET`/`_SIZE` must be defined (on RP2040
`OPENKNX_FLASH_SIZE` must be a multiple of 4096), and a matching
`MASK_VERSION` (0x07B0 TP / 0x57B0 IP / 0x091A IPTP).

**Important compile defines (selection, see `src/OpenKNX/defines.h` and
`README.md` for the full list):**
```
OPENKNX_DUALCORE             – enable dual-core (RP2040 and ESP32)
OPENKNX_WATCHDOG             – enable watchdog (releases only — turned off
                                automatically with OPENKNX_DEBUGGER)
OPENKNX_WATCHDOG_MAX_PERIOD  – watchdog timeout in seconds (default 16)
OPENKNX_RECOVERY_TIME        – ms to hold the prog button for factory reset
                                (default 6000, 0 = off; needs PROG_BUTTON_PIN)
OPENKNX_DISABLE_PSRAM        – disable PSRAM (e.g. for Segger debugging)
OPENKNX_MAX_MODULES          – maximum number of modules (default 9)
OPENKNX_MAX_LOOPTIME /
OPENKNX_LOOPTIME_WARNING     – loop-time monitoring
OPENKNX_WEBCONSOLE /
OPENKNX_WEBCONSOLE_BUFSIZE   – web console incl. ring buffer size (default 4096)
OPENKNX_TRACE                – trace filter, new syntax (see Logger above)
FIRMWARE_REVISION            – changes the signature of openknx.init()
SAVE_INTERRUPT_PIN           – falling edge triggers power-save
                                (define in hardware.h, not a build flag)
```

## Embedded Constraints

The code runs on microcontrollers with very limited resources — treat
every byte and every cycle as precious:

- **RAM**: RP2040 has 264 KB total (shared with stack, heap, lwIP, KNX
  stack). ESP32 typically has ~320 KB free heap. RP2350 and ESP32 can use
  PSRAM when `OPENKNX_PSRAM` is active (auto-detected via
  `BOARD_HAS_PSRAM`/`RP2350_PSRAM_CS`, can be turned off via
  `OPENKNX_DISABLE_PSRAM`) — for that, the helpers `PSRAM_MALLOC`/
  `PSRAM_CALLOC`/`PSRAM_REALLOC`, `psram_new()`/`psram_delete()`,
  `PsramAllocator<T>`, `PSRAM_DATA`/`PSRAM_CODE` in
  `src/OpenKNX/Helper.h`. No dynamic allocation in hot paths.
  The macros expand differently per core (ESP32 `ps_malloc` +
  `.ext_ram.bss`, RP2350 `pmalloc` + `.psram`) — section names must match
  the core's own linker script, or the data silently ends up in normal
  RAM. `PSRAM_DATA` lives in a `NOLOAD` section: it cannot be statically
  initialized and is not zeroed at startup.
- **Flash**: use `const` for read-only data — the linker automatically
  places `.rodata` in flash. `PROGMEM` exists on ESP32/RP2040 (as a
  no-op) but gains nothing there — don't use it. Avoid duplicated string
  literals. On RP2350/ESP32, use `PSRAM_CODE` to move large functions
  into PSRAM and save flash.
- **No heap churn in `loop()`**: no `new`/`delete` and no `std::string`
  construction in hot paths — prefer fixed buffers, stack locals, or
  pre-allocated members. `std::string` itself is part of the mandatory
  API (`name()`, `version()`, `logPrefix()`, `processCommand()`) and is
  therefore not banned outright, just use it sparingly in `loop()`.
- **Use the STL deliberately**: `std::function` is used intentionally in
  the time API (`TimeChangeCallback`) — not a blanket ban, but prefer to
  avoid `std::map`/`std::stringstream` & co. where plain arrays/`snprintf`
  suffice.
- **Stack**: RP2040 has its own stack per core (no RTOS by default),
  monitored via `_freeStackMin`/`_freeStackMin1`. ESP32 runs on FreeRTOS,
  the second core's stack is configurable via `ARDUINO_LOOP1_STACK_SIZE`.
  Avoid recursion and large stack frames in callbacks.
- **CPU**: RP2040 is dual-core Cortex-M0+ @ 125 MHz, no FPU — avoid float
  where integer arithmetic suffices (RP2350/Cortex-M33 has an FPU).

## Build Script (`scripts/pio/prepare.py`)

Runs as a PlatformIO pre-script on **every** OAM build (device project),
not per module — must be wired into the device project's `platformio.ini`
as `extra_scripts`. Generates `include/versions.h` (module/build versions)
among other things, and cleans up stale
`lib/OGM-Common/include/{knxprod,versions,version_main,hardware}.h` (leftovers from an
earlier convention).

### Web Assets (`include/webassets.h`)

Collects a `web/assets/` folder from every included module **and the project
itself**, minifies + gzip-compresses each `.css`/`.js`/`.svg`/`.jpg`/`.jpeg`/
`.png` file, and generates `include/webassets.h` from it — so modules can keep
their web assets as plain, readable source files instead of hand-minified
C++ string literals. No file is generated if nothing is found. For the full
mechanics (identifier scheme, duplicate handling, generated symbols, how a
module consumes them) see `../OFM-Network/AGENTS.md`, which documents this in
depth as the first module using it.

## Code Conventions

- **No `delay()`** — everything non-blocking, state machines with
  `millis()`
- **Platform switches**: `#ifdef ARDUINO_ARCH_ESP32` /
  `#ifdef ARDUINO_ARCH_RP2040` / `#ifdef ARDUINO_ARCH_SAMD`
- **PSRAM**: see Embedded Constraints above, everything defined in
  `src/OpenKNX/Helper.h`
- Comments in German or English (mixed is OK, but consistent per file)

### Formatting (`.clang-format`, based on LLVM with Allman-style brace rules)
- **Always follow `.clang-format` exactly** — it is authoritative
- Braces always on their own line (classes, functions, `if`, `else`,
  `for`, `while`, `case`, `enum`, `struct`, `namespace`)
- No column limit
- 4 spaces indentation, no tabs
- **Preprocessor directives are indented when nested**
  (`IndentPPDirectives: BeforeHash`) — the indentation sits before the `#`
- `namespace` content is indented (`NamespaceIndentation: All`), as are
  `case` labels
- `if` without braces only for a single statement
  (`AllowShortIfStatementsOnASingleLine: OnlyFirstIf`)
- Short functions/lambdas/enums/`case` labels may stay single-line
- When writing new code, match the style of the surrounding file

## References

- [README.md](README.md) — module overview and configuration reference
- [README_LED.md](README_LED.md) — LED subsystem
- [doc/Applikationsbeschreibung-Common.md](doc/Applikationsbeschreibung-Common.md) — full KNX application documentation
- [doc/Build-and-release-environment.md](doc/Build-and-release-environment.md), [doc/Update-ETS-Application.md](doc/Update-ETS-Application.md)
- [CHANGELOG.md](CHANGELOG.md) — version history

## Documentation Maintenance

After **every** code change, update all affected READMEs in the same
pass:

| Kind of change | Files to update |
|-------------|----------------|
| Public API / new function | `README.md` |
| KNX parameters, compile defines | `README.md` + `doc/Applikationsbeschreibung-Common.md` |
| Bugfix (no API change) | No README update needed |

Rules:
- Keep READMEs always in sync with the code — never leave them stale
- Don't create new README files unless explicitly requested
- Don't add a "changed in this session" section or similar — update
  content directly at the appropriate spot
