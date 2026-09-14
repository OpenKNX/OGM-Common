# Geräteinformationen im Device Object

Was OGM-Common beim Start über das Gerät auf den KNX-Bus stellt, woher jeder Wert kommt und was
ein Produktentwickler dafür tun muss.

> Die Spec-Seite — alle Properties des Device Objects, Konformitätsbewertung je PID mit Fundstelle,
> Wertebereiche — steht in `OAM-IP-Interface/doc/DEVICE-OBJECT-PROPERTIES.md`. Dieses Dokument
> beschreibt nur die OGM-Common-Seite: welche Makros gelesen werden und wo sie herkommen.

---

## 1. Die Kurzfassung

**Für die meisten Produkte: nichts tun.** Die Werte kommen aus Makros, die ohnehin schon existieren —
aus `OGM-HardwareConfig`, aus der von openknxproducer erzeugten `knxprod.h` und aus zwei Dateien, die
der PlatformIO-Vorlauf schreibt. Ist ein Makro nicht definiert, meldet das Gerät die betreffende
Property als leer. Kein Fehler, kein Abbruch, keine Pflicht.

Der Code dafür steht am Ende von `Common::initKnx()` in `src/OpenKNX/Common.cpp`.

> **Voraussetzung: knx-Stack ab `2.5.0-beta.1`.** Ältere Stände kennen die Setter nicht. Der Block ist
> deshalb in `#ifdef KNX_HAS_DEVICE_INFORMATION` geklammert und fällt dort weg — OGM-Common übersetzt
> gegen alte und neue Stände. Abschnitt 11 erklärt den Mechanismus.

---

## 2. Alle Felder des Device Objects

Der vollständige Bestand, damit der Zuwachs vergleichbar ist. **ALT** war schon vor dieser Erweiterung
da, **NEU** kam mit ihr dazu. Entfernt wurde nichts.

