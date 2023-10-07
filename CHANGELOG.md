# Changes

## 2023-10-02 -- 2023-10-07
* Restore-Scripts:
  * Use knx from OpenKNX-Repository
  * Improve error-handling: Restore working-directory before exit 1
* Refactor: Optimization of the logger to use less memory on the stack.
* Refactor: Das TimerInterupt wurde für die RP2040 Platform umgebaut.
* Feature: Stack usage monitoring
* Update: Platform raspberrypi

## 2023-09-22 -- 2023-09-26
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

## 2023-09-04 -- 2023-09-17
* Feature: Show uptime in human-readable form (days hh:mm:ss)
* Fix/Extension: `Common:uptime(..)` provides uptime in seconds, detecting overflow of `millis()` for correct uptimes >49 days
* Fix: [Process command 'versions' from console only](https://github.com/OpenKNX/OGM-Common/commit/9576370424712666d5b24dcefb23062d2d4a4ca2)
* Fix: [Automatic reset on RP2040 using DualCore did not work.](https://github.com/OpenKNX/OGM-Common/commit/63258937b93dc46fbac401834a46a01faaae9b47) Solved by patch included in https://github.com/maxgerhardt/platform-raspberrypi.git#5208e8c
* Fix: [Show versions only for request from console](https://github.com/OpenKNX/OGM-Common/commit/76935a146fda62f25f3538abe97fe69110f2ed44)
* Feature #9: Optional Runtime Statistics Measurement to Support Development, available setting `OPENKNX_RUNTIME_STAT`
* Fix: [Update RP2040 platform and platform_packages](https://github.com/OpenKNX/OGM-Common/commit/90ebc9d4f7618a905ca8a38617a844ee7f130452)
* Extension: Introduce `Common::freeLoopIterate(..)` as Helper for module / channel iteration respecting `freeLoopTime()`
  * Change: At least one module will be called in `Common::loop()`

## 2023-08-11 -- 2023-09-03
* Improvement: [Show commons version in console and show running firmware version first](https://github.com/OpenKNX/OGM-Common/commit/05c58c60fa8ffc4406be2c6e058be3110b2430e3)
* Documentation: Reformat Tables in README
* Fix: [Parsing of application number](https://github.com/OpenKNX/OGM-Common/commit/86f6f77f338c310ac4f687f9650e90a829e16a35)
* Build-Process: Handling of include header-files outside commons-directory
  * Update release scripts
  * Delete files from commons
* Improvement: Suppress duplicate lines in memory dump
* Portability: [Use flash slots + slot version only on rp2040](https://github.com/OpenKNX/OGM-Common/commit/6559af514074ed079aaade39f49230b42e5bcb62)
* Feature: [Use callbacks in knx stack for register external flash system](https://github.com/OpenKNX/OGM-Common/commit/c725bd94c6a3111cf39155e2c544984510b00bdc)

# v1 (2023-08-11)
New modular commons architecture seen as stable and recommended for usage in all OpenKNX firmwars. 