# Der Geräte-Steckbrief: Device Object Properties

Was ein OpenKNX-Gerät über sich selbst auf den KNX-Bus legt, woher jeder Wert stammt, ob er
KNX-konform ist, und wie man ihn ausliest.

| | |
|---|---|
| Stand | 13.09.2026 |
| Betrifft | jedes Gerät auf `lib/knx` + `OGM-Common` (TP, IP, Koppler, RF) |
| Beispielgerät | IP-Interface REG1, `11.11.0.210` (5.0.13), Maske 0x07B0 |
| Status | gebaut, auf ESP32 HW-verifiziert, noch nicht committet |
| Offen | eine Entscheidung zu PID 2, siehe Abschnitt 9 |

---

## 1. Was eine PID überhaupt ist

Jedes KNX-Gerät führt sogenannte **Interface Objects** — nummerierte Behälter für Systemdaten.
Objekt Nummer 0 ist immer das **Device Object**, der Ausweis des Geräts. Darin liegen einzelne
Felder, und jedes Feld hat eine **Property ID (PID)** — eine Nummer von 1 bis 255, unter der man es
abfragt. Die Nummern sind zentral von der KNX Association vergeben; ein Hersteller darf sie sich
nicht ausdenken.

Ein Feld ist immer ein **Array aus gleich großen Elementen**:

* Die meisten Felder haben genau **ein** Element (z. B. die Seriennummer: 1 Element à 6 Byte).
* Textfelder haben **ein Element je Zeichen** (z. B. der Gerätename: 30 Elemente à 1 Byte).

Deshalb braucht man beim Lesen immer drei Angaben: **PID**, **Startindex** und **Anzahl Elemente**.

### Warum die Nummern Lücken haben

Die Liste springt: 1, 2, 9, 11, 12, 14, 15, 16, 19, 25 … Das hat zwei Gründe, und beide sind normal.

**Erstens gibt es die Nummer schlicht nicht.** PID 3, 4, 20, 22, 24 und 26 sind im Standard
**nie vergeben worden** — in `03_05_01 Resources` existiert dazu keine einzige Klausel. Die
Nummerierung ist historisch gewachsen, nicht lückenlos durchgezählt.

**Zweitens gehört die Nummer woanders hin.** Der Nummernraum ist in drei Blöcke geteilt:

| Bereich | Bedeutung | Quelle |
|---|---|---|
| 1 – 50 | **objekttyp-unabhängig** — in *jedem* Interface Object dieselbe Bedeutung | `03_05_01` §4.2 |
| 51 – 200 | **objekttyp-spezifisch** — PID 51 heißt im Device Object etwas anderes als im Router Object | `03_05_01` §4.3 ff. |
| ab 202 | **herstellerspezifisch**, frei verwendbar | `03_05_01` §4.3.1, S. 43 |

PID 5, 6, 7 und 13 zum Beispiel *existieren* im Bereich 1–50, gehören aber zu ladbaren Objekten
(Lade- und Laufzeit-Steuerung, Programmversion) und haben im Device Object nichts verloren. Ein
Device Object zeigt also naturgemäß nur eine ausgedünnte Auswahl.

---

## 2. Alle Felder im Device Object

Gemessen am gebauten Stand für Maske **0x07B0** (TP1 System B). Die Spalte *Größe* ist die
Elementgröße, die der Typ vorgibt — `PDT_GENERIC_10` heißt schlicht: 10 Byte je Element.
*Gesamt* ist `Größe x max. Elemente`, also der Platz, den das Feld maximal belegen kann.

| PID | Name | Typ (PDT) | Größe | max. Elem. | Gesamt | Zugriff | Wert am Beispielgerät | Quelle des Werts | Status |
|----:|---|---|---:|---:|---:|---|---|---|---|
| 1 | PID_OBJECT_TYPE | PDT_UNSIGNED_INT `0x04` | 2 B | 1 | 2 B | R3 / ro | `0x0000` | konstant (Objekttyp "Device") | bisher |
| **2** | **PID_OBJECT_NAME** | PDT_UNSIGNED_CHAR `0x02` | 1 B | 80 | 80 B | R3 / ro *(W3 mit Define)* | `OpenKNX REG1 Basismodul LAN+TP` | `DEVICE_NAME` (OGM-HardwareConfig), bis ein Management-Client etwas anderes schreibt | **NEU** |
| **9** | **PID_FIRMWARE_REVISION** | PDT_UNSIGNED_CHAR `0x02` | 1 B | 1 | 1 B | R3 / ro | `0` | `openknx.info.firmwareRevision()` | **NEU** |
| 11 | PID_SERIAL_NUMBER | PDT_GENERIC_06 `0x16` | 6 B | 1 | 6 B | R3 / ro | `00FA 38D6DDDB` | `0x00FA` + Chip-Seriennummer | bisher |
| 12 | PID_MANUFACTURER_ID | PDT_UNSIGNED_INT `0x04` | 2 B | 1 | 2 B | R3 / ro | `0x00FA` | die ersten 2 Byte von PID 11 | bisher |
| 14 | PID_DEVICE_CONTROL | PDT_BITSET8 `0x33` | 1 B | 1 | 1 B | R3 / W3 | Laufzeit-Flags | Stack (Safe State, Verify Mode) | bisher |
| 15 | PID_ORDER_INFO | PDT_GENERIC_10 `0x1A` | 10 B | 1 | 10 B | R3 / ro | `IP-Iface` + 2x `00` | `MAIN_OrderNumber` (knxprod.h) | **Fehler behoben** |
| **16** | **PID_PEI_TYPE** | PDT_UNSIGNED_CHAR `0x02` | 1 B | 1 | 1 B | R3 / ro | `0` | konstant — kein PEI vorhanden | **NEU** |
| **19** | **PID_MANUFACTURER_DATA** | PDT_UNSIGNED_CHAR `0x02` | 1 B | 40 | 40 B | R3 / ro | `REG1-LAN-TP-Base` | `DEVICE_ID` (OGM-HardwareConfig) | **NEU** |
| 25 | PID_VERSION | PDT_VERSION `0x30` | 2 B | 1 | 2 B | R3 / ro | `1.3` (`0x0043`) | `openknx.info.firmwareVersion()` | bisher |
| 51 | PID_ROUTING_COUNT | PDT_UNSIGNED_CHAR `0x02` | 1 B | 1 | 1 B | R3 / W3 | `0x60` (Hop-Count 6) | Vorgabe im Stack | bisher |
| **52** | **PID_MAX_RETRY_COUNT** | PDT_GENERIC_01 `0x11` | 1 B | 1 | 1 B | R3 / W3 | `0x33` (Busy 3 / Nak 3) | Vorgabe, entspricht dem TPUart-Treiber | **NEU** |
| 54 | PID_PROG_MODE | PDT_BITSET8 `0x33` | 1 B | 1 | 1 B | R3 / W3 | `0` | Programmiertaste / Konsole | bisher |
| **55** | **PID_PRODUCT_ID** | PDT_GENERIC_10 `0x1A` | 10 B | 1 | 10 B | R3 / ro | `00FA 1125 00 00 00 00 00 00` | Anwendungshersteller + `DEVICE_HW_ID`; **fehlt**, wo das Board keine HW-Kennung hat | **NEU** |
| 56 | PID_MAX_APDU_LENGTH | PDT_UNSIGNED_INT `0x04` | 2 B | 1 | 2 B | R3 / ro | `254` | konstant | bisher |
| 57 | PID_SUBNET_ADDR | PDT_UNSIGNED_CHAR `0x02` | 1 B | 1 | 1 B | R3 / ro | `0x50` | oberes Byte der phys. Adresse | bisher |
| 58 | PID_DEVICE_ADDR | PDT_UNSIGNED_CHAR `0x02` | 1 B | 1 | 1 B | R3 / ro | `0x0D` | unteres Byte der phys. Adresse | bisher |
| 71 | PID_IO_LIST | PDT_UNSIGNED_INT `0x04` | 2 B | 8 | 16 B | R3 / ro | `0,1,2,9,3,11` | Liste der vorhandenen Objekte, je BAU | bisher |
| 78 | PID_HARDWARE_TYPE | PDT_GENERIC_06 `0x16` | 6 B | 1 | 6 B | R3 / W3 | `00 00 A5 02 13 00` | OpenKnxId, App-Nummer, App-Version | bisher |
| 82 | PID_RF_DOMAIN_ADDRESS | PDT_GENERIC_06 `0x16` | 6 B | 1 | 6 B | R3 / W3 | — | nur auf RF-Masken vorhanden | bisher |
| 83 | PID_DEVICE_DESCRIPTOR | PDT_GENERIC_02 `0x12` | 2 B | 1 | 2 B | R3 / ro | `0x07B0` | Maskenversion, vom BAU gesetzt | bisher |
| **203** | **PID_MFR_FIRMWARE_NAME** | PDT_UNSIGNED_CHAR `0x02` | 1 B | 48 | 48 B | R3 / ro | `IP-Interface (Dev)` | `MAIN_FirmwareName` (knxprod.h) | **NEU** |
| **204** | **PID_MFR_DEVICE_NAME** | PDT_UNSIGNED_CHAR `0x02` | 1 B | 80 | 80 B | R3 / ro | `OpenKNX REG1 Basismodul LAN+TP` | `DEVICE_NAME` — unveränderlich, auch wenn PID 2 überschrieben wurde | **NEU** |
| **205** | **PID_MFR_BUILD_TIMESTAMP** | PDT_UNSIGNED_LONG `0x09` | 4 B | 1 | 4 B | R3 / ro | `6A A7 B1 94` → 2026-09-14 09:50:28 | `BUILD_TIMESTAMP` — Sekunden seit 1970, `0` = nicht gesetzt | **NEU** |
| **206** | **PID_MFR_FIRMWARE_BUILD** | PDT_UNSIGNED_CHAR `0x02` | 1 B | 16 | 16 B | R3 / ro | `b90b324` | `MAIN_Version` (kurzer Git-Hash) | **NEU** |