| PID | Name | Woher der Wert kommt | Typ | |
|----:|---|---|---|---|
| 1 | `PID_OBJECT_TYPE` | konstant `OT_DEVICE` | `PDT_UNSIGNED_INT` | ALT |
| **2** | **`PID_OBJECT_NAME`** | **`DEVICE_NAME`**, ersatzweise `HARDWARE_NAME` | `PDT_UNSIGNED_CHAR` | **NEU** |
| **9** | **`PID_FIRMWARE_REVISION`** | **`FIRMWARE_REVISION`** bzw. das `init()`-Argument | `PDT_UNSIGNED_CHAR` | **NEU** |
| 11 | `PID_SERIAL_NUMBER` | Herstellercode + BAU-Nummer aus `platform().uniqueSerialNumber()` | `PDT_GENERIC_06` | ALT |
| 12 | `PID_MANUFACTURER_ID` | die ersten zwei Oktette von PID 11 | `PDT_UNSIGNED_INT` | ALT |
| 14 | `PID_DEVICE_CONTROL` | Laufzeitbits des Stacks | `PDT_BITSET8` | ALT |
| 15 | `PID_ORDER_INFO` | **`MAIN_OrderNumber`**, auf 10 Oktette gekürzt | `PDT_GENERIC_10` | ALT |
| **16** | **`PID_PEI_TYPE`** | konstant 0 — kein physisches Interface | `PDT_UNSIGNED_CHAR` | **NEU** |
| **19** | **`PID_MANUFACTURER_DATA`** | **`DEVICE_ID`** | `PDT_UNSIGNED_CHAR` | **NEU** |
| 25 | `PID_VERSION` | `openknx.info.firmwareVersion()` aus `MAIN_ApplicationVersion` + Revision | `PDT_VERSION` | ALT |
| 51 | `PID_ROUTING_COUNT` | Vorgabe 6, schreibbar | `PDT_UNSIGNED_CHAR` | ALT |
| **52** | **`PID_MAX_RETRY_COUNT`** | Vorgabe `0x33`, schreibbar; reservierte Bits werden maskiert | `PDT_GENERIC_01` | **NEU** |
| 54 | `PID_PROG_MODE` | Programmiertaste | `PDT_BITSET8` | ALT |
| **55** | **`PID_PRODUCT_ID`** | `00 FA` + **`DEVICE_HW_ID`** + 6 Nulloktette | `PDT_GENERIC_10` | **NEU** |
| 56 | `PID_MAX_APDU_LENGTH` | konstant 254 | `PDT_UNSIGNED_INT` | ALT |
| 57 | `PID_SUBNET_ADDR` | obere Hälfte der physikalischen Adresse | `PDT_UNSIGNED_CHAR` | ALT |
| 58 | `PID_DEVICE_ADDR` | untere Hälfte der physikalischen Adresse | `PDT_UNSIGNED_CHAR` | ALT |
| 71 | `PID_IO_LIST` | Liste der Interface-Objekte | `PDT_UNSIGNED_INT` | ALT |
| 78 | `PID_HARDWARE_TYPE` | `MAIN_OpenKnxId`, `MAIN_ApplicationNumber`, `MAIN_ApplicationVersion` | `PDT_GENERIC_06` | ALT |
| 82 | `PID_RF_DOMAIN_ADDRESS_CEMI_SERVER` | RF-Domänenadresse; nur unter `USE_RF` vorhanden | `PDT_GENERIC_06` | ALT |
| 83 | `PID_DEVICE_DESCRIPTOR` | Maskenversion, ggf. `OPENKNX_OVERRIDE_MASK_VERSION` | `PDT_GENERIC_02` | ALT |
| **203** | **`PID_MFR_FIRMWARE_NAME`** | **`MAIN_FirmwareName`**, ersatzweise `FIRMWARE_NAME` | `PDT_UNSIGNED_CHAR` | **NEU** |
| **204** | **`PID_MFR_DEVICE_NAME`** | wie PID 2, aber dauerhaft schreibgeschützt | `PDT_UNSIGNED_CHAR` | **NEU** |
| **205** | **`PID_MFR_BUILD_TIMESTAMP`** | **`BUILD_TIMESTAMP`** | `PDT_UNSIGNED_LONG` | **NEU** |
| **206** | **`PID_MFR_FIRMWARE_BUILD`** | **`MAIN_Version`** (Git-Kurzhash) | `PDT_UNSIGNED_CHAR` | **NEU** |

**Bilanz: 15 vorhanden, 10 neu, 0 entfernt.** Einige Felder sind maskenabhängig ausgeblendet — PID 9
entfällt auf Maske 2920h, PID 16 und 52 auf 57B0h, PID 19 auf 091A und 2920h, weil das Profil sie dort
beschreibbar verlangt und das dieser Stack nicht leisten kann.

Das Vorherige war also **Identität** — wer bin ich, welche Anwendung, welche Maske. Neu dazu kommt
**Beschreibung**: wie heißt das Gerät im Klartext, auf welcher Platine läuft es, aus welchem Build.

### Was davon ein Produktentwickler selbst füttert

Nur diese sieben. Alles andere setzt der Stack oder OGM-Common ohne Zutun.

| Property | Makro | Ausweichmakro | PID | Grenze |
|---|---|---|---:|---:|
| Gerätename | `DEVICE_NAME` | `HARDWARE_NAME` | 2 und 204 | 80 Zeichen |
| Hardware-Kennung | `DEVICE_ID` | — | 19 | 40 Zeichen |
| Firmware-Name | `MAIN_FirmwareName` | `FIRMWARE_NAME` | 203 | 48 Zeichen |
| Build (Git-Kurzhash) | `MAIN_Version` | — | 206 | 16 Zeichen |
| Build-Zeitpunkt | `BUILD_TIMESTAMP` | — | 205 | 4 Oktette |
| Firmware-Revision | `FIRMWARE_REVISION` | `init(revision)` | 9 | 0–255 |
| Produkt-ID | `DEVICE_HW_ID` | — | 55 | 10 Oktette |

