# OGM-Common – Agent-Anweisungen

## Projekt

**OGM-Common** ist die **OpenKNX-Basisinfrastruktur** — keine eigenständige
Applikation. Sie stellt das Modulsystem, die KNX-Stack-Anbindung, Logging,
Flash-Speicher, LED-Verwaltung, Zeit-API und Konsole für alle
OpenKNX-Firmwareprojekte bereit. Sie hat keine eigene `platformio.ini`,
sondern wird immer als Abhängigkeit in ein konkretes Geräteprojekt
zusammen mit einem oder mehreren OFMs (OpenKNX Function Modules)
eingebunden.

> **Nicht eigenständig lauffähig:** OGM-Common lässt sich nicht alleine
> compilieren. Es braucht ein konkretes Geräteprojekt, das `hardware.h`,
> `knxprod.h`, `versions.h` und ein `main.cpp` mit der gerätespezifischen
> Einrichtung bereitstellt. OFMs (z. B. OFM-Network unter `../OFM-Network`)
> bauen auf OGM-Common auf — nicht umgekehrt.

- **Sprache**: C++ (Arduino-Framework, keine explizite `-std=`-Flag —
  es gilt der Default des jeweiligen Cores, i. d. R. `gnu++17`)
- **Erfahrungslevel**: Fortgeschritten — keine Grundlagenerklärungen zu
  Arduino, RP2040, ESP32 oder KNX nötig, direkt zur Sache.
- **Branch-Konvention**: `v1` = stabiles Release, `v1dev` = aktive
  Entwicklung, Feature-Branches werden in `v1dev` gemergt.
- **Abhängigkeiten** (`library.json`): `khoih-prog/TimerInterrupt_Generic`,
  `robtillaart/ANSI`, `RTTStream` (github koendv) — also nicht komplett
  frei von externen Libraries.

## Hardware-Plattformen

| Plattform | Hinweise |
|----------|-------|
| **RP2040** | Referenzplattform, voll unterstützt, Dual-Core über `OPENKNX_DUALCORE` |
| **RP2350** | Läuft über den RP2040/arduino-pico-Toolchain-Zweig; eigenes PSRAM über `PICO_RP2350_PSRAM_CS`, aber kein eigener `ARDUINO_ARCH_RP2350` — arduino-pico compiliert RP2350 unter `ARDUINO_ARCH_RP2040` |
| **ESP32** | Voll unterstützt (nicht mehr experimentell), PSRAM über `BOARD_HAS_PSRAM`, Dual-Core ebenfalls unterstützt (nicht nur RP2040) |
| **SAMD21** | Veraltet, Code-Pfade existieren noch (`#ifdef ARDUINO_ARCH_SAMD`), funktionieren aber vermutlich nicht mehr zuverlässig — keine neue Hardware, nicht aktiv gepflegt |

- Plattform-Weichen im Code: `#ifdef ARDUINO_ARCH_ESP32` /
  `#ifdef ARDUINO_ARCH_RP2040` / `#ifdef ARDUINO_ARCH_SAMD` — **es gibt
  kein `ARDUINO_ARCH_RP2350`**, RP2350-spezifischer Code läuft über
  Pico-SDK-Macros wie `PICO_RP2350_PSRAM_CS`.

## Architektur & Kernklassen

### `OpenKNX::Common` (`src/OpenKNX/Common.h/.cpp`)
Zentrales Singleton (`openknx.common`, meist über die Facade-Kurzform
angesprochen). Verwaltet die Modul-Registry (`OPENKNX_MAX_MODULES`,
Default 9), treibt `init()` → `setup()`/`setup(bool configured)` →
`loop()`/`loop(bool configured)` für alle Module an (plus `setup1()`/
`loop1()` unter `OPENKNX_DUALCORE`), stößt Flash-Laden/-Speichern an
(die eigentliche Logik liegt in `Flash::Default`), Startup-Delay (nur
aktiv wenn `BASE_StartupDelayBase` definiert ist), Heartbeat (nur aktiv
wenn `BASE_HeartbeatDelayBase` definiert ist), Watchdog-Ansteuerung
(eigentliche Klasse: `OpenKNX::Watchdog`, `openknx.watchdog`) und
Power-Save über `SAVE_INTERRUPT_PIN`.

Weitere wichtige Common-API: `restart()`, `freeLoopTime()`/
`freeLoopIterate()` (kooperatives Multitasking — OFMs sollten das in
`loop()` nutzen statt alles auf einmal abzuarbeiten), Heap-/Stack-Stats.

### `OpenKNX::Base` (`src/OpenKNX/Base.h`) und `OpenKNX::Module` (`src/OpenKNX/Module.h`)
`Module` erbt von `Base`. Beide zusammen bilden die Basisklasse für alle
OFMs — die Methoden sind auf beide verteilt, nicht nur auf `Module`:

