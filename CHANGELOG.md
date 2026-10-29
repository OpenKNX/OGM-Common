# Changes

## ec/v2.0.0-beta.2: 2026-10-29

Fixes, OTA, I2C expander LEDs and build tooling on top of `ec/v2.0.0-beta.1`.

* Change: the KNX and system info widgets label Address and Number in English; comments and an I2C console line are English
* Fix (RP2040): the watchdog fires after the configured 16 s. Erratum RP2040-E1 caps the hardware at about 8.4 s, so it fired after 8.4 s; a 1 s timer now feeds it while `loop()` runs, so a stalled `loop()` resets after 16 to 17 s and a core with interrupts off still within 8 s. RP2350, ESP32 and SAMD are unchanged
* Feature (ESP32): `KNX_FLASH_OFFSET`/`_SIZE` and `OPENKNX_FLASH_OFFSET`/`_SIZE` place the `knx` and `openknx` partitions, as on the RP2040. They were ignored on ESP32, where only the partition table decided. `prepare_esp32_partitions.py` replaces offset and size of these two rows in the configured table when an offset is set after `build_unflags`; without an offset the table is used unchanged, so a product that sets only `KNX_FLASH_SIZE=0` is not affected
* Feature (ESP32): the build stops on an offset not aligned to 64 KiB, a size not aligned to 4 KiB, an offset without size, two different values, an overlap or a partition behind the flash. The firmware logs an error at startup when a flashed partition differs from the flags, since OTA does not write the partition table

* Fix: the time block is initialised and the weekday is set before the time is encoded. `tm` was left uninitialised, so every member the block does not assign was indeterminate when the DPT encoders and `mktime` read it, and a failed encode left the KO holding the previous time. `tm_wday` carries the KNX numbering, 0 no day, 1 Monday to 7 Sunday
* Feature: a developer build compares the `.knxprod` it built with `include/knxprod.h`, the header the firmware uses: the application identity ETS checks before a full download and the size of the parameter block. A mismatch means XML and header come from different producer runs, where ETS aborts the download or the firmware reads parameters at wrong offsets. A match does not prove an identical layout, and the message says so
* Fix: that comparison names every reason a line could not be compared instead of skipping it, checks each value to fit a byte, and shows the green tick only when nothing was left uncompared
* Fix (ESP32, custom IDF build): the `.bin` carries the four OpenKNX identity bytes. The IDF script list had no `patch_esp32.py`; both lists now take the shared `[esp32_scripts_common]`, so `platformio.esp32.ini` is required in `extra_configs` of every project that uses `[esp32idf]`
* Feature (ESP32, custom IDF build): a product with `OPENKNX_WEBSERVER` stops on the IDF script list, which generates no `include/webassets.h`, instead of failing on the missing header or linking a stale one
* Change: the release scripts look for the desktop tool as oknx under `Tools/oknx/<OS>/<arch>/`; a release that still ships the old name is reported as missing
* Fix: the firmware version text with a git hash has room for a long hash; a short hash longer than 10 characters overflowed the 20-byte stack buffer

**OTA**
* Feature (RP2040): an applied picoOTA image is removed at the next start once the bootloader has erased its command; it stayed in LittleFS and blocked the next upload on a 2 MB RP2040. A pending or unreadable command is left alone
* Feature: dev/test switch `OPENKNX_OTA_EXCLUSIVE` -- during a network OTA `loop()` runs only watchdog, LEDs, I2C queue and display, so a module that blocks in `loop()` shows as the cause of a failing OTA. Not for production

**I2C expander LEDs and inputs**
* Feature: `OPENKNX_I2C_SCHEDULER` (RP2040, RP2350, ESP32; opt-in per board) writes the soft-PWM edges of expander LEDs at fixed times (200 Hz) on a shared I2C bus and runs driver transfers in the gaps. RP2040/RP2350: transfers up to 33 write / 16 read octets run from the I2C IRQ, display chunks are queued without waiting, expander inputs are read every 10 ms and served from that cache, a stuck transfer is aborted and the bus cleared with 9 SCL pulses. Refused at compile time with `OPENKNX_WIRE_PIO` and on RP2350 RISC-V
* Fix (soft-PWM without scheduler): a tick during the I2C flush write is kept; an expander write is queued only on a level change; on ESP32 with serial LEDs the I2C LEDs tick every 3 ms instead of every 10 ms; the 500 ms reassert runs in the LED flush context
* Fix (PCA95xx): a short register read is reported as a read error. It returned 0xFF as input data, so the REG2 front panel reported UP, DOWN, OK and RIGHT pressed at once
* Feature: `digitalReadPort()` reads all inputs of a PCA9557/PCA9554 port in one transfer; `outputRegister()` names the output register of an expander pin
* Fix: an LED is added only once per function group; the setup ran twice, so every effect call built its effect twice