Alle sieben sind mit Leseebene 3 lesbar und schreibgeschützt. Die einzige Ausnahme ist PID 2: mit
`-D KNX_DEVICE_NAME_WRITEABLE` wird sie beschreibbar (Leseebene 3 / Schreibebene 3). Der geschriebene
Wert ist **flüchtig** — nach einem Neustart steht wieder der Herstellername da. Deshalb gibt es PID 204:
sie trägt denselben Namen dauerhaft schreibgeschützt, damit ein überschriebener Gerätename die
Werksangabe nicht verdeckt.

## 3. Woher die Makros kommen

Drei Quellen, drei Zuständigkeiten:

### `OGM-HardwareConfig` — was für eine Platine ist das

```c
#define DEVICE_NAME   "OpenKNX REG1 Basismodul LAN+TP"
#define DEVICE_ID     "REG1-LAN-TP-Base"
#define DEVICE_HW_ID  0x1105
```

Ausgewählt über das `DEVICE_*`-Define der Build-Umgebung. `DEVICE_HW_ID` ist optional und nicht auf
jeder Platine gesetzt — dann fehlt PID 55 (siehe Abschnitt 6).

**`DEVICE_ID` und `DEVICE_HW_ID` sind nicht dasselbe**, und der Unterschied ist wichtig, wenn ein
Produkt darüber Hardware erkennen will:

| Board-Define | `DEVICE_HW_ID` | `DEVICE_ID` |
|---|---:|---|
| `DEVICE_REG2_PIPICO_V1` | 0x1100 | `REG2-PP-V1` |
| `DEVICE_REG2_PIPICO_V1_DD` | 0x1101 | `REG2-PP-V1` |
| `DEVICE_REG2_PIPICO_V1_ETH` | 0x1104 | `REG2-PP-V1` |
| `DEVICE_REG2_PIPICO_V1_ETH_DD` | 0x1105 | `REG2-PP-V1` |

`DEVICE_ID` benennt die Platine, `DEVICE_HW_ID` die Platine **samt Bestückung**. Wer GPIO-Belegungen
oder Portzahlen davon abhängig macht, braucht `DEVICE_HW_ID` — also PID 55, nicht PID 19.

### `include/knxprod.h` — welche Anwendung läuft darauf

Von openknxproducer aus der ETS-Produkt-XML erzeugt:

```c
#define MAIN_FirmwareName       "IP-Interface (Dev)"
#define MAIN_OrderNumber        "IP-Iface"
#define MAIN_ApplicationVersion 19
#define MAIN_FirmwareRevision   0
```

### `include/buildtime.h` und `include/versions.h` — wann und woraus gebaut

Zwei getrennte Dateien, beide im PlatformIO-Vorlauf erzeugt.

`buildtime.h`, geschrieben von `scripts/pio/prepare_buildtime.py`:

```c
#define BUILD_DATETIME  "2026-09-14 10:37:32"
#define BUILD_TIMESTAMP 1789375052
```

`versions.h`, geschrieben von `scripts/pio/prepare.py` bzw. `generate_versions.py`:

```c
#define MAIN_Version "b90b324"          // git rev-parse --short HEAD
#define KNX_Version  "2.4.0-ec.beta.1+e5ead04"
```

`BUILD_TIMESTAMP` sind Sekunden seit der Epoche und gehen unverändert auf PID 205. `MAIN_Version` ist
der Git-Kurzhash des Builds und geht auf PID 206 — damit lässt sich am Bus feststellen, aus welchem
Stand ein Gerät gebaut wurde, was die Versionsnummer allein nicht leistet.

`Common.cpp` bindet `buildtime.h` direkt ein. Ohne diesen Include wäre `BUILD_TIMESTAMP` dort nicht
sichtbar und PID 205 würde still 0 melden — `defines.h` zieht nur Hardware, knxprod und Versionen.

---

## 4. Der Revisionspfad

