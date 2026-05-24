# Release Notes

## 1.8.x
### Breaking Changes
- none

### Feature
- none

### Bug
- Fix: `uptime()` race condition causing wrong uptime display (~49d) on multi-core/interrupt contexts — read path no longer writes shared state

## 1.8.1
### Breaking Changes
- none

### Feature
- none

### Bug
- Fix Vcc2 shut off with DCU initialization (POWER_SAVE_PIN vs. SAVE_POWER_PIN)

## 1.8.0
### Breaking Changes
- none

### Feature
- Update: Increase minimum OpenKNXproducer version to 4.3.5
- Feature: Indicate unconfigured and PA not set via PROG-LED
- Extension (LED): Add count and repeat to effect "Flash"
- Refactor: Restore script
- Doc: Application Description
- Add list of KOs with short description
- Link from Readme
- Doc: Improve build script parameter documentation, no coding change

### Bug
- Fix: DST switch for NTP client
- Fix/Change: Save data to flash in case of restart from console
- Fix: SunRise-Calculation was wrong for UTC-day-change (this was not an issue for Europe, but e.g. New Zealand)
- Fix/Change: Support negative hours in TimeDate constructor
- Fix: Suppress warnings for intentionally overlapping parameters
- Fixes/Extension: FunctionPropertyWrapper
- Fix: Small memory leak in case of calling readFlash for modules without saved data
- Fix: Restore script showed success when git checkout failed
- Cleanup TOC, Minor Fixes and Improvements

## 1.7.2
### Breaking Changes
- none

### Feature
- none

### Bug
- Fix: Default brightness of dimmable GPIO-LED 100/255 changed to 255/255

## 1.7.1
### Breaking Changes
- none

### Feature
- Revert: ESP32 Framework back to 3.2.1 (IDF 5.4)

### Bug
- Fix: Parameter Memory Management for StatusLED Implementation
- Fix: resultLength calculation in functionPropertyWrapper
- Fix: removed non-existing apdu object in Common.script.js

## 1.7
### Breaking Changes
- none