**Zugriff** in der Form *Lesestufe / Schreibstufe*. `R3` heißt: ohne Authentisierung lesbar
(Stufe 3 ist die niedrigste Hürde). `ro` heißt: das Gerät weist jeden Schreibversuch ab.

**Zehn Felder sind neu:** 2, 9, 16, 19, 52, 55, 203, 204, 205, 206 — das Device Object wächst damit von
14 auf 24 Felder (Maske 07B0).

Die Maxima sind frei gewählt, nicht vorgegeben. Weil die nur lesbaren Textfelder direkt aus dem Flash
lesen (`CallbackProperty`), kostet ein hohes Maximum dort **null RAM** — es begrenzt nur, wie weit ein
Lesezugriff reichen darf und was der Deskriptor meldet. Gemessen über alle 103 Hardware-Definitionen in
OGM-HardwareConfig ist der längste `DEVICE_NAME` **46 Zeichen**, die längste `DEVICE_ID` **32**. Einzig
bei PID 2 kostet das Maximum etwas, weil dort ein Schreibpuffer belegt wird: 80 Byte Heap, und das erst
beim ersten Schreibzugriff.

---

## 3. Ist das KNX-konform?

`Volume 6 Profiles` legt pro Maske fest, ob ein Feld vorhanden sein **muss**, **darf** oder
**nicht darf** — und ob es dabei nur lesbar sein darf. Die Notation steht dort in
**Anhang A.1.2.1, Table 3, gedruckte Seite 134**:

| Symbol | Bedeutung | read-only erlaubt? |
|---|---|---|
| `m/x` | Pflicht, soll nur lesbar sein | ja |
| `m/n` | Pflicht, soll mit Stufe *n* beschreibbar sein | **nein** |
| `(m/x)` | optional, nur lesbar | ja |
| `(m/n)` | optional, wenn vorhanden dann beschreibbar | **nein** |
| `(m/(n))` | optional, darf nur lesbar sein | ja |
| `X` | darf **nicht** als Data Property existieren | — |

Die Profiltabelle fürs Device Object steht in **Anhang A.2.3, gedruckte Seiten 138–140**
(S-Mode-Endgeräte) bzw. **A.3.2, Seiten 152–154** (Koppler und Schnittstellen).

### Bewertung je Feld, Maske 0x07B0

| PID | Profil 07B0 | heißt | unsere Umsetzung | konform | Fundstelle Definition | Fundstelle Profil |
|----:|---|---|---|:---:|---|---|
| 1 | `3/x` | Pflicht, nur lesbar | ro | ja | `03_05_01` §4.2.1, S. 30 | Vol 6 A.2.3, S. 138 |
| **2** | `(3/3)` | optional, muss beschreibbar sein | RW, Stufe 3 | ja | `03_05_01` §4.2.2, S. 30 | Vol 6 A.2.3, S. 138 |
| **9** | `(3/x)` | optional, nur lesbar | ro | ja | `03_05_01` §4.2.9, S. 33 | Vol 6 A.2.3, S. 138 |
| 11 | `3/x` | Pflicht, nur lesbar | ro | ja | `03_05_01` §4.2.11, S. 33 | Vol 6 A.2.3, S. 138 |
| 12 | `3/x` | Pflicht, nur lesbar | ro | ja | `03_05_01` §4.2.12, S. 33 | Vol 6 A.2.3, S. 138 |
| 14 | `3/3` | Pflicht, beschreibbar | RW | ja | `03_05_01` §4.2.14, S. 34 | Vol 6 A.2.3, S. 138 |
| 15 | `3/x` | Pflicht, nur lesbar | ro | ja | `03_05_01` §4.2.15, S. 36 | Vol 6 A.2.3, S. 138 |
| **16** | `3/x` | Pflicht, nur lesbar | ro | ja | `03_05_01` §4.2.16, S. 37 | Vol 6 A.2.3, S. 138 |
| **19** | `(3/x)` | optional, nur lesbar | ro | ja | `03_05_01` §4.2.19, S. 38 | Vol 6 A.2.3, S. 138 |
| 25 | `3/x` | Pflicht, nur lesbar | ro | ja | `03_05_01` §4.2.25, S. 39 | Vol 6 A.2.3, S. 138 |
| 51 | `3/3` | Pflicht, beschreibbar | RW | ja | `03_05_01` §4.3.2, S. 44 | Vol 6 A.2.3, S. 139 |
| **52** | `3/3` | Pflicht, beschreibbar | RW | ja | `03_05_01` §4.3.3, S. 44 | Vol 6 A.2.3, S. 139 |
| 54 | `3/3` | Pflicht, beschreibbar | RW | ja | `03_05_01` §4.3.5, S. 45 | Vol 6 A.2.3, S. 139 |
| **55** | `(3/x)` | optional, nur lesbar | ro | ja | `03_05_01` §4.3.6, S. 46 | Vol 6 A.2.3, S. 139 |
| 56 | `3/x` | Pflicht bei langen Telegrammen | ro | ja | `03_05_01` §4.3.7, S. 46 | Vol 6 A.2.3, S. 139 |
| 57 | `3/0` + Fn 59 | nur bei Geräten **mit** EMI | ro, vorhanden | Grenzfall | `03_05_01` §4.3.8, S. 47 | Vol 6 A.2.3, S. 139 |
| 58 | `3/0` + Fn 59 | dito | ro, vorhanden | Grenzfall | `03_05_01` §4.3.9, S. 47 | Vol 6 A.2.3, S. 139 |
| 71 | *leer* | für 07B0 nicht spezifiziert | ro, vorhanden | offen | `03_05_01` §4.3.22, S. 72 | Vol 6 A.2.3, S. 139 |
| 78 | `(3/1)` | optional, beschreibbar Stufe 1 | RW Stufe 3 | ja | `03_05_01` §4.3.28, S. 78 | Vol 6 A.2.3, S. 140 |
| 83 | *leer* | für 07B0 nicht spezifiziert | ro, vorhanden | offen | `03_05_01` §4.3.33, S. 82 | Vol 6 A.2.3, S. 140 |
| **203** | Herstellerbereich | frei verwendbar | ro | ja | `03_05_01` §4.3.1, S. 43 | — |
| **204** | Herstellerbereich | frei verwendbar | ro | ja | `03_05_01` §4.3.1, S. 43 | — |

Zu den Grenzfällen: Fussnote 59 zu PID 57/58 sagt *"Mandatory for devices with an External Message
Interface. Not allowed for devices without"*. Für das IP-Interface mit cEMI-Server ist das also
Pflicht und richtig; auf einem reinen TP-Gerät wären die beiden streng genommen unzulässig. Das
ist **kein** neuer Punkt, beide Felder gibt es seit jeher. PID 71 und 83 sind in A.2.3 nur für
Maske 57B0h ausgefüllt; für das KNXnet/IP-Gerätemanagement braucht man sie trotzdem.

### PID 2 ist beschreibbar — und PID 204 fängt auf, was das kostet

Das Profil verlangt für PID 2 auf jeder Maske, die es führt, `(3/3)`: optional, aber **wenn vorhanden,
dann beschreibbar**. Das Gerät nimmt einen Schreibzugriff daher an und liefert danach den geschriebenen
Namen zurück. Zwei Dinge muss man dazu wissen:

**Der geschriebene Wert ist flüchtig.** Es gibt keinen nichtflüchtigen Speicher dafür — jeder Neustart,
egal ob Spannung weg, `restart`, Watchdog oder OTA-Reboot, stellt den Herstellernamen wieder her. Der
Standard fordert Persistenz hier auch nicht: wo er sie will, schreibt er es ausdrücklich hin.

```
4.3.18 PID_PSU_TYPE     • Memory type: Non-volatile memory.
4.3.19 PID_PSU_STATUS   • Memory type: Volatile memory.
4.2.2  PID_OBJECT_NAME  (keine Angabe)
```

**Deshalb gibt es PID 204.** Sobald PID 2 überschreibbar ist, könnte ein Management-Client die
Herstellerangabe zerstören. PID 204 hält denselben Text unveränderlich bereit. `ftc` zeigt die Zeile
"Gerätename (ab Werk)" nur an, wenn sie von PID 2 abweicht.

### Was pro Maske ausgebaut wird

Die Änderung liegt in geteilten Bibliotheken, also bekommt sie **jedes** OpenKNX-Gerät. Der Stack
bedient genau fünf Masken. Zwei Felder verbietet das Profil ausdrücklich, sie fehlen dort:

| Maske | Gerätetyp | Felder | Ausnahme |
|---|---|---:|---|
| `0x07B0` | TP1 System B, Endgerät | 24 | — |
| `0x091A` | KNXnet/IP-Koppler | 23 | **ohne PID 19** — A.3.2 verlangt dort `(3/0)`, also beschreibbar |
| `0x27B0` | System 7 / RF | 25 | zusätzlich PID 82 |
| `0x2920` | RF-Koppler | 23 | **ohne PID 9 und PID 19** — `06_02_42 mask 2920h` §2.5.2.1, S. 24, Fn 10: *"PID_FIRMWARE_REVISION is not allowed. Instead, PID_VERSION shall be used."* |
| `0x57B0` | KNX IP System B | 22 | **ohne PID 16 und PID 52** — Vol 6 A.2.3, S. 139, Spalte 57B0h: `X` |

Es bleiben zwei Abweichungen, die **nur andere Masken** betreffen: PID 16 verlangt auf 57B0h `(3/3)`,
PID 19 auf 091A `(3/0)` und auf 2920h `3/1` — dort sind unsere nur lesbaren Felder zu streng. Auf 07B0,
der Maske dieses Produkts, sind beide korrekt. Beide Felder sind auf den betroffenen Masken optional,
liessen sich dort also per `#if MASK_VERSION` einfach weglassen.