PID 9 hat als einziges Feld eine Kette statt einer direkten Quelle:

```
knxprod.h:  MAIN_FirmwareRevision  ->  FIRMWARE_REVISION      (falls nicht schon gesetzt)
Facade.cpp: common.init(FIRMWARE_REVISION)                    (0-Argument-init)
   oder:    openknx.init(revision)                            (Produkt gibt sie zur Laufzeit)
Common.cpp: openknx.info.firmwareRevision(revision)
Common.cpp: deviceObject().firmwareRevision(openknx.info.firmwareRevision())
```

Beide Wege enden an derselben Stelle, deshalb liest der Code `openknx.info` und nicht das Makro.
Ein Produkt, das `openknx.init()` ohne Argument aufruft, bekommt die Revision aus der `knxprod.h`;
eines mit Argument setzt sie selbst.

**Warum PID 9 zusätzlich zu PID 25 existiert:** PID 25 (`PID_VERSION`) presst die Revision in 5 Bit
und kippt still ab 32 auf 0. PID 9 ist ein volles Oktett und zeigt sie ungekürzt bis 255.

---

## 5. Textfelder: Kodierung und Kürzung

Alle vier Textfelder nehmen **UTF-8** an und geben auf dem Bus **ISO 8859-1** aus, ein Oktett je
Zeichen. Die Umsetzung passiert beim Lesen, nicht beim Setzen — im Gerät bleibt der Zeiger auf den
UTF-8-String im Flash stehen.

Daraus folgt eine Falle, die beim Zählen auffällt:

```c
#define DEVICE_NAME "OpenKNX UP1 Präsenzmelder+"
```

Das sind **26 Zeichen, aber 27 Bytes** — `ä` steht in der Quelle als `C3 A4`. Auf dem Bus kommt ein
Oktett `E4` an, PID 2 meldet als Länge **26**. Wer gegen `strlen()` prüft, rechnet falsch.

Zeichen jenseits von U+00FF haben in ISO 8859-1 keine Entsprechung und werden zu `?`. Ein abgeschnittenes
oder ungültiges UTF-8-Byte ebenso — der Lesevorgang bricht dabei nicht ab.

Ist ein Text länger als seine Grenze aus Abschnitt 2, wird er abgeschnitten; die gemeldete Elementzahl
ist dann die Grenze. Es gibt keine Warnung zur Bauzeit, also beim Benennen an den Grenzen orientieren.

Die Grenzen stehen in `knx/src/knx/device_object.h`:

```c
#define MAX_ELEM_OBJECT_NAME        80
#define MAX_ELEM_MANUFACTURER_DATA  40
#define MAX_ELEM_FIRMWARE_NAME      48
#define MAX_ELEM_FIRMWARE_BUILD     16
```

---

## 6. Fallstricke

**Die Textfelder speichern Zeiger, keine Kopien.** Was hineingeht, muss statische Lebensdauer haben —
String-Literale aus Makros erfüllen das. Ein `String`, ein Stack-Puffer oder ein `snprintf()`-Ergebnis
erfüllen es nicht und ergeben später Müll auf dem Bus. `productId()` ist die Ausnahme: sie kopiert
per `memcpy`, ein Stack-Array ist dort also in Ordnung.

**PID 55 fehlt ohne `DEVICE_HW_ID`.** Das Gerät meldet dann null Elemente statt zehn Nulloktette — ein
Leser kann "nicht vorhanden" so von einer echten Kennung unterscheiden. Der RaumController baut zum
Beispiel auf 20 Platinen, von denen genau eine eine `DEVICE_HW_ID` hat (PiPico BCU Connector, 0x1F00);
auf den anderen 19 läuft die Board-Erkennung über PID 19 im Klartext.

**PID 2 ist flüchtig.** Siehe Abschnitt 2. Wer Gerätenamen dauerhaft vergeben will, braucht dafür den
OpenKNX-Flash, nicht das KNX-Abbild — Letzteres würde jedes bestehende Gerät einmal zur
Neuprogrammierung zwingen.