**Console** (`OPENKNX_CONSOLE_DEV`)
* Change: without the switch `leds` offers only `leds status`; with it a test override per LED (level, blink, sweep) and `leds pwm status/set/demo/blink/stress/stop`, with scheduler counters and the share of loop time spent in I2C
* Fix: `leds pwm` printed the timer rate as an 8-bit value (77 Hz instead of 333 Hz)

**Build**
* Fix: `versions.h` takes a library's git hash from the folder PlatformIO resolved, not from `lib/<library.json name>`; OFM-LightManager stopped the build with FileNotFoundError
* Fix (ESP32): the generated partition table is named `partitions_openknx.csv`; the old name gave an invalid `ARDUINO_PARTITION_` macro and a warning in every translation unit
* Change: the flash report measures the knxOTA image cut from the linked ELF instead of the previous build's `firmware.bin`
* Feature: `OPENKNX_SKIP_KNXPROD=1` lets the release preprocess skip the producer and pack the ETS data of its last run
* Change: producer warning 003 for the unused `newline()` helper of `Common.script.js` is suppressed
* Fix: the release build detects an IDF build from the env section of the project config and adds a firmware to `content.xml` only once
* Fix (ESP32): the factory image `firmware.factory.bin` is merged with the esptool the ESP32 platform configures, run as `$PYTHONEXE $OBJCOPY merge-bin`, instead of an `esptool` executable from the PATH; the script no longer installs esptool through pip. Built for IP-Interface on the REG1 LAN TP Base (ESP32) and the REG2 Pico ESP (ESP32-S3) with esptool 5.0.2: bootloader and app sit at their offsets, the app is byte-identical to `firmware.bin`

## ec/v2.0.0-beta.1: 2026-09-15

The entries below are on `ec/v1dev-ec` and not released upstream yet. The library version moves to
`2.0.0-beta.1` with this tag, so the file and the tag state the same thing -- a tag naming a version
its own `library.json` does not carry is what made a pinned dependency resolve to the wrong code
elsewhere in this workspace.

**Build**
* Fix: `buildtime.h` is generated on the ESP-IDF path as well. `Console.cpp` has included it unconditionally since `60cade2`, but the `esp32idf` script list never ran `prepare_buildtime.py`, so a clean tree failed on a missing header in a core file -- the file is git-ignored, which is why it only showed on a fresh checkout

**Flashing**
* Feature: the upload script has a developer menu behind `-Dev`, the only way in when the fast path flashes without a prompt. Menu item 5 erases the whole flash with `picotool erase -a` or `esptool erase_flash`, behind its own confirmation; it is the path for a device whose console is dead, where the console-based wipe cannot run

**Console safety**
* Fix: `flash knx`, `flash openknx` and `mem 0x...` are refused over the diagnose group object -- anyone able to send to that group address could dump memory or trigger a flash write
* Fix: `bcu` and `bus mon` are refused there too: none of them answers over the object and some take the TP link down
* Fix: the pin commands were bracketed so that the reads `dr` and `ar` stayed reachable from the group object, where `processPinCommand` runs `std::stoi` on unvalidated text and aborts the device on anything that is not a number
* Fix: the time commands answer only on the local console, so a group-object caller got side effects without feedback

**Device and bus diagnostics**
* Feature: `bcu stat` shows the four byte-error counters the driver now separates -- framing, parity, break and overrun. The parity column is the one that matters on a bus, because the NCN encodes a bus-side bit error there, while framing and overrun describe the link to the chip
* Feature: the row appears only where the driver actually collects the status, so a run of zeroes cannot be read as "no errors" on a product that never measured
* Feature: the chip's own acknowledge and CRC state are shown; they were read from the chip and never displayed. `bcu autoack on|off` switches the acknowledge
* Feature: the acknowledge-drop counter moves into the traffic block, where it is visible without the health build flag
* Change: the NCN rails say "stale" while a busmonitor runs and "not read" before the first reply -- the driver does not poll the state in monitor mode, and the block showed power-up defaults (VBUS/V20V LOW, XTAL FAIL) on a healthy device
* Change: every new call is gated on `TPUART_API_LEVEL`, so this still builds against an older driver
* Fix: the help for `bcu poff`/`bcu pon` said "Bus power", which reads as the KNX bus supply; the command clears `V20VEN` and `DC2EN` in the NCN's ACR0, which are the transceiver's own auxiliary rails
* Change: the download counter is gone from the device information, because the knx stack no longer exposes PID 30