> **Nachgemessen:** Das persistierte Flash-Abbild des Device Objects bleibt auf jeder Maske exakt
> gleich groß (16 Byte auf 07B0/091A/57B0, 24 auf 27B0/2920), und die Reihenfolge der gespeicherten
> Felder ändert sich nicht. Geprüft per Byte-für-Byte-Vergleich und Cross-Restore. Deshalb kein
> `apiVersion`-Sprung und keine Neuprogrammierung.

---

## 4. Auslesen

### 4.1 Über KNX TP — jedes Gerät, jedes Werkzeug

Alle Felder liegen im Device Object und sind über den normalen Management-Dienst
**`A_PropertyValue_Read`** erreichbar: verbindungsorientiert an die physikalische Adresse,
Objektindex **0**, dazu die PID. Zwei Eigenheiten muss ein Leser kennen
(`03_03_07 Application Layer` §3.4.4.1, S. 63–64):

1. **Startindex 0 liefert die Länge.** Die Antwort sind zwei Oktette mit der aktuellen Anzahl
   Elemente. Erst danach weiss man, wie lang der Text ist.
2. **Maximal 15 Elemente je Anfrage.** Das Zählfeld im Telegramm ist 4 Bit breit. Ein Textfeld mit
   einem Byte je Zeichen kommt deshalb in Blöcken zu 15 — ein 30 Zeichen langer Name braucht zwei
   Zugriffe.

Ein Lesezugriff über die aktuelle Länge hinaus wird mit *"0 Elemente, keine Daten"* beantwortet,
nicht gekürzt (Prüfvorschrift `08_03_07` §2.13.6, S. 53). Genau so ist es implementiert.

### 4.2 Mit ftc

```
# das Interface selbst  (Weg: KNXnet/IP-Gerätemanagement)
ftc -i 11.11.0.210 info

# ein Gerät am Bus     (Weg: Tunnel + A_PropertyValue_Read)
ftc -i 11.11.0.210 5.0.13 info

  Bestellnr.        IP-Iface
  Gerätename       OpenKNX REG1 Basismodul LAN+TP
  Hardware-Kennung  REG1-LAN-TP-Base
  Firmware-Name     IP-Interface (Dev)
  Produkt-ID        00FA  A5 02 13 00 0000 00 00
  PEI-Typ           0
  Wiederholungen    0x33  - Busy 3 / Nak 3
```

Mit `-q` kommen dieselben Werte als TSV (`device_name`, `hardware_id`, `firmware_name`,
`device_name_factory`, `product_id`, `pei_type`, `max_retry`). Ein Gerät mit älterer Firmware lässt die Zeilen weg und
kostet dabei keine Wartezeit — es antwortet auf eine nicht vorhandene Property sofort mit
"0 Elemente" (gemessen: 2,5 s für den gesamten Aufruf, kein Timeout).

### 4.3 Mit PowerShell, ohne jedes Werkzeug

Zwei Dateien in `scripts/`:

* **`OpenKnx.Knx.psm1`** — ein abhängigkeitsfreies Modul, das man auch in eigenen Skripten einbinden
  kann. Es kennt zwei Wege zum selben Ergebnis und gibt in beiden Fällen die Roh-Oktette zurück.
* **`Read-KnxDeviceObject.ps1`** — das Beispiel darauf: es wählt nur aus, was gelesen und wie es
  dargestellt wird.

```powershell
Import-Module ./scripts/knx/OpenKnx.Knx.psm1

# das Interface selbst, ueber Geraetemanagement
Get-KnxProperty     -InterfaceIp 11.11.0.210 -PropertyId 15
Get-KnxPropertyText -InterfaceIp 11.11.0.210 -PropertyId 2

# ein Geraet am Bus, getunnelt ueber dieselbe Schnittstelle
Get-KnxPropertyText -InterfaceIp 11.11.0.210 -Pa 5.0.13 -PropertyId 2
```

Das Beispielskript nimmt dieselben beiden Wege:

```powershell
.\scripts\Read-KnxDeviceObject.ps1 -i 11.11.0.210              # die Schnittstelle
.\scripts\Read-KnxDeviceObject.ps1 -i 11.11.0.210 -pa 5.0.13   # ein Geraet am Bus
.\scripts\Read-KnxDeviceObject.ps1 -i 11.11.0.210 -pa 5.0.13 -Property 2,19,205,206
```

```
  Device Object von Geraet 5.0.13  ueber 11.11.0.210
  ---------------------------------------------------------------
  PID   2  Object Name (Geraetename)  OpenKNX REG1 Basismodul LAN+TP
  PID  15  Order Info                 IP-Iface
  PID  19  Manufacturer Data (HW-ID)  REG1-LAN-TP-Base
  PID  25  Version                    1.3 rev 0
  PID  55  Product ID                 00 FA A5 02 13 00 00 00 00 00
  PID 203  Firmware Name (OpenKNX)    IP-Interface (Dev)
  PID 204  Device Name ab Werk        OpenKNX REG1 Basismodul LAN+TP
  PID 205  Firmware Version + Build   1.3 rev 0   gebaut 2026-09-14 09:05:21
  PID 206  Build (Git)                b90b324
```

Vier Fallen stecken im Modul, und jede davon ist beim Bauen aufgeschlagen:

* **Ein einelementiges Byte-Array wird beim `return` entrollt.** PowerShell macht daraus einen Skalar,
  `$r.Length` scheitert. Lösung: `return , ([byte[]] ...)`.
* **Der Disconnect muss ins `finally`.** Sonst bleibt die Verbindung offen und der nächste Start
  scheitert rund zwei Minuten lang mit `E_NO_MORE_CONNECTIONS (0x24)`.
* **`A_PropertyValue_Read` ist verbindungslos.** 03_03_07 §3.4.4.1 übergibt den Dienst mit
  `T_Data_Individual` — TPCI `00h`, kein `T_Connect`, keine Sequenznummern, kein `T_Ack`. Der erste
  Entwurf baute eine Transportverbindung auf; das ist mehr Code und schließt Geräte aus, die nur
  verbindungslos antworten.
* **KNXnet/IP-Tunnelling erlaubt genau EIN offenes Telegramm** (03_08_04 §2.6). Wer nach der Antwort
  zurückkehrt, ohne das `L_Data.con` des eigenen Requests abzuholen, lässt es unbestätigt — die
  Schnittstelle wiederholt es und ignoriert den nächsten Request. Symptom: *nur der erste Lesezugriff
  einer Sitzung gelingt.* Deshalb wird nach jeder Antwort kurz nachgeräumt.

### 4.4 In ETS

Belegt ist genau eines: **ETS liest vor jedem Download PID 15, PID 25 und PID 78** und bricht bei
Abweichung ab — `03_05_03 Configuration Procedures`, Abschnitt *Pre-download verification step*,
gedruckte Seite 76:

> "For System B these shall be the Properties PID_ORDER_INFO (identical), PID_VERSION (identical or
> higher), and PID_HARDWARE_TYPE (identical) of the Device Object."

Das ist auch der Grund, warum die Bestellnummer exakt stimmen muss.

**Ob die ETS-Geräteinfo daneben PID 2, 19, 55 oder 203 anzeigt, ist nicht belegt.** In den Lade-
und Managementprozeduren des Standards taucht kein solcher Zugriff auf.

#### Schreibt ETS in PID 2?

**Nein.** Drei unabhängige Belege:

1. **Die Ladeprozeduren schreiben es nicht.** Über alle Prozeduren in `03_05_03` hinweg kommen genau drei
   geschriebene Properties vor: `LdCtrlWriteProp PID="73"`, `PID="13"` und `PID="53"`. PID 2 ist nicht
   darunter.
2. **Wo `PID_OBJECT_NAME` in `03_05_03` vorkommt, wird es gelesen** — und zwar auf E-Mode-Kanalobjekten
   (Interface Object Type 14/15), nicht auf Objekt 0:
   `DMP_InterfaceObjectRead_RCl(object_index = FEC[n].ECObjectIndex[m], PID = PID_OBJECT_NAME)`.
   In `03_05_02 Management Procedures` kommt es überhaupt nicht vor.
3. **ETS schreibt nur, was die Produktdatenbank vorgibt.** Das ist der entscheidende Punkt: ETS erfindet
   keine Schreibzugriffe, es führt die Ladeprozedur aus dem knxprod aus. Unsere sieht so aus:

```
LdCtrlCompareProp   ObjIdx="0"   PropId="12"    Hersteller-ID   -> nur lesen + vergleichen
LdCtrlCompareProp   ObjIdx="0"   PropId="78"    Hardware-Typ    -> nur lesen + vergleichen
LdCtrlLoadImageProp ObjIdx="4"   PropId="27"    MCB-Tabelle     -> Parametersegment
LdCtrlWriteProp     ObjType="11" PropId="201"   Tunnel-Reservierung
LdCtrlWriteProp     ObjType="11" PropId="202"   Tunnel-Reservierung
```

Ins Device Object schreibt ETS bei uns **gar nichts**. Der Gerätename im ETS-Projektbaum ist eine
Projekteigenschaft und bleibt in der Projektdatei.

Einschränkung: Was ETS über den Standard hinaus tut, ist nirgends spezifiziert; "nie" lässt sich aus
Dokumenten allein nicht beweisen. Wer es hart sehen will, schneidet einen Download mit und sucht nach
`PropertyValue_Write` mit `Obj=00, Prop=02`:

```bash
ftc -i <zweite-schnittstelle> bm --log ~/ets-download.log --seconds 120
grep -i "PropertyValue_Write" ~/ets-download.log
```

#### Muss man die Felder "propagieren"?

Nein — und es gibt auch keinen Weg dafür. KNX kennt **keinen Mechanismus, mit dem ein Gerät seine
Properties ankündigt**. Ein Werkzeug findet sie nur auf zwei Arten:

1. Es fragt gezielt eine PID ab, die es kennt (so machen es ETS beim Download, `ftc` und das
   PowerShell-Skript).
2. Es läuft die Liste per `A_PropertyDescription_Read` mit aufsteigendem *Property Index* durch und
   bekommt zu jedem Eintrag PID, Typ und maximale Elementzahl zurück (`03_03_07` §3.4.3.3, S. 56).