**Keine Property kostet persistenten Speicher.** Alle neuen Felder sind `CallbackProperty` und erben
`saveSize() == 0`. Das Flash-Abbild bleibt auf allen fünf Maskenversionen unverändert groß, es gibt
keinen `apiVersion`-Sprung, und kein Gerät verliert beim Update seine Konfiguration.

---

## 7. Auslesen

```bash
ftc -i <ip> info              # die Schnittstelle über sich selbst
ftc -i <ip> <pa> info         # ein Busgerät über die Schnittstelle
```

```powershell
Import-Module OpenKnx.Knx.psm1
Get-KnxPropertyText -InterfaceIp 11.11.0.210 -PropertyId 19
Get-KnxPropertyText -InterfaceIp 11.11.0.210 -PropertyId 19 -Pa 5.0.7
```

Das Modul und das Beispielskript `Read-KnxDeviceObject.ps1` liegen in `OAM-IP-Interface/scripts/`.

ETS zeigt die Felder nicht in der Oberfläche an — sie sind über die Property-Dienste erreichbar, nicht
über den Gerätedialog.

---

## 8. Herstellerbereich: Belegung

Ab PID 202 gehört der Bereich dem Hersteller (03_05_01 §4.3.1). Der PID ist ein Oktett, es gibt also
**54 Plätze, 202 bis 255**.

| PID | Name | Bedeutung |
|----:|---|---|
| 202 | — | frei auf dem Device Object; der Name `PID_CUSTOM_RESERVED_TUNNELS_IP` ist im gemeinsamen Enum von `property.h` schon vergeben und gehört dem KNXnet/IP-Parameterobjekt |
| 203 | `PID_MFR_FIRMWARE_NAME` | Firmware-Name |
| 204 | `PID_MFR_DEVICE_NAME` | Gerätename ab Werk |
| 205 | `PID_MFR_BUILD_TIMESTAMP` | Build-Zeitpunkt |
| 206 | `PID_MFR_FIRMWARE_BUILD` | Git-Kurzhash |
| 207–255 | frei | |

**Regeln für eine neue Device-Object-PID in diesem Bereich:**

1. Die nächste freie Nummer nehmen und **diese Tabelle im selben Commit fortschreiben**. Es gibt keine
   vorreservierten Platzhalter — ein leeres `PID_MFR_CUSTOM_1` lädt dazu ein, dass zwei Produkte
   unabhängig dieselbe Nummer greifen.
2. Nur lesen, wenn PID 12 `0x00FA` meldet. Nummern über 201 bedeuten je Hersteller etwas anderes; zwei
   Siemens-Geräte antworten auf 203 und 204 mit völlig fremden Daten. Wer das nicht prüft, zeigt fremde
   Bytes als OpenKNX-Feld an.
3. `CallbackProperty` statt `DataProperty` verwenden, wenn der Wert nicht persistent sein soll — sonst
   wächst das Flash-Abbild und jedes bestehende Gerät braucht eine Neuprogrammierung.
4. Schreibgeschützte Properties gehören als `CallbackProperty` **ohne** Schreib-Callback angelegt.
   `WriteEnable()` wird nur auf dem cEMI-Pfad geprüft, nicht in `propertyValueWriteIndication` — eine
   als schreibgeschützt gekennzeichnete `DataProperty` ist über TP trotzdem beschreibbar.

---

## 9. Maskenübersicht: was auf welcher Maske vorhanden ist

Nicht jede Property darf auf jeder Maske existieren. Wo das Profil (Volume 6) eine Property als
**beschreibbar** verlangt, dieser Stack sie aber nur lesend anbieten kann, wird sie dort **weggelassen**
statt schreibgeschützt ausgeliefert — eine vorhandene, aber falsch zugängliche Property ist der
schlimmere Verstoß.