- `Base`: `name()` (Pure Virtual — **muss** implementiert werden),
  `init()`, `setup()`/`setup(bool configured)`, `loop()`/
  `loop(bool configured)`, `setup1()`/`loop1()` (nur `OPENKNX_DUALCORE`),
  `processInputKo()`, `processFunctionProperty[State]()`.
- `Module`: `version()` (Pure Virtual — **muss** implementiert werden),
  `flashSize()`, `writeFlash()`, `readFlash(...)`,
  `processAfterStartupDelay()`, `processBeforeRestart()`,
  `processBeforeTablesUnload()`, `savePower()`, `restorePower()`,
  `processCommand(const std::string cmd, bool diagnoseKo)`,
  `showHelp()`, `showInformations()`.

Module registrieren sich über `openknx.addModule(uint8_t id, Module& module)`.

Für Kanäle innerhalb eines Moduls gibt es zusätzlich `OpenKNX::Channel`
(`src/OpenKNX/Channel.h`) — das übliche Muster in OFMs mit mehreren
gleichartigen Kanälen (siehe auch das Kanalauswahl-Muster in
`.claude/agents/openknx-channelselect.md`).

### `OpenKNX::Log::Logger` (`src/OpenKNX/Log/Logger.h/.cpp`)
Wichtiger als der interne Ringpuffer ist die Log-API selbst:
`logInfoP`/`logErrorP`/`logWarningP`/`logDebugP`/`logTraceP` (plus
`logHex*`-Varianten), `logIndentUp()`/`logIndentDown()`. Nachrichten sind
auf `OPENKNX_MAX_LOG_PREFIX_LENGTH` (23) bzw.
`OPENKNX_MAX_LOG_MESSAGE_LENGTH` (200) begrenzt.

Trace-Filter (`OPENKNX_TRACE`) haben eine neue Syntax: eine einzelne,
Semikolon-getrennte Liste von Filtern statt der alten
`OPENKNX_TRACE1`..`OPENKNX_TRACE5`. Format `PREFIX<SUB>` mit `*` als
Wildcard, Bereiche (`1-19`) und Listen (`4,5,7`) innerhalb `<...>`.

Ist `OPENKNX_WEBCONSOLE` aktiv, existiert zusätzlich ein einfacher
Byte-Ringpuffer (`OPENKNX_WEBCONSOLE_BUFSIZE`, Default 4096) für die
Webkonsole, zugänglich über `ringBuf()`/`ringWritePos()` — kein
strukturiertes Frame-Format, keine Sequenznummern.

### `OpenKNX::Flash::Default` (`src/OpenKNX/Flash/Default.h/.cpp`, `openknx.flash`)
Das ist die Klasse, mit der Module tatsächlich interagieren
(`readXXX`/`writeXXX`). Sie vergibt jedem Modul einen festen Bereich in
Registrierungsreihenfolge, basierend auf `flashSize()`.

### `OpenKNX::Flash::Driver` (`src/OpenKNX/Flash/Driver.h/.cpp`)
Niedrigere Ebene: reiner Zugriff auf einen Flash-Bereich (lesen,
schreiben, löschen, sektorweise puffern). Kennt keine Module. Zwei
Instanzen: `openknx.openknxFlash` und `openknx.knxFlash`.

### `OpenKNX::Time::TimeProvider` (`src/OpenKNX/Time/TimeProvider.h/.cpp`) und `TimeManager`
Abstrakte Basis für Zeitquellen (KNX DPT19 über `TimeProviderKnx`, NTP
über OFM-Network). `TimeManager` verwaltet **genau einen** aktiven
Provider (`setTimeProvider()`) — keine Arbitrierung zwischen mehreren
Quellen. Wichtiger für OFMs ist meist das Event-System
(`TimeChangedEvents`/`TimeChangeCallback`), über das man auf
Zeitänderungen reagiert, statt selbst zu pollen.

### `OpenKNX::Facade` (`src/OpenKNX/Facade.h/.cpp`)
Öffentliche API-Oberfläche — `openknx` ist eine Instanz davon. Neben den
oben genannten hängen hier u. a. `console`, `hardware`, `leds`
(`Led::Manager`), `ledFunctions` (`Led::FunctionManager`), `gpio`
(`GPIO::Manager`), `sun`, `calendar`, `progButton`/`func1..3Button` und
`modules` (die Registry selbst).

### Weitere Subsysteme (nicht im Detail dokumentiert, aber vorhanden)
`src/OpenKNX/Console.h/.cpp` (Kommandokonsole), `src/OpenKNX/Led/*`
(siehe `README_LED.md`), `src/OpenKNX/GPIO/*` (u. a. PCA9554/PCA9557/
TCA6408/TCA9555-Expander), `src/OpenKNX/Button.h`,
`src/OpenKNX/Watchdog.h`, `src/OpenKNX/TimerInterrupt.h`,
`src/OpenKNX/Stat/*`, `src/OpenKNX/Sun/*`.

