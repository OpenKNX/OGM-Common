# Changes

## 1.1.3: 2024-07-02
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