| PID | Name | 07B0 | 091A | 27B0 | 2920 | 57B0 |
|----:|---|:--:|:--:|:--:|:--:|:--:|
| 1 | `PID_OBJECT_TYPE` | ✓ | ✓ | ✓ | ✓ | ✓ |
| **2** | **`PID_OBJECT_NAME`** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **9** | **`PID_FIRMWARE_REVISION`** | ✓ | ✓ | ✓ | **—** | ✓ |
| 11 | `PID_SERIAL_NUMBER` | ✓ | ✓ | ✓ | ✓ | ✓ |
| 12 | `PID_MANUFACTURER_ID` | ✓ | ✓ | ✓ | ✓ | ✓ |
| 14 | `PID_DEVICE_CONTROL` | ✓ | ✓ | ✓ | ✓ | ✓ |
| 15 | `PID_ORDER_INFO` | ✓ | ✓ | ✓ | ✓ | ✓ |
| **16** | **`PID_PEI_TYPE`** | ✓ | ✓ | ✓ | ✓ | **—** |
| **19** | **`PID_MANUFACTURER_DATA`** | ✓ | **—** | ✓ | **—** | ✓ |
| 25 | `PID_VERSION` | ✓ | ✓ | ✓ | ✓ | ✓ |
| 51 | `PID_ROUTING_COUNT` | ✓ | ✓ | ✓ | ✓ | ✓ |
| **52** | **`PID_MAX_RETRY_COUNT`** | ✓ | ✓ | ✓ | ✓ | **—** |
| 54 | `PID_PROG_MODE` | ✓ | ✓ | ✓ | ✓ | ✓ |
| **55** | **`PID_PRODUCT_ID`** | ✓ | ✓ | ✓ | ✓ | ✓ |
| 56 | `PID_MAX_APDU_LENGTH` | ✓ | ✓ | ✓ | ✓ | ✓ |
| 57 | `PID_SUBNET_ADDR` | ✓ | ✓ | ✓ | ✓ | ✓ |
| 58 | `PID_DEVICE_ADDR` | ✓ | ✓ | ✓ | ✓ | ✓ |
| 71 | `PID_IO_LIST` | ✓ | ✓ | ✓ | ✓ | ✓ |
| 78 | `PID_HARDWARE_TYPE` | ✓ | ✓ | ✓ | ✓ | ✓ |
| 82 | `PID_RF_DOMAIN_ADDRESS` | — | — | ✓ | ✓ | — |
| 83 | `PID_DEVICE_DESCRIPTOR` | ✓ | ✓ | ✓ | ✓ | ✓ |
| **203** | **`PID_MFR_FIRMWARE_NAME`** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **204** | **`PID_MFR_DEVICE_NAME`** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **205** | **`PID_MFR_BUILD_TIMESTAMP`** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **206** | **`PID_MFR_FIRMWARE_BUILD`** | ✓ | ✓ | ✓ | ✓ | ✓ |
| | **Summe** | **24** | **23** | **25** | **23** | **22** |

Die Summenzeile ist nicht gerechnet, sondern gemessen: die Property-Listen wurden für alle fünf Masken
übersetzt und gezählt (vorher 14 / 14 / 15 / 15 / 14).

**Warum die vier Lücken bestehen:**

| PID | fehlt auf | Grund |
|----:|---|---|
| 9 | 2920h | 06_02_42 §2.5.2.1 Anmerkung 10 — dort nicht zulässig, PID 25 trägt die Revision |
| 16 | 57B0h | Profil A.2.3 verlangt `(3/3)` — beschreibbar; wird lieber weggelassen |
| 19 | 091A, 2920h | Profil A.3.2 verlangt `(3/0)` bzw. `3/1` — beide beschreibbar |
| 52 | 57B0h | Profil A.2.3 markiert sie dort `X` — darf als Data Property nicht existieren |

PID 82 hängt nicht an der Maske, sondern an `USE_RF`, und das setzt `config.h` für 27B0h und 2920h.

---