**Build and release tooling**
* Feature: the image facts file carries identity and provenance next to the checksums — order number, firmware name and version, MCU, ETS application id, build environment and date
* Feature: the MCU is read out of the package (`.uf2` family id, `.factory.bin`) instead of being guessed from the file extension, which never told RP2040 and RP2350 apart
* Feature: the OTA upload names the product of every device it finds, refuses a target whose chip does not match the image, and asks before sending to a different product -- `-Force` skips both checks
* Change: the device table sizes its columns from the content: a product name comes from the device, and one value wider than a fixed column shifted the whole row
* Fix: `-AutoExit` aborts a product mismatch with a message instead of sending silently, because there is nobody to answer the question
* Feature: build-time flash and knxOTA reporting — every build prints what the image costs and what fits over the bus, so a target running out of flash is visible before the linker says so
* Feature: one upload path for all targets, and delta patches generated from released packages
* Feature: `Prepare-Firmware.ps1` replaces `Extract-Images` — a menu offering full image, gzip or delta, with a real file browser (arrow keys, up to drive level, Windows/macOS/Linux) instead of a fixed path
* Feature: shared file chooser in the script UI (`OpenKNX_SelectInteractive`, `OpenKNX_PickFile`), usable by any OpenKNX script
* Feature: module release hooks by convention — a module contributes to the release without the product script knowing about it
* Feature: ESP32 knxOTA (over KNX) enabled in the scripts
* Change: the build reports are unified in layout, sorting and totals, and their output is fixed on Windows — a redirected stdout falls back to cp1252/cp850, where the box and check glyphs raised `UnicodeEncodeError`
* Change: `prepare.py` is split, with one shared style and resolver module used by all build scripts
* Fix: JS comments are dropped when embedding web assets, which the minifier left in before
* Fix: the flash script waits for the BOOTSEL volume before writing the `.uf2` instead of failing on a volume that is not mounted yet
* Fix: a `lib/` module without a remote is skipped in the dependency snapshot — it used to be listed with an empty URL, naming a commit nobody else can fetch
* Fix: mDNS discovery and the espota upload work on Windows
* Fix: `prepare.py` writes files through `with open(...)`

**Bus load**
* Feature: `OpenKNX::BusLoad` (`openknx.busLoad`) samples TP1 line occupancy once per second, keeps a one minute history and a peak, and offers `reset()`; every product with a TP line gets it without a change of its own
* Feature: occupancy comes from the TPUart frame bit counter, so a saturated line reads 100 % whatever the telegram length — a fixed bytes-per-second ceiling cannot do that
* Fix: `Statistics::getBusLoad()` resets the window it measures, so its callers took the value from each other; `WidgetKnxBcu` and both console blocks read the shared sampler now
* Fix: a BCU reset zeroes the TPUart counters — the sampler detects the backward jump and skips one sample instead of reporting a spike

**Display widgets**
* Fix: the common widgets are registered from `loop()` instead of `processAfterStartupDelay()`, which waits on a KNX parameter an unprogrammed device never provides -- the widgets were missing with no message at all
* Feature: the system-info widget gains a diagnose page: heap and stack low-water marks, watchdog resets and period, chip and clock -- it reports the minimum rather than the current value, because a leak only shows in the low-water mark
* Change: the system-info widget drops its network page and the whole `WSI_HAS_NETWORK` detection; those values belong to OFM-Network, which ships them as a widget of its own now
* Change: system-info and BCU widget put the title left and show page dots instead of an "n/N" counter, and keep clear of the manager corner

**Firmware**
* Feature: ESP32 stamps the OpenKNX identity into the app image, and the combined-image report is corrected
* Feature: `pausePeriodicSave()` suspends periodic flash writes, for code paths that must not be interrupted by one
* Feature: human-readable stat formatting plus 32-bit counter-wrap handling for widgets — 8 to 10 digit bus counters overran the 21-character OLED rows
* Change: raw `MASK_VERSION` checks are replaced by the semantic layer from the knx stack
* Change: the "console disabled" notice names the external session holding it, in red
* Fix: uptime rollover, unreadable bus counters and unchecked module registration
* Doc: the Info-LED help text says the count and labeling are device-dependent


* Update: Increase minimum OpenKNXproducer version to 4.3.12 (from 4.3.5 in Common.share.xml)
  * Resulting changes:
    * Disable ReadOnInitFlag for KOs
    * Small HOTFIX: DPT 251.600 (RGBW) check was incorrect (expected 4 Bytes, correct are 6 Bytes).
* Fix (ESP32): GPIO expanders did not build on ESP32 — `GPIO::Manager::init()` called `setSDA()`/`setSCL()`, which only exist in arduino-pico. On ESP32 the pins are now passed to `OPENKNX_GPIO_WIRE.begin(sda, scl)` instead; the `OPENKNX_GPIO_SDA`/`OPENKNX_GPIO_SCL` defines are unchanged

## 1.9.1: 2026-08-14

* Hotfix: The setup script rejected an installed OpenKNXproducer 4.3.11 as outdated. The required version was written with four components while the producer reports three, so `[System.Version]` treated `4.3.11` (Revision -1) as lower than `4.3.11.0`. Both sides are padded to four components before comparing now