## KNX-Konzepte & Integration

KNX-Parameter kommen über Macros aus `knxprod.h` (z. B. `ParamBASE_*`,
`KoBASE_*`). Definiert in `src/Common.share.xml` (siehe auch
`src/Common.Router.share.xml`, `src/InfoLed.part.xml`).

**Zwingend erforderlich** (sonst schlägt der Build fehl bzw. ist die
Applikation funktionslos): `MAIN_OpenKnxId`, `MAIN_ApplicationNumber`,
`MAIN_ApplicationVersion`.

**Optional, per `#ifdef` geschützt** (aktivieren jeweils eine Funktion):
`BASE_StartupDelayBase` + `ParamBASE_StartupDelayTimeMS` (Startup-Delay),
`BASE_HeartbeatDelayBase` + `KoBASE_Heartbeat` +
`ParamBASE_HeartbeatDelayTimeMS` (Heartbeat), `MAIN_OrderNumber`,
`BASE_PeriodicSave`, `BASE_KoManualSave`, `ParamBASE_Latitude`
(schaltet die Sonnenstand-Berechnung frei).

**Vom Compiler über `#error` in `src/OpenKNX.h` erzwungen:**
`-D SMALL_GROUPOBJECT` muss gesetzt sein, die Zielarchitektur muss
SAMD/RP2040/ESP32 sein, `OPENKNX_FLASH_OFFSET`/`_SIZE` und
`KNX_FLASH_OFFSET`/`_SIZE` müssen definiert sein (auf RP2040 muss
`OPENKNX_FLASH_SIZE` ein Vielfaches von 4096 sein), sowie ein passender
`MASK_VERSION` (0x07B0 TP / 0x57B0 IP / 0x091A IPTP).

**Wichtige Compile-Defines (Auswahl, siehe `src/OpenKNX/defines.h` und
`README.md` für die vollständige Liste):**
```
OPENKNX_DUALCORE             – Dual-Core aktivieren (RP2040 und ESP32)
OPENKNX_WATCHDOG             – Watchdog aktivieren (nur Releases — wird bei
                                OPENKNX_DEBUGGER automatisch abgeschaltet)
OPENKNX_WATCHDOG_MAX_PERIOD  – Watchdog-Timeout in Sekunden (Default 16)
OPENKNX_RECOVERY_TIME        – ms Prog-Taster halten für Factory-Reset
                                (Default 6000, 0 = aus; braucht PROG_BUTTON_PIN)
OPENKNX_DISABLE_PSRAM        – PSRAM deaktivieren (z. B. für Segger-Debugging)
OPENKNX_MAX_MODULES          – maximale Anzahl Module (Default 9)
OPENKNX_MAX_LOOPTIME /
OPENKNX_LOOPTIME_WARNING     – Loop-Zeit-Überwachung
OPENKNX_WEBCONSOLE /
OPENKNX_WEBCONSOLE_BUFSIZE   – Webkonsole samt Ringpuffergröße (Default 4096)
OPENKNX_TRACE                – Trace-Filter, neue Syntax (siehe Logger oben)
FIRMWARE_REVISION            – ändert die Signatur von openknx.init()
SAVE_INTERRUPT_PIN           – fallende Flanke löst Power-Save aus
                                (Define in hardware.h, kein Build-Flag)
```

## Embedded-Rahmenbedingungen

Der Code läuft auf Mikrocontrollern mit sehr begrenzten Ressourcen —
jedes Byte und jeden Takt als kostbar behandeln:

- **RAM**: RP2040 hat 264 KB insgesamt (geteilt mit Stack, Heap, lwIP,
  KNX-Stack). ESP32 hat typischerweise ~320 KB freien Heap. RP2350 und
  ESP32 können PSRAM nutzen, wenn `OPENKNX_PSRAM` aktiv ist (automatisch
  erkannt über `BOARD_HAS_PSRAM`/`PICO_RP2350_PSRAM_CS`, abschaltbar über
  `OPENKNX_DISABLE_PSRAM`) — dazu die Helfer `PSRAM_MALLOC`/
  `PSRAM_CALLOC`/`PSRAM_REALLOC`, `psram_new()`/`psram_delete()`,
  `PsramAllocator<T>`, `PSRAM_DATA`/`PSRAM_CODE` aus
  `src/OpenKNX/Helper.h`. Keine dynamische Allokation in Hot Paths.