Weg 2 ist die einzige echte "Selbstauskunft" — und unsere Felder tauchen dort korrekt auf. Dass ETS sie
nicht zeigt, liegt daran, dass die ETS-Geräteinfo eine **feste** Auswahl darstellt, nicht an etwas, das
auf dem Gerät fehlt. Am Gerät ist nichts nachzurüsten; es ist reine Werkzeugsache.

## 5. Bewusst nicht belegt

| PID | Name | Grund |
|----:|---|---|
| 8 | PID_SERVICE_CONTROL | Steuerregister, das u. a. das Setzen der physikalischen Adresse sperren kann. Halb verdrahtet wäre schlimmer als gar nicht vorhanden. |
| 21 | PID_DESCRIPTION | Der genormte Platz für zusätzlichen beschreibenden Text (`03_05_01` §4.2.21, S. 38). Wäre wie PID 2 nur flüchtig beschreibbar und stünde inhaltlich neben PID 2 / 203 / 204, ohne etwas hinzuzufügen. Kann jederzeit nachgezogen werden, wenn ein Bedarf auftaucht. |
| 53 | PID_ERROR_FLAGS | Die Bitbedeutungen sind nur für die Masken 0300h und 2300h definiert (Vol 6 A.2.3.2). Für System B gäbe es nichts Verbindliches. |
| 72 | PID_MGT_DESCRIPTOR_01 | `03_05_01` §4.3.23, S. 72 definiert nur zwei Strukturkennungen (System 300 auf TP1 und auf RF), 3–255 sind reserviert. Für System B existiert kein zulässiger Wert. |
| 85 / 86 | Telegrammraten-Begrenzung | Setzt eine echte Ratenbegrenzung im Data-Link-Layer voraus, die es nicht gibt. Beide dürfen nur gemeinsam existieren (Vol 6 A.2.3, Fn 64). |

---

## 6. Nebenbefund: ein Fehler in der Bestellnummer

PID 15 hat den Typ `PDT_GENERIC_10`, also genau zehn Oktette. Der Stack hat den Wert bisher mit
einem starren 10-Byte-Kopiervorgang aus einem nullterminierten Text gesetzt. Bei einer Bestellnummer
mit **acht Zeichen oder weniger** — `"IP-Iface"` hat acht — wurde damit **ein Byte über das
Stringende hinaus gelesen** und landete als zehntes Oktett auf dem Bus.

AddressSanitizer bestätigt das am alten Stand als `global-buffer-overflow, READ of size 10`:

```
vorher   49 50 2D 49 66 61 63 65 00 50    <- 'P' aus dem Nachbarliteral
nachher  49 50 2D 49 66 61 63 65 00 00
```

Auf dem konkreten ESP32-Abbild stand hinter dem Text zufällig eine Null, dort war der Fehler nicht
sichtbar. Er ist trotzdem real und trifft jede Bestellnummer mit höchstens acht Zeichen. Die Regel
"Bestellnummer auf zehn Zeichen oder weniger halten" bleibt richtig — sie war nur nicht sauber
umgesetzt.

---

## 7. Fundstellen im Code

| Was | Datei |
|---|---|
| Die Properties selbst, Textumwandlung, Lesepfad | `lib/knx/src/knx/device_object.cpp` |
| PID-Konstanten | `lib/knx/src/knx/property.h` |
| Werte einspeisen (`setupKnxDeviceInformation`) | `lib/OGM-Common/src/OpenKNX/Common.cpp` |
| Gerätename und Hardware-Kennung je Board | `lib/OGM-HardwareConfig/include/HardwareConfig/...` |
| Bestellnummer, Firmware-Name, App-Nummer | `include/knxprod.h` (vom openknxproducer erzeugt) |
| Auslesen am Interface | `ftc-cli/src/core/DeviceMgmt.h` |
| Auslesen an einem Gerät am Bus | `ftc-cli/src/core/DeviceText.h` |
| PowerShell-Referenz | `scripts/knx/Read-KnxDeviceObject.ps1` |

---

## 8. Textkodierung

`03_05_01` §4.2.2 (S. 30) verlangt für Textfelder wörtlich *"encoded in ASCII / ISO 8859-1 as an
array of characters"* — also **ein Oktett je Zeichen**. Die Quellen im HardwareConfig sind dagegen
UTF-8 kodiert; `"OpenKNX UP1 Präsenzmelder+"` hat 26 Zeichen, aber 27 UTF-8-Bytes. Die Umwandlung
passiert beim Lesen im Gerät: aus `C3 A4` wird das eine Oktett `E4`. Nicht abbildbare Zeichen
(Euro-Zeichen, Emoji) werden zu `?`.

Zum Anzeigen muss ein Werkzeug den umgekehrten Weg gehen — `ftc` und das PowerShell-Skript tun das
(Codepage 28591).

> Hinweis: dieselbe Klausel nennt in ihrer Kopfzeile `DPT_UTF_8 (28.001)`, was dem eigenen
> Fließtext widerspricht. `03_07_02` §3.27.1 (S. 65) beschreibt für diesen DPT sogar eine andere
> Semantik (ein String je Element, mit Nullterminierung). Die Umsetzung folgt dem normativen
> Fließtext von §4.2.2. Wer eine Prüfvorlage gegen `08_TSSJ` §3.8.2 schreibt, sollte den
> Unterschied kennen.

---

## 9. Was noch offen ist

### Verifikationsstand

| | |
|---|---|
| Hosttests (ASan + UBSan) | grün — Textpfad, Lese-/Schreibsemantik, Grenzen, Schreibschutz |
| Flash-Abbild | auf allen fünf Masken unverändert, Byte-für-Byte geprüft + Cross-Restore |
| Builds | 12 Firmware-Umgebungen + 8 ftc-Plattformen |
| Unabhängige Reviews | drei Prüfer (Spec, Speichersicherheit, Shared-Lib-Regression), alle Befunde abgearbeitet |
| **Hardware** | **nur ESP32** (`11.11.0.210`, Maske 07B0). RP2040 und RP2350 sind gebaut, aber **nicht am Gerät verifiziert** — auf der Testlinie 5.0.x steht kein RP-Gerät zur Verfügung. |

### Die Entscheidung zu PID 2

PID 2 ist derzeit **beschreibbar, aber flüchtig**. Das ist profilkonform und kostet nichts, aber ein
geschriebener Name überlebt keinen Neustart. Drei Möglichkeiten:

| | | Preis |
|---|---|---|
| **A** | zurück auf nur lesbar, PID 204 entfällt | nichts. Die Profilabweichung `(3/3)` bleibt bestehen und ist dokumentiert |
| **B** | so lassen *(aktueller Stand)* | nichts. Profilkonform, Herstellername durch PID 204 geschützt. Die Flüchtigkeit fällt in der Praxis nicht auf, weil kein Werkzeug PID 2 beschreibt |
| **C** | persistent im KNX-Flashabbild | `saveSize` 16 → 98 Byte, **`apiVersion` 2 → 3** ⇒ einmalige Flash-Entwertung auf **jedem** OpenKNX-Produkt |
| **D** | persistent im OpenKNX-Flash | kein `apiVersion`-Sprung, kein Produkt verliert etwas; braucht ein Callback-Paar `lib/knx` ↔ OGM-Common, etwa ein Tag mit Test |

**Vorschlag: B.** Solange belegbar kein Werkzeug PID 2 beschreibt, ist Persistenz Aufwand ohne Nutzen —
und B räumt zugleich die letzte Konformitätsabweichung auf unserer Maske weg. Wenn später ein Werkzeug
auftaucht, das Geräte benennen will, ist **D** der Weg, nicht C.

### Kleinere offene Punkte

* PID 16 auf Maske 57B0h und PID 19 auf 091A / 2920h sind bei uns strenger als das Profil verlangt. Beide
  sind dort optional, könnten also per `#if MASK_VERSION` weggelassen werden. Betrifft die Maske dieses
  Produkts nicht.
* `PID_MAX_RETRY_COUNT` (52) wird angenommen und zurückgemeldet, aber **nicht an den Data-Link-Layer
  durchgereicht**. `TPUart::DataLinkLayer::setRepetitions()` existiert mit exakt demselben Bitlayout und hat
  heute keinen Aufrufer — der gemeldete Wert `0x33` stimmt also, solange niemand schreibt.
---

## 10. Was machen andere Hersteller?

Gemessen, nicht recherchiert: Device Objects von Fremdgeräten im Netz, ausgelesen mit
`scripts/knx/Read-KnxDeviceObject.ps1` (Gerätemanagement) bzw. `ftc <pa> info` (über den Bus).

| Gerät | Hersteller | Maske | Art | PID 2 Name | PID 19 HW-Kennung | PID 21 Beschreibung | PID 55 Produkt-ID |
|---|---|---|---|:---:|:---:|:---:|:---:|
| Siemens IP-Router N 146/02 | 0x0001 | 091A | Router | – | – | – | **ja** |
| Siemens IP Interface N148 | 0x0001 | 0012 | Schnittstelle | – | – | – | – |
| MDT SCN-IP000.03 | 0x0083 | – | Schnittstelle | – | – | – | – |
| MDT-Busgerät `1.1.161` | 0x0083 | 0705 | Busgerät | – | – | – | – |
| Busgerät `1.1.2` | 0x00E1 | 0021 | Busgerät (BCU2) | – | – | – | – |
| Busgerät `1.1.4` | 0x00E1 | 0021 | Busgerät (BCU2) | – | – | – | – |
| knxd (Software) | – | – | Gateway | antwortet auf keine Property | | | |
| **OpenKNX IP-Interface** | **0x00FA** | **07B0** | Schnittstelle | **ja** | **ja** | – | **ja** |

**Kein einziges Fremdgerät füllt PID 2, 19 oder 21** — über vier Hersteller, fünf Maskenversionen
(0021, 0705, 0012, 091A) und beide Geräteklassen hinweg. Einzig der Siemens-Router führt
PID_PRODUCT_ID. Mit den beschreibenden Feldern wären wir die ersten.