### Feature
- Feature: Force OpenKNXproducer to version 3.13.6
- Feature: Restore script can also restore just links (without checkout) (PR#129)
- Feature: propertyFunctionWrapper for APDU aware execution of propertyFunction calls. Not for productive use yet!

### Bug
- Fix: remove OpenKNXHardware.h (replaced by OGM-HardwareConfig)
- Fix: skip initialization of USB-Port in case of Debug-Session
- Feature: improve Build-Script with optional debug build and named parameters (full compatible with old calls to this script)
- Hotfix: Modules list could not handle Modules inserted in between during ETS update - is fixed now. All modules are selected after Update.
- Fix: Typo "Noitz" -> "Notiz" (PR#130)

## 1.6.0
### Breaking Changes
- Breaking changes: Old LED Apis (openknx.progLed and openknx.infoLeds) no longer available!

### Feature
- Feature: New StatusLED Implementation see README_LED.md
- Update: RP2040 Framework update to 5.4.4
- Update: ESP32 Framework update to 3.3.6
- Feature: Vcc2 shut off with DCU (POWER_SAVE_PIN)
- Extension: Time Implementation
- Add inaccurate time flag
- Add callback support to time manager
- Change (Minor) in ETS-App: Label for column "Aktiv" in module overview
- Add Build Check Action

### Bug
- Fix: Network bug causing crashes (with 5.4.3)
- Fix: Estimation of KNX-data size in flash layout display/check

## 1.5.1
### Breaking Changes
- none

### Feature
- Update: RP2040 Environment
- platform: platform-raspberrypi 6af38e2 to 22a4cc6
- platform_packages: framework-arduinopico 4.6.0 to 5.4.1
- Change (Build): `lib_ldf_mode` from `deep+` to `chain`
- Add include path for OGM-HardwareConfig in platformio.base.ini
- Chore (Build): Removed unused lib_ignore entry in PIO-config
- Refactor: Build flags and configurations for consistency across platforms
- Support new usb handler for ARDUINO_ARCH_RP2040 in common for UsbExchange

### Bug
- Fix: Do not init the serial in constructor of *Logger*
- Update Readme: Add `OPENKNX_DEBUGGER` and reorder configuration table
- Feature: Add `-DebugBuild` switch for Build-Step.ps1 script
- Fix: Solve Escaping Warning for Regex in Python Build Helper Scripts
- Fix: (Release Preprocess) Ensure Microsoft.PowerShell.Archive >= 1.2.3.0 to prevent broken release zips with `\` as path seperator
- Change `OPENKNX_LOOPTIME_WARNING` setting: Disable as default and set to 7 (ms) for debug builds
- Fix: ESP firmware upload
- Fix: ESP firmware upload script failed with new release structure
- Fix: improve texts for ESP-USB-Firmware-Upload
- Fix: suppress KNX-Upload-Firmware script for ESP as long as KNX-OTA is not available

## 1.5.0
### Breaking Changes
- none

### Feature
- Feature: Sun elevation and azimuth can be displayed by diagnose object (`sun elevation`, `sun azimuth`)
- Feature: Release scripts build now a release with a directory structure per supported Hardware. Provides better overview.
- Feature: Common contains new module overview with disable option and version information. Needs OpenKNXproducer 3.11.0 at least.
- Feature: logWarning/logWarningP/logHexWarning/logHexWarningP exist now with yellow output
- Refactor: Rename variable 'ddl' to 'dll' for consistency and clarity in data link layer operations and remove unnecessary NCN5120 check
- Update: Platform for ESP from 54.03.20 to 54.03.21-2
- Feature: ARGB support for RP2040 using PIO (#97)
- Feature: Added support for PCA9554 port expander open openknx.gpio. (#98)
- Refactor: Add helper functions to logger for separator and header lines
- Feature: Add console commands to delete and dump files
- `fs del <file>`: Delete a file
- `fs dmp <file>`: "Dump a file"
- Feature: Added option to digital read/write PINs in HEX via console and OpenKNX GPIO implementation.
- Change: Use UTF8-encoding for dependencies.txt file

### Bug
- Fix: Require FileTransferClient minimal version 0.2.7 to prevent issues with firmware update resume
- Fix/Doc: Replace parameters migrated from LOG (to BASE) in Readme
- Fix: Log and diagnose messages raise only a fatalError "Buffer overflow", if this really happens
- Fix: Looptime-Warning-Message is now yellow instead red
- Fix: Error-output for overlapping memory areas was broken/mixed with layout output
- Feature: Add new intern BCU debug mode
- Fix: Context-help "In Betrieb" was overwritten by producer documentation build, as not updated in application description.
- Fix: Prevent issues with ADC usage, after showing console info;
- Fix: Console help-line for `aw` presented wrong value range (maximum is 4095)

## 1.4.3
### Breaking Changes
- none

### Feature
- *Impact:* This resulted in malfunction of timer-switches in Logikmodul (Kanaltype "ZEITSCHALTUHR") depending on day
- See also changes of 1.4.0 especially the new KO-numbers.

### Bug
- Hotfix: Day-of-week was not set in `DateTime::toTm(..)` provided by *new time implementation* of v1.4.0

## 1.4.2
### Breaking Changes
- none

### Feature
- Feature: Enhancement for extended "In Betrieb"
- See also changes of 1.4.0 especially the new KO-numbers.

### Bug
- none

## 1.4.1
### Breaking Changes
- none

### Feature
- Improvement: Context-Help in ETS-App
- Change: Use Standard Build Process for Context-Help
- Remove unused FIRMWARE_VARIANT

### Bug
- Fix: Broken Context-Help in ETS-App

## 1.4.0
### Breaking Changes
- Breaking: Changes in KO-Numbers 2 to 19. See [table below](#änderung-von-zentralen-ko-nummern-mit-v14x).

### Feature
- Feature: New KNX TPUart Implementation of KNX Stack
- Feature: New time implementation and sun calculation
- Change: Use DPT19 as new default configuration for time-input.
- Feature: Adds new GPIO abstraction layer with support for expander
- Feature: Adds IP-OTA
- Feature: Adds for new DEVICE_INIT() Macro
- Feature: Adds RP2350 Support
- Feature: Adds support for combined TP/IP in knxprod
- Feature: ESP23 Support
- Update Platform to IDFv5 with Platform 54.03.20
- Use RTOS Timer instead TimerInterrupts
- Adds new Partition layouts
- Use LittleFS
- Use new RMT Library
- Support installscripts
- Refactor: Watchdog handling with auto erase on too many restarts
- Update: RP2040 Platform to Core 4.6.0 + Rpi Base Platform
- Optimizes: Stop build process on overlapping flash memory regions
- Refactor: Memory Layout-Check/Info in Build-Process
- Optimizes PIO scripts
- Optimizes led handling
- Optimizes device console
- Adds some icons

### Bug
- none

## 1.4.x
### Breaking Changes
- none

### Feature
- none

### Bug
- none

## 1.2.1
### Breaking Changes
- none

### Feature
- Update: RP2040 Platform to Core 4.1.1 + Rpi Base Platform
- Add: Now allows you to delete the KNX or OpenKNX flash area on all platforms
- Some small optimizations
- Add: ESP32: Collect stack size statistics
- Add: ESP32: support printCore
- Update: Scripts
- App: Support SerialLED (Neopixel/WS2812)
- Add: PSRAM info in 'm' console command
- Add: Support ESP32 JTAG

### Bug
- Fix: delayTimerInit could return 0 in case of overflow between calls to millis()
- Fix: Misspelling writeDiagenoseKo -> writeDiagnoseKo
- Change: Set JLink und JTAG as default debugger and USB as default uploader

## 1.2.0
### Breaking Changes
- Breaking changes: No support for old knx stack

### Feature
- Update: RP2040 Platform to Core 3.9.3
- Update: ESP Platform to Core 6.7.0
- Enhancement: Scripts
- Add: New Icons
- Enhancement: New ETS UI
- Support longer commands than 14 chars (only console not diagnose ko)
- Support for new knx stack (openknx repo)
- Add: New commands for bcu information
- Add: Support DMA & IRQ for new knx stack

### Bug
- none

## 1.1.2
### Breaking Changes
- none

### Feature
- Update: RP2040 & ESP32 Platform
- Refactor/Feature: Refactor watchdog to support ESP32

### Bug
- none

## 1.1.1
### Breaking Changes
- none

### Feature
- Improvement: Project restore scripts

### Bug
- Fix: Add typedef for missing pin_size_t for esp32
- Fix: Button handling when using short, long and double click

## 1.1.0
### Breaking Changes
- Breaking changes: Remove the call addModule with pointer to prevent duplicate instances.

### Feature
- Enhancement: Reusable scripts for compatibility with MacOS and Linux.
- Feature: Adds new periodic & manual (by ko) save
- Change: Adds a reserved memory area

### Bug
- Fix: Sent heartbeat in configured format

## 1.0.12
### Breaking Changes
- none

### Feature
- ESP32 Platform: Call `setup1()` and `loop1()`
- Change: Use mklink as default in new restore-script
- Feature: (RP2040) Save data to flash before upload
- Baggages: Adds new icons

### Bug
- Fix: Watchdog in fatal error
- Fix: Flash Write Protection

## 1.0.11
### Breaking Changes
- none

### Feature
- Feature: Include network-state in 8-bit heartbeat ko
- Baggages: Adds new icons
- Documentation: Add application description for common
- Change: Symbolic link creation in new restore-script
- SAMD Platform:
- Complete watchdog support
- Updated generic SAMD upload script

### Bug
- Improvement: Watchdog and memory debug output
- Fix LED-handling

## 1.0.10
### Breaking Changes
- none

### Feature
- Feature: Adds a recovery mode during bootup by hold knx prog button
- Refactor: Heartbeat with new advanced info
- Refactor: Support new producer with Common.share.xml
- Move many XML Elements from Logicmodule into Common.share.xml
- Adds new multiline comment input
- New base-config-page
- Feature: Buildscript support MacOS and Linux
- Baggages: Adds new icons
- Update: RP2040 Platform to Core 3.6.2
- Refactor: Uses new helper method from RP2040 Platform Core
- Feature: Show and monitor the stack usage
- Update: Uses write callbacks in knx stack as default to save settings by the own flash writer
- Extension: Include binary build time to version output of console
- Change: Show uptime before console-outputs (and disable time output by pio)
- Hardware-Support:
- Add OKNXHW_REG1_BASE_V0
- Add REG2

### Bug
- Refactor/Fix: Watchdog

## 1.0.9
### Breaking Changes
- none

### Feature
- Feature: Adds an inverted mode for the led effect activity
- Feature: Adds custom button processing to offer short click, long click and double click

### Bug
- none

## 1.0.8
### Breaking Changes
- none

### Feature
- Feature: Allows to pass a module reference to addModule
- Improvement: Swap the meaning of info1Led and progLed in the boot phase.

### Bug
- none

## 1.0.7
### Breaking Changes
- none

### Feature
- Refactor: Led handling to optimize ram usage
- Refactor: Rename openknx.ledInfo to openknx.led1Info and also the defines
- Feature: Add 2 new infoLeds. Now we have 4 global Leds: progLed, info1Led, info2Led, info3Led
- Feature: Add new Led effects (Activity & Flash)
- Refactor: Remove -D LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS

### Bug
- none

## 1.0.6
### Breaking Changes
- none

### Feature
- Refactor: Force an overload of the name() and version().
- Feature: Hide modules with blank version on the console.

### Bug
- none

## 1.0.5
### Breaking Changes
- none

### Feature
- Restore-Scripts:
- Use knx from OpenKNX-Repository
- Update: Platform raspberrypi
- Refactor: TimerInterrupt for RP2040 platform using SDK API
- Refactor: Optimization of the logger to use less memory on the stack.
- Feature: Stack usage monitoring

### Bug
- Improve error-handling: Restore working-directory before exit 1

## 1.0.4
### Breaking Changes
- none

### Feature
- Stability: Initialize string buffers
- Stability/Improvement: Read hardware serial once on startup only, not in console output

### Bug
- Fix: [Skip processModulesLoop with 0 modules (for testing)](https://github.com/OpenKNX/OGM-Common/commit/c663b51cadbfc9ae9b2c9ca61a919f20e5632598)

## 1.0.3
### Breaking Changes
- none

### Feature
- Improvement: Console-Output
- Version-Listing on Console
- Show watchdog status on console
- Refactor prepary.py
- Update: [RP2040 Platform to Core 3.6.0](https://github.com/OpenKNX/OGM-Common/commit/5c081a0c81b395fbeda5c7bbac025722a9efb410) with lwIP Support
- Feature: [Add "ETS application version" in versions.h](https://github.com/OpenKNX/OGM-Common/commit/42a2740591fc96ce435c4aa184e3582ee7b7f149)
- Change: Increase `OPENKNX_LOOPTIME_WARNING` default from 6ms to 7ms
- Extension: New Hardware `OKNXHW_REG1_CONTROLLER2040` and `OKNXHW_REG1_IPCONTROLLER2040`

### Bug
- Fix: Diagnose-KO with 14 characters input ([Quick-Fix for Missing \0 at End of 14 Characters Strings](https://github.com/OpenKNX/OGM-Common/commit/ec3a31ef96d1b1af7b5327e356de78fcdc092293))
- Increase log-level in loading module data from debug to info
- Fix: Add help-entry for uptime

## 1.0.2
### Breaking Changes
- none

### Feature
- Feature: Show uptime in human-readable form (days hh:mm:ss)
- Feature #9: Optional Runtime Statistics Measurement to Support Development, available setting `OPENKNX_RUNTIME_STAT`
- Extension: Introduce `Common::freeLoopIterate(..)` as Helper for module / channel iteration respecting `freeLoopTime()`
- Change: At least one module will be called in `Common::loop()`

### Bug
- Fix/Extension: `Common:uptime(..)` provides uptime in seconds, detecting overflow of `millis()` for correct uptimes >49 days
- Fix: [Process command 'versions' from console only](https://github.com/OpenKNX/OGM-Common/commit/9576370424712666d5b24dcefb23062d2d4a4ca2)
- Fix: [Automatic reset on RP2040 using DualCore did not work.](https://github.com/OpenKNX/OGM-Common/commit/63258937b93dc46fbac401834a46a01faaae9b47) Solved by patch included in https://github.com/maxgerhardt/platform-raspberrypi.git#5208e8c
- Fix: [Show versions only for request from console](https://github.com/OpenKNX/OGM-Common/commit/76935a146fda62f25f3538abe97fe69110f2ed44)
- Fix: [Update RP2040 platform and platform_packages](https://github.com/OpenKNX/OGM-Common/commit/90ebc9d4f7618a905ca8a38617a844ee7f130452)

## 1.0.1
### Breaking Changes
- none

### Feature
- Improvement: [Show commons version in console and show running firmware version first](https://github.com/OpenKNX/OGM-Common/commit/05c58c60fa8ffc4406be2c6e058be3110b2430e3)
- Documentation: Reformat Tables in README
- Build-Process: Handling of include header-files outside commons-directory
- Update release scripts
- Delete files from commons
- Improvement: Suppress duplicate lines in memory dump
- Portability: [Use flash slots + slot version only on rp2040](https://github.com/OpenKNX/OGM-Common/commit/6559af514074ed079aaade39f49230b42e5bcb62)
- Feature: [Use callbacks in knx stack for register external flash system](https://github.com/OpenKNX/OGM-Common/commit/c725bd94c6a3111cf39155e2c544984510b00bdc)

### Bug
- Fix: [Parsing of application number](https://github.com/OpenKNX/OGM-Common/commit/86f6f77f338c310ac4f687f9650e90a829e16a35)

## 1.0.0
### Breaking Changes
- none


### Feature
- none

### Bug
- none