## 10. Was noch hineinpasst

### Wieviel Platz ist da

* **49 freie Hersteller-PIDs** (207–255). Der PID ist ein Oktett, der Bereich ab 202 gehört dem
  Hersteller, vier sind belegt.
* Je Property bis zu **255 Elemente**. Ein einzelner Lesezugriff liefert aber höchstens **15** — die
  Elementzahl in `A_PropertyValue_Read` ist ein 4-Bit-Feld. Ein 80-Zeichen-Name kostet also 1 Telegramm
  für die Länge plus 6 für den Inhalt.
* Kosten je Feld, gemessen: ein `CallbackProperty` belegt 48 Byte, ein `DataProperty` 40 (Host arm64,
  64-Bit-Zeiger; auf RP2040/ESP32 entsprechend weniger). Die zehn neuen Properties zusammen kosten
  **+1992 Byte Code** (Host arm64, `-Os`, Maske 07B0) und **0 Byte im persistenten Abbild**.

### Kandidaten aus der Spezifikation

Properties, die 03_05_01 für das Device Object kennt und die dieser Stack nicht anbietet:

| PID | Name | Was es brächte | Größe |
|----:|---|---|---|
| 21 | `PID_DESCRIPTION` | der **normgerechte** Platz für eine Klartextbeschreibung | frei |
| 28 | `PID_ERROR_CODE` | letzter Fehlercode des Interface-Objekts | 1 Oktett |
| 30 | `PID_DOWNLOAD_COUNTER` | wie oft ETS das Gerät programmiert hat | 2 Oktette |
| 53 | `PID_ERROR_FLAGS` | Fehlerbits des Geräts — Diagnose ohne Konsole | 1 Oktett |
| 10 | `PID_SERVICES_SUPPORTED` | welche Management-Dienste das Gerät beherrscht | 2 Oktette |
| 72 | `PID_MGT_DESCRIPTOR_01` | Management-Deskriptor | 10 Oktette |

**Zu PID 21 der Haken**, der auch der Grund ist, warum stattdessen 203/204 entstanden sind: das Profil
verlangt sie beschreibbar, und für einen geschriebenen Wert gibt es hier keinen nichtflüchtigen Speicher.
Dasselbe Muster wie bei PID 2.

**PID 53 wäre der nächste sinnvolle Schritt** — ein Oktett, rein lesend, keine Profilfalle, und es macht
Gerätefehler am Bus sichtbar, für die man heute die Konsole braucht.

### Kandidaten im Herstellerbereich

Dort ist der Inhalt frei, solange PID 12 `0x00FA` meldet. Was sich anböte:

| Was | Warum | Größe |
|---|---|---|
| Letzter Reset-Grund | Watchdog, Brownout, OTA, Netzausfall — heute nur im Log, das der Reset löscht | 1 Oktett |
| Betriebsstunden seit Reset | Stabilität beurteilen, ohne die Konsole zu öffnen | 4 Oktette |
| Freier Heap / kleinster je gesehener | Speicherleck erkennen, bevor es abstürzt | 2×4 Oktette |
| Bootloader-Stand | welcher OTA-Pfad möglich ist | Text |
| Stack-Version (`KNX_Version`) | welcher knx-Stand im Gerät steckt | Text, ~24 Zeichen |
| Modulliste mit Versionen | welche OFM-Module laufen | Text, lang |

Die ersten drei sind Laufzeitwerte und brauchen einen Callback, der bei jedem Lesen neu misst — nicht
einen beim Start eingefrorenen Wert.

**Bevor eine neue Property entsteht**, die Regeln in Abschnitt 8 lesen; besonders die Pflicht, PID 12 zu
prüfen, und die Falle mit schreibgeschützten `DataProperty`.

---

## 11. Voraussetzung: knx-Stand, und wie ältere abgefangen werden