Dass die Lesezugriffe die Geräte wirklich erreicht haben, ist belegt: `1.1.2` und `1.1.4` antworten auf
**PID 16 mit PEI-Typ 17** — ein Feld, das erst mit dieser Änderung überhaupt abgefragt wird. Bei
`1.1.161` kamen PID 15 und PID 78 zurück. Die Abwesenheit der übrigen Felder ist also eine echte
Abwesenheit, kein Verbindungsproblem. Fünf weitere Geräte auf 1.1 senden zwar Gruppentelegramme,
antworten aber nicht auf verbindungsorientierte Lesezugriffe innerhalb von 2,4 s.

Weitere Beobachtungen:

* **Die Bestellnummer (PID 15) ist bei den Fremdgeräten binär, nicht lesbar.** Siemens-Router:
  `16 75 C1 04 41 81 45 20 02 00`. Unsere ASCII-Bestellnummer ist eine OpenKNX-Konvention, keine
  Selbstverständlichkeit — ein Werkzeug muss auf nicht druckbare Zeichen gefasst sein und sie als Hex
  darstellen.
* **Die MDT-Schnittstelle verweigert die Gerätemanagement-Verbindung** mit Status `0x28` =
  `E_AUTHORISATION_ERROR` (03_08_02, S. 39). Auslesen geht dort nur über den Bus.
* **Beide Siemens haben eigene Properties auf PID 203 und 204.** Völlig regelkonform — der Bereich ab
  202 ist herstellerspezifisch. Es heißt aber: eine Hersteller-PID darf man **nur nach Prüfung von
  PID_MANUFACTURER_ID** interpretieren. Rohantwort des Siemens-Routers auf PID 204:

```
FB 00 00 01 CC 10 01 FD FF      1 Element, 2 Oktette - nichts mit unserem Gerätenamen zu tun
```

  Genau das hatte `ftc` anfangs falsch gemacht und Siemens-Daten als "Firmware-Name" angezeigt. Ist
  behoben: PIDs über 201 werden nur gelesen, wenn PID 12 `0x00FA` meldet.

**Einschränkung der Stichprobe:** sieben Geräte, vier Hersteller. Den Hersteller `0x00E1` konnte ich
lokal keinem Namen zuordnen — keine KNX-Master-Daten verfügbar. Produktive Geräte wurden ausschließlich
gelesen, nie beschrieben.
---

## 11. Drei echte Beispiele auf REG2

Alle Werte compilerseitig aufgelöst, nicht von Hand zusammengesetzt: die `DEVICE_*`-Makros durch
Übersetzen von `HardwareConfig.h` mit dem jeweiligen Board-Define, die `MAIN_*`-Werte aus der
`knxprod.h` des Produkts. **Vollständig** — nicht nur die neuen Felder; die neuen sind mit **NEU**
markiert.

### 11.1 OAM-NeoPixel auf REG2 PiPico2 V1

Board-Define `DEVICE_REG2_PIPICO2_V1`, Env `release_OKNXHW_REG2_PIPICO2_V1`.
OpenKnxId `0xAD`, App-Nummer 0, App-Version 8 → 0.8.

| PID | Name | Wert | Herkunft | |
|----:|---|---|---|---|
| 1 | OBJECT_TYPE | `0000` | konstant | |
| 2 | OBJECT_NAME | `OpenKNX REG2 PiPico2 V1` | `DEVICE_NAME`, 23 Zeichen | **NEU** |
| 9 | FIRMWARE_REVISION | Revision aus `openknx.init(rev)` | volles Oktett, 0…255 | **NEU** |
| 11 | SERIAL_NUMBER | `00FA` + 4 B Chip-ID | Plattform | |
| 12 | MANUFACTURER_ID | `00FA` | aus PID 11 | |
| 14 | DEVICE_CONTROL | Laufzeit-Flags | Stack | |
| 15 | ORDER_INFO | `4E 65 6F 50 69 78 65 6C 00 00` = „NeoPixel" | `MAIN_OrderNumber`, **8 Zeichen** | |
| 16 | PEI_TYPE | `00` | kein PEI | **NEU** |
| 19 | MANUFACTURER_DATA | `REG2-PP2-V1` | `DEVICE_ID`, 11 Zeichen | **NEU** |
| 25 | VERSION | `0.8 rev r` | `MAIN_ApplicationVersion 8` | |
| 51 | ROUTING_COUNT | `60` (Hop 6) | Vorgabe | |
| 52 | MAX_RETRY_COUNT | `33` | 3 Busy / 3 Nak | **NEU** |
| 54 | PROG_MODE | `00` | Programmiertaste | |
| 55 | PRODUCT_ID | `00FA AD 00 08 rr 11 20 00 00` | + HW-ID `0x1120` | **NEU** |
| 56 | MAX_APDU_LENGTH | `00FE` = 254 | konstant | |
| 57 / 58 | SUBNET / DEVICE_ADDR | aus der physikalischen Adresse | Laufzeit | |
| 71 | IO_LIST | vorhandene Objekttypen | BAU | |
| 78 | HARDWARE_TYPE | `00 00 AD 00 08 00` | OpenKnxId, App, Version | |
| 83 | DEVICE_DESCRIPTOR | Maskenversion | BAU | |
| 203 | MFR_FIRMWARE_NAME | `OpenKNX NeoPixel` | `MAIN_FirmwareName` | **NEU** |
| 204 | MFR_DEVICE_NAME | `OpenKNX REG2 PiPico2 V1` | `DEVICE_NAME`, unveränderlich | **NEU** |
| 205 | MFR_BUILD_TIMESTAMP | Sekunden seit 1970 | `BUILD_TIMESTAMP` | **NEU** |
| 206 | MFR_FIRMWARE_BUILD | Git-Kurzhash | `MAIN_Version` | **NEU** |

**Dieses Produkt war vom Bestellnummer-Fehler betroffen:** `"NeoPixel"` sind 8 Zeichen, also 9 Byte mit
Terminator — der alte Code kopierte stur 10 und legte ein Fremdbyte als zehntes Oktett auf den Bus.

### 11.2 OAM-IP-Interface auf REG2 PiPico2 V1 Ethernet

Board-Define `DEVICE_REG2_PIPICO2_V1_ETH_DD`, Env `release_REG2_PICO2_ETH_DD`.
OpenKnxId `0xA5`, App-Nummer 2, App-Version 19 → 1.3.

| PID | Name | Wert | |
|----:|---|---|---|
| 1 | OBJECT_TYPE | `0000` | |
| 2 | OBJECT_NAME | `OpenKNX REG2 PiPico2 V1 Ethernet` — 32 Zeichen, **drei Lesezugriffe** | **NEU** |
| 9 | FIRMWARE_REVISION | `00` | **NEU** |
| 11 | SERIAL_NUMBER | `00FA` + Chip-ID | |
| 12 | MANUFACTURER_ID | `00FA` | |
| 14 | DEVICE_CONTROL | Laufzeit-Flags | |
| 15 | ORDER_INFO | `49 50 2D 49 66 61 63 65 00 00` = „IP-Iface" | |
| 16 | PEI_TYPE | `00` | **NEU** |
| 19 | MANUFACTURER_DATA | `REG2-PP2-V1-ETH`, 15 Zeichen | **NEU** |
| 25 | VERSION | `1.3 rev 0` | |
| 51 | ROUTING_COUNT | `60` | |
| 52 | MAX_RETRY_COUNT | `33` | **NEU** |
| 54 | PROG_MODE | `00` | |
| 55 | PRODUCT_ID | `00 FA A5 02 13 00 11 25 00 00` — HW-ID `0x1125` | **NEU** |
| 56 | MAX_APDU_LENGTH | `00FE` | |
| 57 / 58 | SUBNET / DEVICE_ADDR | aus der PA | |
| 71 | IO_LIST | 6–8 Objekttypen | |
| 78 | HARDWARE_TYPE | `00 00 A5 02 13 00` | |
| 83 | DEVICE_DESCRIPTOR | `07 B0` | |
| 203 | MFR_FIRMWARE_NAME | `IP-Interface (Dev)` | **NEU** |
| 204 | MFR_DEVICE_NAME | `OpenKNX REG2 PiPico2 V1 Ethernet` | **NEU** |
| 205 | MFR_BUILD_TIMESTAMP | `6A A7 B1 94` = 2026-09-14 09:50:28 | **NEU** |
| 206 | MFR_FIRMWARE_BUILD | `b90b324` | **NEU** |

### 11.3 OAM-Meter auf REG2 PiPico V1 DeviceDisplay Meter

Board-Define `DEVICE_REG2_PIPICO_V1_DD_METER`.
OpenKnxId `0xA3`, App-Nummer 4, App-Version 7 → 0.7.

| PID | Name | Wert | |
|----:|---|---|---|
| 1 | OBJECT_TYPE | `0000` | |
| 2 | OBJECT_NAME | `OpenKNX REG2 PiPico V1 DeviceDisplay Meter` — 42 Zeichen | **NEU** |
| 9 | FIRMWARE_REVISION | aus `openknx.init(rev)` | **NEU** |
| 11 | SERIAL_NUMBER | `00FA` + Chip-ID | |
| 12 | MANUFACTURER_ID | `00FA` | |
| 14 | DEVICE_CONTROL | Laufzeit-Flags | |
| 15 | ORDER_INFO | `4F 70 65 6E 4B 6E 78 4D 65 74` = **„OpenKnxMet"** | |
| 16 | PEI_TYPE | `00` | **NEU** |
| 19 | MANUFACTURER_DATA | `REG2-PP-V1-DD-Meter`, 19 Zeichen | **NEU** |
| 25 | VERSION | `0.7 rev r` | |
| 51 | ROUTING_COUNT | `60` | |
| 52 | MAX_RETRY_COUNT | `33` | **NEU** |
| 54 | PROG_MODE | `00` | |
| 55 | PRODUCT_ID | `00FA A3 04 07 rr 11 03 00 00` — HW-ID `0x1103` | **NEU** |
| 56 | MAX_APDU_LENGTH | `00FE` | |
| 57 / 58 | SUBNET / DEVICE_ADDR | aus der PA | |
| 71 | IO_LIST | Objekttypen | |
| 78 | HARDWARE_TYPE | `00 00 A3 04 07 00` | |
| 83 | DEVICE_DESCRIPTOR | Maskenversion | |
| 203 | MFR_FIRMWARE_NAME | `Zaehlermodul (Beta)` | **NEU** |
| 204 | MFR_DEVICE_NAME | `OpenKNX REG2 PiPico V1 DeviceDisplay Meter` | **NEU** |
| 205 | MFR_BUILD_TIMESTAMP | Sekunden seit 1970 | **NEU** |
| 206 | MFR_FIRMWARE_BUILD | Git-Kurzhash | **NEU** |