- **Flash**: `const` für Read-Only-Daten verwenden — der Linker legt
  `.rodata` automatisch ins Flash. `PROGMEM` existiert zwar auf ESP32/
  RP2040 (als No-Op), bringt dort aber nichts — nicht verwenden.
  Duplizierte String-Literale vermeiden. Auf RP2350/ESP32 `PSRAM_CODE`
  nutzen, um große Funktionen ins PSRAM zu verschieben und Flash zu
  sparen.
- **Kein Heap-Churn in `loop()`**: kein `new`/`delete` und keine
  `std::string`-Konstruktion in Hot Paths — feste Puffer, Stack-Locals
  oder vorallozierte Member bevorzugen. `std::string` selbst ist Teil der
  Pflicht-API (`name()`, `version()`, `logPrefix()`,
  `processCommand()`) und damit nicht grundsätzlich verboten, nur
  sparsam in `loop()` einsetzen.
- **STL mit Bedacht einsetzen**: `std::function` wird in der Zeit-API
  bewusst genutzt (`TimeChangeCallback`) — kein Totalverbot, aber
  `std::map`/`std::stringstream` & Co. eher vermeiden, wo einfache
  Arrays/`snprintf` reichen.
- **Stack**: RP2040 hat pro Core einen eigenen Stack (kein RTOS per
  Default), überwacht über `_freeStackMin`/`_freeStackMin1`. ESP32 läuft
  auf FreeRTOS, zweiter Core-Stack über `ARDUINO_LOOP1_STACK_SIZE`
  konfigurierbar. Rekursion und große Stack-Frames in Callbacks
  vermeiden.
- **CPU**: RP2040 ist Dual-Core Cortex-M0+ @ 125 MHz, kein FPU — Float
  vermeiden, wo Integer-Arithmetik reicht (RP2350/Cortex-M33 hat einen
  FPU).

## Code-Konventionen

- **Kein `delay()`** — alles nicht-blockierend, Zustandsautomaten mit
  `millis()`
- **Plattform-Weichen**: `#ifdef ARDUINO_ARCH_ESP32` /
  `#ifdef ARDUINO_ARCH_RP2040` / `#ifdef ARDUINO_ARCH_SAMD`
- **PSRAM**: siehe Embedded-Rahmenbedingungen oben, alles definiert in
  `src/OpenKNX/Helper.h`
- Kommentare auf Deutsch oder Englisch (gemischt ok, aber pro Datei
  konsistent)

### Formatierung (`.clang-format`, an llvm angelehnt mit Allman-artigen Brace-Regeln)
- **Immer exakt nach `.clang-format` richten** — maßgeblich
- Klammern immer in eigener Zeile (Klassen, Funktionen, `if`, `else`,
  `for`, `while`, `case`, `enum`, `struct`, `namespace`)
- Kein Spaltenlimit
- 4 Leerzeichen Einrückung, keine Tabs
- **Präprozessor-Direktiven werden bei Verschachtelung eingerückt**
  (`IndentPPDirectives: BeforeHash`) — die Einrückung steht vor dem `#`
- `namespace`-Inhalte werden eingerückt (`NamespaceIndentation: All`),
  ebenso `case`-Label
- `if` ohne Klammern nur bei einer einzelnen Anweisung
  (`AllowShortIfStatementsOnASingleLine: OnlyFirstIf`)
- Kurze Funktionen/Lambdas/Enums/`case`-Label dürfen einzeilig bleiben
- Beim Schreiben neuen Codes am Stil der umgebenden Datei orientieren

## Referenzen

- [README.md](README.md) — Modulübersicht und Konfigurationsreferenz
- [README_LED.md](README_LED.md) — LED-Subsystem
- [doc/Applikationsbeschreibung-Common.md](doc/Applikationsbeschreibung-Common.md) — vollständige KNX-Applikationsdokumentation
- [doc/Build-and-release-environment.md](doc/Build-and-release-environment.md), [doc/Update-ETS-Application.md](doc/Update-ETS-Application.md)
- [CHANGELOG.md](CHANGELOG.md) — Versionshistorie

## Dokumentationspflege

Nach **jeder Codeänderung** im selben Zug alle betroffenen READMEs
aktualisieren:

| Art der Änderung | Zu aktualisierende Dateien |
|-------------|----------------|
| Öffentliche API / neue Funktion | `README.md` |
| KNX-Parameter, Compile-Defines | `README.md` + `doc/Applikationsbeschreibung-Common.md` |
| Bugfix (keine API-Änderung) | Kein README-Update nötig |

Regeln:
- READMEs immer synchron zum Code halten — nie veraltet stehen lassen
- Keine neuen README-Dateien anlegen, außer explizit gewünscht
- Keinen "in dieser Session geändert"-Abschnitt o. Ä. ergänzen — Inhalte
  direkt an der passenden Stelle aktualisieren