## 1.9.0: 2026-08-14

* Update: Increase minimum OpenKNXproducer version to 4.3.11.0
* Feature: Web console log support — additional plain byte ring buffer in `Logger` (`OPENKNX_WEBCONSOLE`, size via `OPENKNX_WEBCONSOLE_BUFSIZE`, default 4096), accessible via `ringBuf()`/`ringWritePos()`
* Feature: PSRAM helpers in `src/OpenKNX/Helper.h` for RP2350 and ESP32 — `PSRAM_MALLOC`/`PSRAM_CALLOC`/`PSRAM_REALLOC`, `psram_new()`/`psram_delete()`, `PsramAllocator<T>`, `PSRAM_DATA`/`PSRAM_CODE`
  * Fix: Correct RP2350/ESP32 PSRAM section names and macros, data silently ended up in normal RAM before
* Fix: `platformio.base.ini` now sets a dummy `lib_deps` entry, since PlatformIO re-resolves/re-downloads all included modules on every build whenever `lib_deps` is empty
* Fix: `BUILD_DATETIME`/`BUILD_TIMESTAMP` moved out of `include/versions.h` into their own generated `include/buildtime.h`, since they change on every build and previously forced a full rebuild of everything including `versions.h` (via `defines.h`) even when no module version changed
* Feature: Add `OpenKNX::Charset` (`src/OpenKNX/Charset.hpp`) — header-only `encodeUtf8()`/`decodeUtf8()` conversion between the firmware/KNX-bus encoding (ISO-8859-15) and UTF-8, for modules that expose bus/filesystem strings over a browser-facing boundary (WebSocket, HTTP) that forces UTF-8. Gated behind `#ifdef OPENKNX_CHARSET`; not enabled by OGM-Common itself
* Feature: `prepare.py` now generates `include/webassets.h` from every included module's (and the project's own) `web/assets/` folder — plain, readable `.css`/`.js`/`.svg`/`.jpg`/`.png` source files are minified and gzip-compressed into flash-resident byte arrays at build time, so modules no longer need to hand-minify web assets into C++ string literals
* Feature: Add shared HelpContext topics `BASE-ChannelSelect` and `BASE-ChannelSuspended` for modules using the tab-based Kanalauswahl pattern, replacing per-module placeholder help texts
* Change: `PT-Suspended` now renders as a Ja/Nein selection instead of a checkbox, consistent with `PT-OnOffYesNo`
* Feature (ETS): Horizontal ruler in front of section headlines
* Feature (ETS): Generic JS helper to mark inactive channels (`MarkInactiveChannel`), the value representing "inactive" is taken from the context so it works for any numeric UI control
* Fix (ESP32): Logger uses a recursive mutex/semaphore, fixing log output from nested/multi-task contexts
* Change: `prepare.py` section headlines now match the names of the generated files
* Doc: `openknx-channelselect` skill updated to reference the new shared HelpContext values instead of `Empty`/`%DOC%` placeholders; note added to `BASE-Verfuegbare-Kanaele` that it is the legacy pattern superseded by the Kanalauswahl-Tabelle
* Breaking: Trace filter reworked. The regex dependency (`nickgammon/Regexp`, ~80&nbsp;kB flash when tracing was enabled) is removed and replaced by a compact custom matcher. `OPENKNX_TRACE1..5` are dropped in favor of a single `OPENKNX_TRACE` define holding one or more filters separated by `;` (e.g. `OPENKNX_TRACE=Test1<1-4>;Test2<8>`). New filter syntax `PREFIX<SUB>`: prefix exact or `*` suffix wildcard; optional `<sub>` with exact value, numeric range (`1-19`), comma list (`4,5,7`, combinable `1-5,9`), or `*` suffix wildcard. A filter without `<sub>` ignores the sub part. Existing regex filters must be migrated (e.g. `OPENKNX_TRACE1=Common.*` → `OPENKNX_TRACE=Common*`).
* Fix: `uptime()` race condition causing wrong uptime display (~49d) on multi-core/interrupt contexts — read path no longer writes shared state
* Fix: USB MSC support in Common.cpp
* Breaking: Remove ETS-ModuleDef of KommentarModule, not used anymore since introduction of ConfigTransfer (not compatible).
* Add suffix °N/°O to location coordinates 
* Change: Generate build timestamp in prepare hook and use ISO-DateTime format in console instead of __DATE__/__TIME__
* Refactor: Optimize CPU temperature logging and remove unused ADC temperature sensor initialization
* Update: RP2040 Environment
  * platform: platform-raspberrypi cc24cfef37ed22ca9f2a6aead28c2deb76c39f24 to 79ce473e1a8010ed8096388690222fd2c94858a6
  * platform_packages: framework-arduinopico 5.4.4 to 5.6.0

## 1.8.1: 2026-05-18