Der Block braucht sieben Setter auf `DeviceObject` — `deviceName()`, `hardwareId()`, `firmwareName()`,
`firmwareBuild()`, `buildTimestamp()`, `firmwareRevision()` und `productId()`. Die gibt es ab
**`2.5.0-beta.1`**. Ein älterer Stack kennt sie nicht.

### Der Schalter

`knx/src/knx/device_object.h` kündigt das Merkmal dort an, wo die Setter stehen:

```c
#define KNX_HAS_DEVICE_INFORMATION 1
```

`Common.cpp` klammert den Block darauf:

```c
#ifdef KNX_HAS_DEVICE_INFORMATION
    ... deviceName / hardwareId / firmwareName / firmwareBuild / buildTimestamp
        firmwareRevision / productId ...
#endif
```

Ein alter Stack definiert das Makro nicht, der Block fällt weg, OGM-Common übersetzt. Ein neuer Stack
definiert es, der Block greift.

### Warum in `device_object.h` und nicht in `config.h`

`config.h` ist ab Zeile 3 vollständig in `#ifndef NO_KNX_CONFIG` gewickelt. Ein Ziel, das seine
Konfiguration selbst mitbringt, schaltet die Datei ab — **und das Merkmalsmakro gleich mit**. Die Setter
wären vorhanden, das Makro nicht, der Block fiele still weg, und PID 2/19/55/203–206 kämen leer heraus.
Ohne Übersetzungsfehler. In `device_object.h` kann das Makro nicht von den Settern getrennt werden, die
es ankündigt.

Erreichbarkeit ist geprüft, nicht angenommen — `Common.h` zieht `knx.h`, und von dort führt über jede
Maske ein Weg zum Header:

```
knx.h -> knx_facade.h -> bau07B0.h    -> bau_systemB_device.h -> device_object.h
knx.h -> knx_facade.h -> bau07B0_ip.h -> ip_parameter_object.h -> device_object.h
knx.h -> knx_facade.h -> bau091A.h    -> bau_systemB_coupler.h -> device_object.h
```

### Warum nicht über die Versionsnummer

`versions.h` liefert die knx-Version nur als Zeichenkette (`KNX_Version "2.5.0-beta.1+<hash>"`), mit der
der Präprozessor nicht vergleichen kann. Man könnte `prepare.py` erweitern, damit sie wie bei den
OFM-Modulen in `Major`/`Minor`/`Revision` zerfällt — bliebe aber der Nachteil, dass eine Nummer ein
Merkmal nur behauptet: wer den Commit einzeln übernimmt, bekommt das Merkmal ohne die Nummer. Als
Ergänzung für Mindeststand-Prüfungen sinnvoll, als Schalter nicht.

### Dass die Klammer wirklich hält, ist nachgewiesen

Zwei Befunde zusammen ergeben den Beweis:

1. Das Makro wurde versuchsweise auskommentiert und `release_REG1_LAN_TP_BASE` übersetzt — **grün**.
   Damit steht fest, dass ausserhalb der Klammer nichts Neues referenziert wird, auch nicht
   `LEN_PRODUCT_ID`, das es nur im neuen Header gibt.
2. Alle sieben neuen Setter kommen in `Common.cpp` ausschliesslich innerhalb der Klammer vor. Die vier
   älteren Aufrufe — `hardwareType()`, `maskVersion()`, `version()` und `knx.orderNumber()` — stehen
   darüber und bleiben ungeklammert, weil es sie in beiden Ständen gibt.

### Was Common NICHT tun muss: die Maske kennen

Die Setter sind reine Zuweisungen ohne Nebenwirkung. Welche Property daraus am Bus sichtbar wird,
entscheidet allein die Property-Liste im Stack (Abschnitt 9). Auf Maske 091A existiert PID 19 nicht,
also wird `_hardwareId` nie gelesen — der Wert liegt da und kostet vier Byte.

Deshalb steht in `Common.cpp` **keine einzige Maskenabfrage**, und das ist Absicht: läge die
Gatterlogik an zwei Stellen, könnten die beiden auseinanderlaufen.