**Hier greift die 10-Zeichen-Grenze sichtbar:** `MAIN_OrderNumber` ist „OpenKnxMeter" mit 12 Zeichen, auf
dem Bus heißt das Produkt „OpenKnxMet". Kein Fehler im Code, sondern der harte Typ `PDT_GENERIC_10`.

### Was die drei zeigen

* **Der Gerätename kommt vom Board, nicht vom Produkt.** Dieselbe NeoPixel-Firmware auf einem UP1 meldet
  einen anderen Namen. Wer wissen will *was* läuft, liest PID 203; wer wissen will *worauf*, liest
  PID 19 und 204.
* **Die Revision (`rr`) kommt bei NeoPixel und Meter aus `openknx.init(firmwareRevision)`** — nicht aus
  `MAIN_FirmwareRevision`, das dort gar nicht definiert ist. Deshalb speist der Code sie aus
  `openknx.info.firmwareRevision()`; mit dem Makro würden beide fälschlich `0` melden.
* **Beide Bestellnummer-Fallen sind real belegt:** NeoPixel mit 8 Zeichen traf den Überlauf, Meter mit
  12 Zeichen trifft die Kürzung.

> **Kein Sensormodul-Beispiel:** Es gibt **kein REG2-Sensorboard**. Die realen Sensor-Boards heißen
> `REG1-SEN-Multi`, `REG1-LAN-SEN-Multi`, `UP1-SEN-8x`, `SEN-UP1-8xTH` und `SMARTMF-SEN-V4`. Statt ein
> REG2-Sensorboard zu erfinden steht hier OAM-Meter, für das echte Produktdaten vorliegen.

---

## 12. Vorschlag: Board-Prüfung für `ftc knxota`

Der eigentliche Nutzen der neuen Felder liegt nicht im Anzeigen, sondern im **Verhindern eines falschen
Flashvorgangs**.

### Was heute verglichen wird

`ftc knxota` liest die Kennung aus der `.uf2` und vergleicht sie mit dem Gerät. Der Stempel sitzt im
UF2-Block bei Offset 288 und ist 8 Byte lang (`lib/OGM-Common/scripts/pio/patch_uf2.py`):

```
Offset  288  289  290  291   292        293        294          295
Inhalt   08  'K'  'N'  'X'   OpenKnxId  AppNummer  AppVersion   Revision
```

Verglichen wird in `ftc-cli/src/cli/KnxOta.h`, `compareVersions()`: OpenKnxId, App-Nummer und Version.
**Das Board kommt darin nicht vor.**

### Die Lücke

Ein IP-Interface-Image für REG1 und eines für REG2 tragen dieselbe Kennung `0xA5 / App 2`. Innerhalb
derselben Architektur — etwa `REG2-PP-V1-DD` (`0x1101`) gegen `REG2-PP-V1-ETH` (`0x1104`), beide RP2040 —
ist heute **nichts** da, was ein Vertauschen verhindert. Das Ergebnis wäre eine Firmware mit falschem
Pinout: TPUart an den falschen Pins, Display und SD daneben.

### Der Vorschlag

Die neuen Felder liefern die fehlende Seite bereits:

* **PID 55 Oktett 7-8** trägt `DEVICE_HW_ID` — beim Beispiel oben `11 25` für `REG2-PP2-V1-ETH`.
* **PID 19** trägt denselben Board-Namen im Klartext, für die Fehlermeldung.

Fehlt nur die Dateiseite. Konkret drei Schritte:

1. **UF2-Stempel erweitern.** Tag von 8 auf 12 Byte (UF2-Tags sind 4-Byte-ausgerichtet, 10 ginge nicht),
   Offset 296/297 nehmen `DEVICE_HW_ID` auf. Offene Frage: `patch_uf2.py` liest heute `knxprod.h`, dort
   steht `DEVICE_HW_ID` aber nicht drin — es kommt aus einem Board-Header. Der Wert muss also entweder
   über den Präprozessor aufgelöst oder beim Build in die generierte Datei geschrieben werden.
   Dasselbe gilt sinngemäß für `patch_esp32.py`.
2. **`Uf2Identity` um `hwId` erweitern** (`ftc-cli/src/core/Uf2.h`).
3. **`compareVersions()` um ein Urteil `DifferentHardware` ergänzen** — abweisen, nur mit `--force`
   durchlassen, und in der Fehlermeldung beide Klartextnamen zeigen:

```
Gerät   REG2-PP-V1-ETH   (HW-ID 0x1104)
Datei   REG2-PP-V1-DD    (HW-ID 0x1101)
Ergebnis  [FALSCHES BOARD]  -- nicht geflasht
```

**Abwärtskompatibel:** Ein Gerät ohne PID 55 oder eine Datei ohne die zwei Oktette liefert kein Urteil,
und `knxota` verhält sich wie heute. Es kann also schrittweise ausgerollt werden.

**Nicht enthalten und bewusst offen:** wie `patch_uf2.py` an `DEVICE_HW_ID` kommt. Das ist der einzige
echte Entwurfspunkt; der Rest ist Mechanik. Es ist eine Änderung an geteiltem Build-Werkzeug in
OGM-Common und betrifft die `.uf2` jedes Produkts, gehört also besprochen bevor sie gebaut wird.
---

## 13. Was wo hingehört — die Konsole gegen den Bus

Ein Gerät weiß eine Menge über sich. Die Kunst ist nicht, möglichst viel auf den Bus zu legen, sondern
jede Angabe an den Platz zu legen, an dem ein Werkzeug sie **erwartet** — und den Rest bewusst
wegzulassen. Hier die vollständige Zuordnung, Zeile für Zeile, gegen die Ausgabe von `info` und `version`
auf der Gerätekonsole.

### Was auf dem Bus landet

| Konsolenzeile | PID | Warum dort |
|---|---|---|
| `ID: REG2-PP-V1-ETH` | **19** | `PID_MANUFACTURER_DATA` ist laut 03_05_01 §4.2.19 der genormte Platz für „manufacturer specific device information" |
| `(HW ID: 0x1105)` | **55**, Oktett 7-8 | Teil des „Device Type" in `PID_PRODUCT_ID` (§4.3.6) — maschinenlesbar, für die Board-Prüfung |
| `Name: OpenKNX REG2 PiPico V1 Ethernet` | **2** und **204** | §4.2.2: im Device Object ist PID 2 „the name of the device". PID 204 hält denselben Text unveränderlich |
| `Serial number: 00FA:132F8531` | **11** | `PID_SERIAL_NUMBER`, gab es schon |
| `Firmware Name: IP-Router (Dev)` | **203** | herstellereigen — es gibt keine genormte PID für einen Firmware-*Namen* |
| `Firmware Version: 8.0.0` | **25** + **9** | PID 25 trägt Major/Minor als `DPT_Version` für ETS, PID 9 die Revision ungekürzt |
| `Number: $A11E` | **78** und **55** | OpenKnxId + App-Nummer stehen in beiden, PID 78 ist der ETS-Vergleichswert |
| `KNX-Type: Router (091A)` | **83** | `PID_DEVICE_DESCRIPTOR`, die Maskenversion |
| `Address: 5.0.0` | **57** + **58** | oberes und unteres Byte der physikalischen Adresse |
| `This Firmware: 8.0.0+f78503f` | **25** + **9** + **206** | die Zahlen aus den genormten Properties, der Git-Hash als Text in 206 — zusammen ergibt das die Zeile exakt |
| `Buildtime: 2026-09-13 08:17:09` | **205** | `BUILD_TIMESTAMP` als 32-Bit-Sekunden, damit ein Werkzeug rechnen und nicht parsen muss |

### Was bewusst **nicht** auf den Bus geht

| Konsolenzeile | Warum nicht |
|---|---|
| `KNX: 2.4.0-ec.beta.1+8af2cd5`, `Common:`, `Network:`, `FileTransfer:`, `DeviceDisplay:`, `SDCardModule:` | Acht Modulversionen wären acht weitere Properties oder ein langer Text. Sie ändern sich pro Produkt und sind über `ftc <pa> con` → `version` vollständig abrufbar. Der Git-Hash der Firmware in PID 206 identifiziert den Gesamtstand ohnehin eindeutig |
| `Free memory`, `Free stack size`, `Watchdog`, `CPU-Temp`, `CPU-Mode` | Laufzeitzustand, kein Steckbrief. Ändert sich sekündlich, gehört in die Konsole und in die Weboberfläche |
| `Hostname`, `Network: Established (11.11.0.126)` | steht bereits im KNXnet/IP-Parameterobjekt (Objekttyp 11) — dort gehört es hin, nicht ins Device Object |
| `(Configured)` beim Programmierstatus | steckt in den Ladezuständen der Tabellenobjekte, die `ftc` ohnehin liest |

### Warum PID 205 nur vier Oktette hat

Der naheliegende Weg wäre gewesen, alles als Text in PID 203 zu packen —
`"IP-Router (Dev) 8.0.0+f78503f 2026-09-13 08:17:09"`. Das sind 48 Zeichen und damit **vier**
Lesezugriffe, und jeder Leser müsste den String zerlegen.