* Fix Vcc2 shut off with DCU initialization (POWER_SAVE_PIN vs. SAVE_POWER_PIN)

## 1.8.0: 2026-05-15

* Fix: DST switch for NTP client
* Fix/Change: Save data to flash in case of restart from console
* Fix: SunRise-Calculation was wrong for UTC-day-change (this was not an issue for Europe, but e.g. New Zealand)
  * Fix/Change: Support negative hours in TimeDate constructor
* Update: Increase minimum OpenKNXproducer version to 4.3.5
  * Fix: Suppress warnings for intentionally overlapping parameters
* Feature: Indicate unconfigured and PA not set via PROG-LED
  * Extension (LED): Add count and repeat to effect "Flash"  
* Fixes/Extension: FunctionPropertyWrapper
* Fix: Small memory leak in case of calling readFlash for modules without saved data
* Fix: Restore script showed success when git checkout failed
* Refactor: Restore script
* Doc: Application Description
  * Add list of KOs with short description
  * Cleanup TOC, Minor Fixes and Improvements
  * Link from Readme
* Doc: Improve build script parameter documentation, no coding change


## 1.7.2: 2026-03-02

- Fix: Default brightness of dimmable GPIO-LED 100/255 changed to 255/255

## 1.7.1: 2026-02-26

- Fix: Parameter Memory Management for StatusLED Implementation
- Fix: resultLength calculation in functionPropertyWrapper
- Fix: removed non-existing apdu object in Common.script.js
- Revert: ESP32 Framework back to 3.2.1 (IDF 5.4)

## 1.7: 2026-02-16