Der zweite Entwurf legte Major, Minor, Revision und Zeitstempel gemeinsam in PID 205 — ein Telegramm,
aber eine Doppelung: Major und Minor stehen längst in PID 25, die Revision in PID 9, dort sogar
genauer. Geblieben ist deshalb **nur der Zeitstempel**, vier Oktette, die es sonst nirgends gibt:

```
PID 205   6A A7 A7 44   =  1789372228  =  2026-09-14 09:50:28
```

Ein Werkzeug setzt die Zeile aus vier Lesezugriffen zusammen, drei davon auf genormte Properties:

```
PID  25  ->  1.3          Major und Minor
PID   9  ->  0            Revision, ungekuerzt
PID 206  ->  b90b324      Build-Kennung
PID 205  ->  1789372228   Build-Zeitpunkt
```

### Gegenprobe am Gerät

Die Konsole der `11.11.0.210` meldet `1.3.0+b90b324`, `Buildtime 2026-09-14 09:05:21`. Über den Bus
gelesen:

```
PID 205  Firmware Version + Build   1.3 rev 0   gebaut 2026-09-14 09:05:21
PID 206  Build (Git)                b90b324
```

Und so setzt `ftc` es wieder zusammen, auf beiden Wegen:

```
Firmware-Name     IP-Interface (Dev)
Firmware-Stand    1.3.0+b90b324
Gebaut            2026-09-14 09:05:21
```
---

## 14. Regeln für die Werte

Jedes Feld hat eine harte Grenze, die entweder aus dem KNX-Typ kommt oder aus der Art, wie OpenKNX den
Wert erzeugt. Wer sie überschreitet, bekommt **keine Fehlermeldung** — der Wert wird abgeschnitten oder
läuft über. Deshalb hier verbindlich, was gilt.

### Die Grenzen

| Feld | Quelle | Erlaubt | Was darüber passiert | Wer es abfängt |
|---|---|---|---|---|
| **15** Bestellnummer | `MAIN_OrderNumber` | **≤ 10 Zeichen** | wird auf 10 gekürzt, ohne Hinweis | niemand — Regel beim Anlegen des Produkts |
| **2 / 204** Gerätename | `DEVICE_NAME` | ≤ 80 Zeichen | Länge meldet 80, der Rest ist nicht lesbar | `maxElements` im Gerät |
| **19** Hardware-Kennung | `DEVICE_ID` | ≤ 40 Zeichen | dito | `maxElements` |
| **203** Firmware-Name | `MAIN_FirmwareName` | ≤ 48 Zeichen | dito | `maxElements` |
| **206** Build-Kennung | `MAIN_Version` | ≤ 16 Zeichen | dito | `maxElements` |
| **25** Major | `MAIN_ApplicationVersion` oberes Nibble | **0…15** | nicht darstellbar — das Makro ist *ein* Byte | der Typ selbst |
| **25** Minor | unteres Nibble | **0…15** | dito | der Typ selbst |
| **25** Revision | `openknx.init(rev)`, dann `& 0x1F` | **0…31** | **32 meldet 0** — stiller Überlauf | *niemand* |
| **9** Revision | `openknx.init(rev)`, volles Oktett | 0…255 | nicht möglich, `uint8_t` | der Typ selbst |
| **205** Zeitstempel | `BUILD_TIMESTAMP` | bis 07.02.2106 | `uint32`-Überlauf | der Typ selbst |
| **206** Build-Kennung | `MAIN_Version` | ≤ 16 Zeichen | wird gekürzt | `maxElements` |
| **55** Hardware-ID | `DEVICE_HW_ID` | 0x0000…0xFFFF | — | `uint16` |
| **52** Wiederholungen | Vorgabe `0x33` | Bit 7 und 3 müssen 0 sein | wird auf `& 0x77` maskiert | der Code |
| alle Texte | ISO 8859-1 | Codepoints ≤ 0xFF | wird zu `?` | die Umwandlung im Gerät |

### Die Regeln in einem Satz

1. **Bestellnummer auf zehn Zeichen planen.** `PDT_GENERIC_10` ist hart; „OpenKnxMeter" heißt auf dem
   Bus „OpenKnxMet".
2. **Major und Minor bleiben unter 16.** Beide sind Nibbles *eines* Bytes. Eine Version 16.0 lässt sich
   nicht ausdrücken — das ist eine Grenze von `MAIN_ApplicationVersion`, nicht von KNX.
3. **Revision unter 32 halten, solange PID 25 stimmen soll.** Darüber ist nur PID 9 verlässlich, und
   ETS liest PID 25.
4. **Texte unter ihrem Maximum halten.** Kein Fehler, nur ein abgeschnittener Name.
5. **Umlaute ja, Euro-Zeichen nein.** Was ISO 8859-1 nicht kennt, wird zu `?`.

### Was die Grenzen wert sind: ein Gerät am Anschlag

Ein gedachtes Produkt „OAM-Maximum", das jede Grenze genau ausreizt:

| Quelle | Wert | Länge |
|---|---|---|
| `MAIN_OrderNumber` | `MaxOrder12` | 10 — exakt voll |
| `MAIN_ApplicationVersion` | `0xFF` | → Version **15.15** |
| `openknx.init(31)` | Revision 31 | das Maximum, das PID 25 noch trägt |
| `DEVICE_NAME` | 80 Zeichen | exakt voll |
| `DEVICE_ID` | 40 Zeichen | exakt voll |
| `MAIN_FirmwareName` | 48 Zeichen | exakt voll |
| `MAIN_Version` | 16 Zeichen | exakt voll |

Was dabei auf dem Bus steht:

```
PID  15   4D 61 78 4F 72 64 65 72 31 32           "MaxOrder12", kein Platz mehr
PID  25   0xFBCF                                  15.15 rev 31, alle drei Felder am Anschlag
PID   9   1F                                      Revision 31, hier waeren bis 255 moeglich
PID 205   <4 Oktette>                             Build-Zeitstempel
PID   2   80 Elemente                             sechs Lesezugriffe (15+15+15+15+15+5)
PID  19   40 Elemente                             drei Lesezugriffe
```

Und wo es kippt, jeweils um genau eins darüber:

```
MAIN_OrderNumber "MaxOrder123"   ->  PID 15 meldet "MaxOrder12"   das elfte Zeichen fehlt
openknx.init(32)                 ->  PID 25 meldet Revision 0     PID 9 meldet korrekt 32
DEVICE_NAME mit 81 Zeichen       ->  PID 2 meldet Laenge 80       Zeichen 81 ist nicht lesbar
```

Der Revisionsfall ist der gefährlichste, weil er **still von 31 auf 0 springt** statt zu kürzen. Genau
deshalb gibt es PID 9 mit dem vollen Oktett — und die Spec bietet in §4.2.9 selbst beide Orte an.

### Gegengeprüft

Alle Grenzen sind als Hosttest hinterlegt und laufen unter AddressSanitizer:

```
ok   Maximum ist 15.15                        PID 25 = 03CF -> 15.15
ok   31 ist in beiden gleich
ok   PID 25 meldet 32 faelschlich als 0
ok   PID 9 meldet die echte 32
ok   PID 9 traegt bis 255
ok   uint32 vollstaendig -> reicht bis 2106
ok   ohne BUILD_TIMESTAMP bleibt 0 = 'nicht gesetzt'
```

---

## 15. Wo Angaben doppelt stehen — und warum

Grundsatz: jede Angabe hat **einen** Platz. Vier Überschneidungen bleiben, jede mit einem Grund.

| Angabe | steht in | Grund für die Doppelung |
|---|---|---|
| Gerätename | **2** und **204** | Absicht. PID 2 darf ein Management-Client überschreiben, PID 204 nie — sonst wäre die Herstellerangabe verlierbar |
| Herstellercode | **12** und **55** Oktett 0-1 | **keine Doppelung.** PID 12 ist laut §4.2.12 der *„manufacturer of the device"*, PID 55 Oktett 0-1 heißt laut §4.3.6 *„Application Manufacturer"* — Gerätehersteller gegen Anwendungshersteller. Bei OEM-Geräten laufen die auseinander, bei OpenKNX sind beide `0x00FA`. Die Aufteilung von PID 55 ist vorgeschrieben, die zwei Oktette lassen sich nicht weglassen |
| Revision | **25** und **9** | keine echte Doppelung, sondern unterschiedliche Genauigkeit: PID 25 kappt bei 31, PID 9 trägt bis 255. §4.2.9 bietet beide Orte ausdrücklich an — ETS liest PID 25, ein Werkzeug PID 9 |

Bewusst **nicht** doppelt abgelegt: Seriennummer (nur 11), Maske (nur 83), Hardware-ID (nur 55),
Bestellnummer (nur 15), Firmware-Name (nur 203), Build-Kennung (nur 206), Build-Zeitstempel (nur 205),
Major und Minor (nur 25).

Der erste Entwurf von PID 205 trug Major, Minor und Revision zusätzlich mit — das war eine echte
Doppelung und ist entfernt. PID 205 hat jetzt vier Oktette statt sieben und enthält nur noch, was es
sonst nirgends gibt.
---

## 16. Wo Major, Minor, Revision und Build wirklich stehen

Die naheliegende Vermutung ist, `PID_FIRMWARE_REVISION` (9) trage die ganze Version. Tut es nicht.

### Was PID 9 laut Spec ist

03_05_01 §4.2.9, S. 33, wörtlich:

> • Property Datatype: PDT_UNSIGNED_CHAR
> „This Property shall contain the **revision number** of the firmware … encoded as a **single octet**
> unsigned character. This information can also be stored in PID_VERSION (PID = 25), which is encoded
> as DPT_Version."

Also: **ein Oktett, nur die Revision.** Kein Major, kein Minor, kein Build. Die Spec verweist selbst
darauf, dass dieselbe Zahl auch in PID 25 steckt.

### Die vier Teile und ihre Plätze