- Fix: remove OpenKNXHardware.h (replaced by OGM-HardwareConfig)
- Fix: skip initialization of USB-Port in case of Debug-Session
- Feature: improve Build-Script with optional debug build and named parameters (full compatible with old calls to this script)
- Feature: Force OpenKNXproducer to version 3.13.6
- Hotfix: Modules list could not handle Modules inserted in between during ETS update - is fixed now. All modules are selected after Update.
- Fix: Typo "Noitz" -> "Notiz" (PR#130)
- Feature: Restore script can also restore just links (without checkout) (PR#129)
- Feature: propertyFunctionWrapper for APDU aware execution of propertyFunction calls. Not for productive use yet!

## 1.6.0: 2026-01-28

* Feature: New StatusLED Implementation see README_LED.md
* Breaking changes: Old LED Apis (openknx.progLed and openknx.infoLeds) no longer available!
* Update: RP2040 Framework update to 5.4.4
  * Fix: Network bug causing crashes (with 5.4.3)
* Update: ESP32 Framework update to 3.3.6
* Feature: Vcc2 shut off with DCU (POWER_SAVE_PIN)
* Fix: Estimation of KNX-data size in flash layout display/check
* Extension: Time Implementation
  * Add inaccurate time flag
  * Add callback support to time manager
* Change (Minor) in ETS-App: Label for column "Aktiv" in module overview 
* Add Build Check Action

## 1.5.1: 2025-10-24

* Fix: Do not init the serial in constructor of *Logger*
* Update: RP2040 Environment
  * platform: platform-raspberrypi 6af38e2 to 22a4cc6
  * platform_packages: framework-arduinopico 4.6.0 to 5.4.1
* Update Readme: Add `OPENKNX_DEBUGGER` and reorder configuration table
* Feature: Add `-DebugBuild` switch for Build-Step.ps1 script
* Fix: Solve Escaping Warning for Regex in Python Build Helper Scripts
* Change (Build): `lib_ldf_mode` from `deep+` to `chain`
  * Add include path for OGM-HardwareConfig in platformio.base.ini
* Fix: (Release Preprocess) Ensure Microsoft.PowerShell.Archive >= 1.2.3.0 to prevent broken release zips with `\` as path seperator 
* Chore (Build): Removed unused lib_ignore entry in PIO-config
* Refactor: Build flags and configurations for consistency across platforms
* Change `OPENKNX_LOOPTIME_WARNING` setting: Disable as default and set to 7 (ms) for debug builds
* Fix: ESP firmware upload
  * Fix: ESP firmware upload script failed with new release structure
  * Fix: improve texts for ESP-USB-Firmware-Upload
  * Fix: suppress KNX-Upload-Firmware script for ESP as long as KNX-OTA is not available
* Support new usb handler for ARDUINO_ARCH_RP2040 in common for UsbExchange


## 1.5.0: 2025-10-15

* Fix: Require FileTransferClient minimal version 0.2.7 to prevent issues with firmware update resume
* Fix/Doc: Replace parameters migrated from LOG (to BASE) in Readme 
* Fix: Log and diagnose messages raise only a fatalError "Buffer overflow", if this really happens
* Feature: Sun elevation and azimuth can be displayed by diagnose object (`sun elevation`, `sun azimuth`)
* Feature: Release scripts build now a release with a directory structure per supported Hardware. Provides better overview.
* Feature: Common contains new module overview with disable option and version information. Needs OpenKNXproducer 3.11.0 at least.
* Fix: Looptime-Warning-Message is now yellow instead red
* Feature: logWarning/logWarningP/logHexWarning/logHexWarningP exist now with yellow output
* Fix: Error-output for overlapping memory areas was broken/mixed with layout output
* Refactor: Rename variable 'ddl' to 'dll' for consistency and clarity in data link layer operations and remove unnecessary NCN5120 check
* Update: Platform for ESP from 54.03.20 to 54.03.21-2
* Feature: Add new intern BCU debug mode
  <!-- Use `getSecondaryDataLinkLayer()` with `MASK_VERSION == 0x091A` for BCU debug -->
* Feature: ARGB support for RP2040 using PIO (#97)
* Feature: Added support for PCA9554 port expander open openknx.gpio. (#98)
* Refactor: Add helper functions to logger for separator and header lines
* Feature: Add console commands to delete and dump files
  * `fs del <file>`: Delete a file
  * `fs dmp <file>`: "Dump a file"
  <!-- Fix compile error on ESP32 platform -->
* Feature: Added option to digital read/write PINs in HEX via console and OpenKNX GPIO implementation.
* Fix: Context-help "In Betrieb" was overwritten by producer documentation build, as not updated in application description.
  This could result in outdated help-text.
* Fix: Prevent issues with ADC usage, after showing console info;
       use "analogReadTemp()" to read CPU temperature.
* Change: Use UTF8-encoding for dependencies.txt file
* Fix: Console help-line for `aw` presented wrong value range (maximum is 4095)  


## 1.4.3: 2025-07-16
* Hotfix: Day-of-week was not set in `DateTime::toTm(..)` provided by *new time implementation* of v1.4.0
  * *Impact:* This resulted in malfunction of timer-switches in Logikmodul (Kanaltype "ZEITSCHALTUHR") depending on day
* See also changes of 1.4.0 especially the new KO-numbers.

## 1.4.2: 2025-07-14
* Feature: Enhancement for extended "In Betrieb"
* See also changes of 1.4.0 especially the new KO-numbers.

## 1.4.1: 2025-07-07
Note: *No* functional changes, affects documentation integrated in UI only

* Fix: Broken Context-Help in ETS-App
  (Wrong Text for "In Betrieb senden",
   Missing Links in OpenKNX Intro,
   Missing Value Description for "Sommerzeit ermitteln durch",
   Erweitertes Speichern)
* Improvement: Context-Help in ETS-App
* Change: Use Standard Build Process for Context-Help
* Remove unused FIRMWARE_VARIANT

## 1.4.0: 2025-07-04
* Breaking: Changes in KO-Numbers 2 to 19. See [table below](#änderung-von-zentralen-ko-nummern-mit-v14x).
* Feature: New KNX TPUart Implementation of KNX Stack
* Feature: New time implementation and sun calculation
  * Change: Use DPT19 as new default configuration for time-input.
    *Compatibility/Migration Note:* To use config-transfer from old versions without migration to new default, use "Parameterauswahl=auch Standardwerte" for export
* Feature: Adds new GPIO abstraction layer with support for expander
* Feature: Adds IP-OTA
* Feature: Adds for new DEVICE_INIT() Macro
* Feature: Adds RP2350 Support
* Feature: Adds support for combined TP/IP in knxprod
* Feature: ESP23 Support
  * Update Platform to IDFv5 with Platform 54.03.20
  * Use RTOS Timer instead TimerInterrupts
  * Adds new Partition layouts
  * Use LittleFS
  * Use new RMT Library
  * Support installscripts
* Refactor: Watchdog handling with auto erase on too many restarts
* Update: RP2040 Platform to Core 4.6.0 + Rpi Base Platform
* Optimizes: Stop build process on overlapping flash memory regions
  * Refactor: Memory Layout-Check/Info in Build-Process
* Optimizes PIO scripts
* Optimizes led handling
* Optimizes device console
* Adds some icons

### Änderung von zentralen KO-Nummern mit v1.4.x

> ***Tipp:*** Referenzen auf KO-Nummern im Logikmodul können leicht 
> auf der Seite "*Übersicht interne Verknüpfungen*" angepasst werden.

| KO                               | alt | NEU (ab 1.4.0) | Kommentar                                |
|----------------------------------|-----|----------------|------------------------------------------|
| In Betrieb                       | 1   | 1              | (unverändert)                            |
| **Uhrzeit**                      | 2 * | 2              | KO war bislang geteilt mit Uhrzeit/Datum |
| Datum                            | 3   | 3              | (unverändert)                            |
| **Uhrzeit/Datum**                | 2 * | 4              | Separiert von KO 2 Uhrzeit.              |
| **Sommerzeit aktiv**             | 10  | 5              |                                          |
| **Speichern**                    | 11  | 6              |                                          |
| Diagnose                         | 7   | 7              | (unverändert)                            |
| **Urlaub**                       | 4   | 15             |                                          |
| **Welcher Feiertag ist heute?**  | 5   | 16             |                                          |
| **Welcher Feiertag ist morgen?** | 6   | 17             |                                          |
| **LED sperren**                  | 8   | 18             | (nicht in allen Applikationen vorhanden) |
| **Buzzer sperren**               | 9   | 19             | (nicht in allen Applikationen vorhanden)        |


\*: Doppelbelegung der KO-Nummer. Einblendung war abhängig von Konfiguration.

## 1.2.1: 2024-11-18
* Update: RP2040 Platform to Core 4.1.1 + Rpi Base Platform
* Add: Now allows you to delete the KNX or OpenKNX flash area on all platforms
* Some small optimizations
* Add: ESP32: Collect stack size statistics
* Add: ESP32: support printCore
* Update: Scripts
* App: Support SerialLED (Neopixel/WS2812)
* Add: PSRAM info in 'm' console command
* Fix: delayTimerInit could return 0 in case of overflow between calls to millis()
* Fix: Misspelling writeDiagenoseKo -> writeDiagnoseKo
* Change: Set JLink und JTAG as default debugger and USB as default uploader
* Add: Support ESP32 JTAG

## 1.2.0: 2024-07-02
* Update: RP2040 Platform to Core 3.9.3
* Update: ESP Platform to Core 6.7.0
* Enhancement: Scripts
* Add: New Icons
* Enhancement: New ETS UI
* Support longer commands than 14 chars (only console not diagnose ko)
* Support for new knx stack (openknx repo)
  * Breaking changes: No support for old knx stack
  * Add: New commands for bcu information
  * Add: Support DMA & IRQ for new knx stack

## 1.1.2: 2024-02-10
* Update: RP2040 & ESP32 Platform
* Refactor/Feature: Refactor watchdog to support ESP32

## 1.1.1: 2024-01-29 -- 2024-02-09
* Improvement: Project restore scripts
* Fix: Add typedef for missing pin_size_t for esp32 
* Fix: Button handling when using short, long and double click

## 1.1.0: 2024-01-09 -- 2024-01-14
* Enhancement: Reusable scripts for compatibility with MacOS and Linux.
* Fix: Sent heartbeat in configured format
* Breaking changes: Remove the call addModule with pointer to prevent duplicate instances.
* Feature: Adds new periodic & manual (by ko) save
* Change: Adds a reserved memory area

## 1.0.12: 2024-01-03 -- 2024-01-08
* ESP32 Platform: Call `setup1()` and `loop1()`
* Change: Use mklink as default in new restore-script
* Feature: (RP2040) Save data to flash before upload
* Fix: Watchdog in fatal error
* Fix: Flash Write Protection
* Baggages: Adds new icons

## 1.0.11: 2023-12-21 -- 2023-12-29
* Feature: Include network-state in 8-bit heartbeat ko
* Improvement: Watchdog and memory debug output
* Baggages: Adds new icons
* Documentation: Add application description for common
* Change: Symbolic link creation in new restore-script
* SAMD Platform:
  * Complete watchdog support
  * Fix LED-handling
  * Updated generic SAMD upload script

## 1.0.10: 2023-11-13 - 2023-12-21
* Feature: Adds a recovery mode during bootup by hold knx prog button
* Refactor/Fix: Watchdog
* Refactor: Heartbeat with new advanced info
* Refactor: Support new producer with Common.share.xml
  * Move many XML Elements from Logicmodule into Common.share.xml
  * Adds new multiline comment input
  * New base-config-page
* Feature: Buildscript support MacOS and Linux
* Baggages: Adds new icons
* Update: RP2040 Platform to Core 3.6.2
* Refactor: Uses new helper method from RP2040 Platform Core
* Feature: Show and monitor the stack usage
* Update: Uses write callbacks in knx stack as default to save settings by the own flash writer
* Extension: Include binary build time to version output of console
* Change: Show uptime before console-outputs (and disable time output by pio)
* Hardware-Support:
  * Add OKNXHW_REG1_BASE_V0
  * Add REG2

## 1.0.9: 2023-11-06 - 2023-11-12
* Feature: Adds an inverted mode for the led effect activity
* Feature: Adds custom button processing to offer short click, long click and double click

## 1.0.8: 2023-10-30 - 2023-11-05
* Feature: Allows to pass a module reference to addModule
* Improvement: Swap the meaning of info1Led and progLed in the boot phase.

## 1.0.7: 2023-10-23 - 2023-10-29
* Refactor: Led handling to optimize ram usage
* Refactor: Rename openknx.ledInfo to openknx.led1Info and also the defines
* Feature: Add 2 new infoLeds. Now we have 4 global Leds: progLed, info1Led, info2Led, info3Led
* Feature: Add new Led effects (Activity & Flash)
* Refactor: Remove -D LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS

## 1.0.6: 2023-10-13
* Refactor: Force an overload of the name() and version(). 
* Feature: Hide modules with blank version on the console.

## 1.0.5: 2023-10-02 -- 2023-10-07
* Restore-Scripts:
  * Use knx from OpenKNX-Repository
  * Improve error-handling: Restore working-directory before exit 1
* Update: Platform raspberrypi
* Refactor: TimerInterrupt for RP2040 platform using SDK API
* Refactor: Optimization of the logger to use less memory on the stack.
* Feature: Stack usage monitoring

## 1.0.4: 2023-09-29 -- 2023-10-01
* Stability: Initialize string buffers 
* Stability/Improvement: Read hardware serial once on startup only, not in console output
* Fix: [Skip processModulesLoop with 0 modules (for testing)](https://github.com/OpenKNX/OGM-Common/commit/c663b51cadbfc9ae9b2c9ca61a919f20e5632598)

## 1.0.3: 2023-09-22 -- 2023-09-26
* Fix: Diagnose-KO with 14 characters input ([Quick-Fix for Missing \0 at End of 14 Characters Strings](https://github.com/OpenKNX/OGM-Common/commit/ec3a31ef96d1b1af7b5327e356de78fcdc092293))
* Improvement: Console-Output
  * Version-Listing on Console
  * Increase log-level in loading module data from debug to info
  * Show watchdog status on console
* Refactor prepary.py
* Update: [RP2040 Platform to Core 3.6.0](https://github.com/OpenKNX/OGM-Common/commit/5c081a0c81b395fbeda5c7bbac025722a9efb410) with lwIP Support
* Feature: [Add "ETS application version" in versions.h](https://github.com/OpenKNX/OGM-Common/commit/42a2740591fc96ce435c4aa184e3582ee7b7f149)
* Fix: Add help-entry for uptime
* Change: Increase `OPENKNX_LOOPTIME_WARNING` default from 6ms to 7ms
* Extension: New Hardware `OKNXHW_REG1_CONTROLLER2040` and `OKNXHW_REG1_IPCONTROLLER2040`

## 1.0.2: 2023-09-04 -- 2023-09-17
* Feature: Show uptime in human-readable form (days hh:mm:ss)
* Fix/Extension: `Common:uptime(..)` provides uptime in seconds, detecting overflow of `millis()` for correct uptimes >49 days
* Fix: [Process command 'versions' from console only](https://github.com/OpenKNX/OGM-Common/commit/9576370424712666d5b24dcefb23062d2d4a4ca2)
* Fix: [Automatic reset on RP2040 using DualCore did not work.](https://github.com/OpenKNX/OGM-Common/commit/63258937b93dc46fbac401834a46a01faaae9b47) Solved by patch included in https://github.com/maxgerhardt/platform-raspberrypi.git#5208e8c
* Fix: [Show versions only for request from console](https://github.com/OpenKNX/OGM-Common/commit/76935a146fda62f25f3538abe97fe69110f2ed44)
* Feature #9: Optional Runtime Statistics Measurement to Support Development, available setting `OPENKNX_RUNTIME_STAT`
* Fix: [Update RP2040 platform and platform_packages](https://github.com/OpenKNX/OGM-Common/commit/90ebc9d4f7618a905ca8a38617a844ee7f130452)
* Extension: Introduce `Common::freeLoopIterate(..)` as Helper for module / channel iteration respecting `freeLoopTime()`
  * Change: At least one module will be called in `Common::loop()`

## 1.0.1: 2023-08-11 -- 2023-09-03
* Improvement: [Show commons version in console and show running firmware version first](https://github.com/OpenKNX/OGM-Common/commit/05c58c60fa8ffc4406be2c6e058be3110b2430e3)
* Documentation: Reformat Tables in README
* Fix: [Parsing of application number](https://github.com/OpenKNX/OGM-Common/commit/86f6f77f338c310ac4f687f9650e90a829e16a35)
* Build-Process: Handling of include header-files outside commons-directory
  * Update release scripts
  * Delete files from commons
* Improvement: Suppress duplicate lines in memory dump
* Portability: [Use flash slots + slot version only on rp2040](https://github.com/OpenKNX/OGM-Common/commit/6559af514074ed079aaade39f49230b42e5bcb62)
* Feature: [Use callbacks in knx stack for register external flash system](https://github.com/OpenKNX/OGM-Common/commit/c725bd94c6a3111cf39155e2c544984510b00bdc)

## 1.0.0: (2023-08-11)
New modular commons architecture seen as stable and recommended for usage in all OpenKNX firmwars. 