| Teil | PID | Format | Bereich |
|---|---|---|---|
| Major | **25** | Bits 10-6 von `DPT_Version` | 0…15 (Quelle ist ein Nibble) |
| Minor | **25** | Bits 5-0 | 0…15 (dito) |
| Revision | **25** *und* **9** | 25: Bits 15-11 → **0…31**  ·  9: volles Oktett → **0…255** | siehe unten |
| Build-Kennung | **206** | Text, Git-Kurzhash | ≤ 16 Zeichen |
| Build-Zeitpunkt | **205** | `uint32`, Sekunden seit 1970 | bis 2106 |

**Die Revision steht bewusst an zwei Stellen, und zwar mit unterschiedlicher Genauigkeit:** PID 25 kappt
sie bei 31 — eine 32 meldet dort **0**. PID 9 trägt sie ungekürzt. Das ist keine Nachlässigkeit, sondern
genau die Wahlmöglichkeit, die §4.2.9 selbst anbietet. ETS liest PID 25; wer es genau braucht, liest
PID 9.

### Was daraus für PID 205 folgte

Der erste Entwurf packte Major, Minor und Revision **zusätzlich** in PID 205, um alles in einem Telegramm
zu haben. Das war eine Doppelung von Angaben, die längst in PID 25 und PID 9 stehen. PID 205 trägt jetzt
**nur noch den Build-Zeitstempel** — vier Oktette, die es sonst nirgends gibt.

Zusammensetzen muss das ein Werkzeug ohnehin, und es liest dafür genormte Properties:

```
PID 25  ->  1.3        Major und Minor
PID  9  ->  0          Revision, voll
PID 206 ->  b90b324    Build-Kennung
PID 205 ->  1789372228 Build-Zeitpunkt
            ------------------------------------------------
ftc:        Firmware-Stand   1.3.0+b90b324
            Gebaut           2026-09-14 09:50:28
```

### Und was ist `[0]` in „Version: [0] 1.3"?

Die **Revision**, kein Force-Flag. Es ist die Darstellung von `DPT_Version`, wie ETS sie zeigt:
`[Revision] Major.Minor`. Im Code:

```
"[%u] %u.%u"   mit  (version >> 11) & 0x1F ,  (version >> 6) & 0x1F ,  version & 0x3F
                      Revision                 Major                    Minor
```

Dieselbe Null taucht in der Gerätekonsole als `This Firmware: 1.3.0+b90b324` **hinten** auf — dort nach
semver-Art als dritte Stelle. Ein Wert, zwei Schreibweisen.

---

## 17. Die Felder für die Geräteerkennung in ETS nutzen

Im knxprod steht heute bei jedem Produkt eine Prüfung, die ETS **vor** dem Download ausführt — bei
OAM-NeoPixel etwa:

```xml
<LdCtrlCompareProp InlineData="%VersionCheck%" ObjIdx="0" PropId="78">
  <OnError Cause="CompareMismatch" MessageRef="%AID%_M-1" />
</LdCtrlCompareProp>
```

ETS liest PID 78 aus dem Gerät und vergleicht den Wert mit dem aus der Produktdatenbank. Stimmt er
nicht, bricht ETS mit der hinterlegten Meldung ab.

**Was das heute erkennt:** PID 78 ist `00 00 <OpenKnxId> <AppNummer> <AppVersion> 00` — also *Produkt und
Anwendungsversion*. Ob die Firmware zum **Board** passt, sieht ETS damit nicht. Ein REG2-Image und ein
REG1-Image desselben Produkts haben denselben Wert.

**Was jetzt möglich wird:** PID 19 trägt die Board-Kennung im Klartext und ist pro Board stabil — also
genau das, was `LdCtrlCompareProp` braucht, **ohne neue Property**:

```xml
<LdCtrlCompareProp InlineData="5245 4732 2D50 5032 2D56 312D 4554 48" ObjIdx="0" PropId="19">
  <OnError Cause="CompareMismatch" MessageRef="%AID%_M-HW" />
</LdCtrlCompareProp>
```

(`InlineData` ist „REG2-PP2-V1-ETH" in ASCII-Hex.) Damit weigert sich ETS, ein Produkt auf das falsche
Board zu programmieren — dieselbe Prüfung, die Abschnitt 12 für `ftc knxota` vorschlägt, nur eben in
ETS.

Drei Dinge sind dabei zu beachten:

* **Ein Produkt, das auf mehreren Boards läuft, braucht mehrere Produkteinträge** — der Vergleich ist ein
  exakter Gleichheitstest, keine Auswahl aus einer Liste. Bei NeoPixel mit vielen Boards wäre das ein
  Eintrag je Board.
* **PID 19 ist variabel lang.** ETS liest die aktuelle Elementzahl und vergleicht den ganzen Wert; die
  `InlineData` muss also exakt so lang sein wie der Board-Name.
* **Geräte mit älterer Firmware haben PID 19 nicht.** Dort schlägt der Vergleich fehl — die Prüfung
  gehört also erst in ein Produkt, dessen Mindest-Firmware die Property mitbringt.

Wer statt Text lieber Zahlen vergleicht: die Board-Kennung steht **binär in PID 55, Oktett 7-8**
(`11 25` für `REG2-PP2-V1-ETH`). PID 55 als Ganzes taugt aber nicht für `LdCtrlCompareProp`, weil Oktett 6
die Firmware-Revision trägt und sich damit von Build zu Build ändert.
---

## 18. Beispiel: OpenKNX-RaumController-Release-5.6.1

Echte Werte aus `OAM-RaumController/include/knxprod.h`, die Boards compilerseitig aufgelöst.
Das Produkt läuft auf 19 Boards — hier drei davon, darunter das Sensormodul.

### Woher die 5.6.1 kommt

```
MAIN_ApplicationVersion 86  = 0x56  ->  Major 5, Minor 6
MAIN_FirmwareRevision    1          ->  Revision 1
                                        ergibt 5.6.1
```

Alle drei Zahlen bleiben in ihren Grenzen: Major 5 und Minor 6 unter 16, Revision 1 unter 32. Die
Version ist also vollständig in PID 25 darstellbar — `0x0D86`, und ETS zeigt sie als `[1] 5.6`.

### Die Bestellnummer wird gekürzt

```
MAIN_OrderNumber  "OpenKnxRaumController"   21 Zeichen
PID 15            4F 70 65 6E 4B 6E 78 52 61 75   =  "OpenKnxRau"
```

Elf Zeichen fallen weg. Das ist kein Fehler, sondern `PDT_GENERIC_10` — und genau der Grund, warum
diese Doku als Regel formuliert: **Bestellnummer auf zehn Zeichen planen.**

### Das Device Object, produktweite Felder

| PID | Wert | |
|----:|---|---|
| 11 | `00FA` + 4 B Chip-ID | |
| 12 | `00FA` — Gerätehersteller | |
| 15 | `OpenKnxRau` (von „OpenKnxRaumController" gekürzt) | |
| 25 | `0D86` → **5.6**, Revision 1 | |
| 9 | `01` — Revision, ungekürzt | **NEU** |
| 16 | `00` | **NEU** |
| 52 | `33` | **NEU** |
| 78 | `00 00 A0 12 56 00` — OpenKnxId `0xA0`, App 18, Version 0x56 | |
| 203 | `RaumController` | **NEU** |
| 205 | Build-Zeitstempel | **NEU** |
| 206 | Git-Kurzhash | **NEU** |
| **55** | **fehlt** auf diesen drei Boards — sie definieren keine `DEVICE_HW_ID` | **NEU** |

### Die boardabhängigen Felder

| Board-Define | PID 19 `DEVICE_ID` | PID 2 / 204 `DEVICE_NAME` |
|---|---|---|
| `DEVICE_UP1_PM_HF` | `UP1-PM-HF` (9) | `OpenKNX UP1 Präsenzmelder+` (26 Zeichen, **27 UTF-8-Bytes**) |
| `DEVICE_SMARTMF_SENSORMODUL` | `SMARTMF-SEN-V4` (14) | `Smart-MF Sensormodul V4` (23) |
| `DEVICE_REG1_LAN_TP_BASE` | `REG1-LAN-TP-Base` (16) | `OpenKNX REG1 Basismodul LAN+TP` (30) |

Das Präsenzmelder-Board ist der Fall, an dem die Kodierung sichtbar wird: der Name hat **26 Zeichen,
aber 27 UTF-8-Bytes**, weil `ä` in der Quelle `C3 A4` ist. Auf dem Bus steht ein Oktett je Zeichen, das
`ä` also als `E4` — PID 2 meldet Länge **26**, nicht 27.

### Was ein Werkzeug daraus zusammensetzt

```
ftc -i <ip> <pa> info

  Bestellnr.             OpenKnxRau
  Geraetename            OpenKNX UP1 Präsenzmelder+
  Hardware-Kennung       UP1-PM-HF
  Firmware-Name          RaumController
  Firmware-Stand         5.6.1+<hash>
  Gebaut                 <Zeitstempel>
  Hardware-Typ           00 00 A0 12 56 00
  Version                [1] 5.6
```

### Was dabei auffällt

* **PID 55 fehlt auf 19 der 20 Boards**, weil sie keine `DEVICE_HW_ID` definieren; nur der PiPico BCU
  Connector hat eine (0x1F00). Statt zehn Nulloktette zu senden, die ein Leser nicht von einem echten
  Wert unterscheiden kann, meldet das Gerät dort null Elemente. Die Board-Erkennung läuft also über
  **PID 19** im Klartext — was für `LdCtrlCompareProp` in ETS ohnehin der bessere Weg ist (Abschnitt 17).
* **Die Revision kommt aus `MAIN_FirmwareRevision 1`**, nicht aus einem Laufzeitargument: `main.cpp`
  ruft `openknx.init()` ohne Parameter, also greift der `FIRMWARE_REVISION`-Pfad.
* Ein Produkt auf 20 Boards braucht für eine ETS-Board-Prüfung **20 Produkteinträge** — der Vergleich ist
  ein Gleichheitstest, keine Auswahl aus einer Liste.

