---
name: openknx-channelselect
description: Erklärt und implementiert das OpenKNX Kanalauswahl-Pattern — eigener Tab mit Kanalübersichtstabelle, TypeSelect-Sync via BASE_SyncChannelType, versteckte VisibleChannels-Konstante. Verwenden wenn ein Modul von Slider-basierter Kanalauswahl auf Tab-basierte Kanalauswahl umgestellt werden soll.
---

# OpenKNX Kanalauswahl-Pattern

## Beschlossene Regeln (Beschluss 09.07.2026)

- Kanalauswahl bekommt immer einen **eigenen Tab**, immer **unter "Allgemein"**, immer **über dem ersten Kanal-Tab**
- "Verfügbare Kanäle"-Slider entfällt überall
- Inaktive Kanäle erscheinen nicht mehr im Baum links
- "(mehr …)"-Tab entfällt überall
- Einheitliches "Suspendiert" — immer nur auf dem Kanal-Tab, niemals in der Kanalauswahl-Tabelle
- Kanaltyp enthält **niemals** "Suspendiert"

## Glossar (verbindliche Begriffe)

| Begriff | Bedeutung |
|---|---|
| **Deaktiviert** | Ausgeschalteter Kanal (Wert 0) |
| **Aktiviert** | Eingeschalteter Kanal ohne weitere Typisierung (Wert 1) |
| **Suspendiert** | Was früher "zu Testzwecken deaktiviert" o.ä. war — immer PT_Suspended, Aus/Ein |
| **Beschreibung** | Der Freitext-Name eines Kanals (ohne weitere Zusätze) |
| **Startverzögerung** | Einheitlicher Begriff, falls vorhanden |

---

## Wichtige Konzepte

**`VisibleChannels` und `PT-XxxNumChannels` entfallen vollständig:**
- Im alten Pattern steuerte `VisibleChannels` via `choose test=">=C%"` die Sichtbarkeit von Kanal-Tabs. Das entfällt — Sichtbarkeit steuert jetzt `choose =1` auf dem Active-Parameter.
- `VisibleChannels` wird in keinem C++-Code gelesen. Kein Firmware-Modul referenziert diesen Wert.
- Beide — Parameter und ParameterType — vollständig aus `share.xml` entfernen.

**Firmware-Init: nur aktive Kanäle instantiieren:**
```cpp
// BinaryInputModule.h
BinaryInputChannel* _channels[BI_ChannelCount] = {};  // nullptr-initialisiert

// BinaryInputModule.cpp setup()
BinaryInputChannel* ch = new BinaryInputChannel(i);
if (ch->isActive()) {
    _channels[i] = ch;
    _channels[i]->setup();
} else {
    delete ch;
}

// loop() und alle anderen Iterationen
if (_channels[i] != nullptr) _channels[i]->loop();
```
> Sobald ein Suspended-Parameter existiert, muss `isActive()` diesen ebenfalls prüfen.

**Ausnahme: Hardware-gebundene Kanäle (z.B. SML)**

Wenn `main.cpp` Kanalobjeckte direkt nach `setup()` für Hardware-Zuweisung verwendet (z.B. `getChannel(i)->setSerial(...)`), dürfen Kanäle **nicht** lazy allokiert werden — `getChannel()` würde `nullptr` zurückgeben und crashen. In diesem Fall alle Kanäle immer anlegen; `channel->setup()` soll selbst mit `if (ParamXxx_cType > 0)` absichern.

```cpp
// SMLModule.cpp — kein isActive()-Pattern, da main.cpp setSerial() auf alle Kanäle aufruft
void SMLModule::setup(bool configured)
{
    for (uint8_t i = 0; i < SML_ChannelCount; i++) {
        _channels[i] = new SMLChannel(i);
        _channels[i]->setup(configured);  // setup() prüft intern ParamSML_cType > 0
    }
}
```

**Parameterpräfixe:**
- `UP-` = Union-Parameter → immer in `<Union>` mit `<Memory>` → in Device-RAM gespeichert
- `P-` = Parameter → **mit** `<Memory>`: in Device-RAM; **ohne** `<Memory>`: nur in ETS gespeichert (nicht auf Gerät)
- Für TypeSelect: `P-` **ohne** Memory-Referenz → nur ETS-seitig

**Zwei Typ-Parameter pro Kanal (Typ-Variante):**
- **Haupttyp** (`UP-`, memory-backed): hat "Deaktiviert" (Wert 0), steht in der
  **Kanalauswahl-Tabelle** — nur dort wird ein Kanal aktiviert und deaktiviert.
  Auf ihn zeigt auch das `choose` des Kanal-Tabs.
- **TypeSelect** (`P-`, kein Memory): **ohne** "Deaktiviert", steht auf dem
  **Kanal-Tab** — dort wird nur der Typ gewechselt, nie deaktiviert.
- `BASE_SyncChannelType` hält beide synchron. Das `input.TypeValue > 0`-Guard
  darin sorgt dafür, dass "Deaktiviert" aus der Tabelle den gemerkten Typ auf
  dem Kanal-Tab nicht überschreibt.
- Die Sync-Funktion ist symmetrisch (dieselbe Funktion in `LRTransformationFunc`
  und `RLTransformationFunc`, sie kennt nur `input`/`output`) — welcher der
  beiden Parameter in `LParameters` und welcher in `RParameters` steht, ist
  funktional egal.

> Warum "Deaktiviert" zwingend in die Tabelle gehört: inaktive Kanäle
> erscheinen nicht im Baum links. Stünde der deaktivierbare Parameter nur auf
> dem Kanal-Tab, gäbe es keinen Weg, einen Kanal überhaupt zu aktivieren.

**Rendering — eine Tabelle pro Kanal:**
Jede Zeile in der Kanalauswahl ist eine **eigene** `Inline="true" Layout="Grid"`-Tabelle im Settings-Block. Nicht eine große Tabelle für alle Kanäle. Das ist aus Performance-Gründen zwingend (ETS rendert 100 einzeilige Tabellen wesentlich schneller als eine hundertzeilige).

---

## Kanalauswahl-Tabelle: Aufbau

### Spalten und Reihenfolge

| Spalte | Inhalt | Hinweis |
|---|---|---|
| 1 | **Kanal** | Gleicher Text wie der ParameterBlock-Tab des Kanals — z.B. "Kanal 1", "Zähler 1", "Logik 1", "SML A" |
| 2 | **Kanaltyp** (Typ-Variante) / **Kanalaktivität** (Aktiv/Inaktiv-Variante) | Typ-Variante: TypeSelect ohne "Deaktiviert". Aktiv/Inaktiv-Variante: Deaktiviert/Aktiviert als Radio-Select |
| 3 | **Beschreibung** | Freitext-Eingabefeld, **bleibt auch bei Deaktiviert sichtbar und eingebbar** |

> **Kanal-Spalte — maßgeblich ist der Channel-Tab, nicht raten:** Der angezeigte Text in Spalte 1 muss **exakt** dem `Text=`/`Name=` des Channel-Tab-`ParameterBlock` entsprechen (Block 2, `Name="Channel"` → `Name="Channel%C%Page"` bzw. `b%C%Page` mit `Text="..."`). Nicht aus der Beschreibung raten, sondern den Channel-Tab-Code direkt nachschlagen und **denselben Platzhalter** (`%C%` oder `%Z%`) 1:1 übernehmen. Beispiel-Fehler: VirtualButton hatte im Settings-Block `Text="Taster %Z%"`, während der Channel-Tab tatsächlich `Text="Taster %C%: ..."` verwendet — das ergibt "Taster A" (Settings) vs. "Taster 1" (Tab), ein Mismatch. Richtig: beide Stellen `%C%` verwenden, weil der Channel-Tab `%C%` verwendet.

> **Spaltentitel:** In der Kopfzeile (share.xml) heißt Spalte 3 immer **"Beschreibung"** — nicht "Bezeichnung", nicht "Name".

> **Spaltenbreiten:** Variieren je nach Modul und Inhalt. Feste Regel nur für Aktiv/Inaktiv-Variante mit Radio-Select: Spalte 1 **15%**, Spalte 2 **35%**, Spalte 3 **50%** — bei weniger als 35% kommt es zum Umbruch im Radio-Select. Für Typ-Variante (TypeSelect/Dropdown) sind andere Breiten möglich.

---

## Änderungen share.xml

### 1. VisibleChannels: Slider → versteckte Konstante

**ParameterType** — nur noch ein gültiger Wert:
```xml
<ParameterType Id="%AID%_PT-XxxNumChannels" Name="XxxNumChannels">
    <TypeNumber SizeInBit="8" Type="unsignedInt" minInclusive="%N%" maxInclusive="%N%" />
</ParameterType>
```

**Parameter** — `Access="None"`, kein SuffixText:
```xml
<Union SizeInBit="8">
    <Memory CodeSegment="%MID%" Offset="0" BitOffset="0" />
    <Parameter Id="%AID%_UP-%TT%00001" Name="VisibleChannels"
        ParameterType="%AID%_PT-XxxNumChannels"
        Offset="0" BitOffset="0"
        Text="Verfügbare Kanäle" Value="%N%" Access="None" />
</Union>
```

- `op:config XxxNumChannelsDefault` entfernen
- ParameterRef auf VisibleChannels aus Allgemein-Tab entfernen

### 2. Neuer ParameterType: TypeSelect (ohne "Deaktiviert")

Identische Werte wie Haupttyp, aber **Wert 0 / "Deaktiviert" fehlt**:
```xml
<ParameterType Id="%AID%_PT-XxxTypeSelect" Name="XxxTypeSelect">
    <TypeRestriction Base="Value" SizeInBit="4">
        <Enumeration Text="Typ A" Value="1" Id="%ENID%" />
        <Enumeration Text="Typ B" Value="2" Id="%ENID%" />
        <!-- alle Typen außer Deaktiviert -->
    </TypeRestriction>
</ParameterType>
```

### Einheitlicher Typbezeichner (Typ-Variante)

Der `Text=`-Wert des Haupttyp-Parameters wird an **drei Stellen identisch** übernommen:

1. **TypeSelect-Parameter** (`P-`, templ.xml): gleicher `Text=` wie Haupttyp
2. **Spalte 2 der Grid-Kopfzeile** (share.xml): gleicher `Text=` wie Haupttyp
3. Ggf. Beschriftung des Feldes auf dem Kanal-Tab — sofern kein eigener HelpContext die Bezeichnung vorgibt

**Vorgehen:** Den `Text=` des Haupttyp-Parameters (`UP-`, memory-backed) nachschlagen und diesen Wert 1:1 an alle drei Stellen übertragen. Beispiel: Haupttyp hat `Text="Logik-Operation"` → TypeSelect und Kopfzeile erhalten ebenfalls `Text="Logik-Operation"`. `Text="Kanaltyp"` ist nur ein Fallback, wenn der Haupttyp keinen sprechenden Text hat.

### 3. Kanalauswahl-Tab in der Dynamic

> **Kein Kanaldefinition-Header, keine HorizontalRuler:** Der Tab startet direkt mit der Grid-Kopfzeile. Weder `UIHint="Headline" Text="Kanaldefinition"` noch `UIHint="HorizontalRuler"` vor der Tabelle einfügen.

```xml
<ParameterBlock Id="%AID%_PB-nnn" Name="XxxKanalauswahl" Text="Kanalauswahl"
    Icon="format-list-bulleted-type" HelpContext="BASE-ChannelSelect">

    <!-- Grid-Kopfzeile — Spaltenbreiten müssen mit Settings-Zeilen in templ.xml übereinstimmen -->
    <ParameterBlock Id="%AID%_PB-nnn" Inline="true" Layout="Grid">
        <Rows><Row Id="%AID%_PB-nnn_R-1" /></Rows>
        <Columns>
            <Column Id="%AID%_PB-nnn_C-1" Width="20%" />
            <Column Id="%AID%_PB-nnn_C-2" Width="30%" />
            <Column Id="%AID%_PB-nnn_C-3" Width="50%" />
        </Columns>
        <ParameterSeparator Id="%AID%_PS-nnn" Cell="1,1" UIHint="Headline" Text="Kanal" />
        <!-- Text= vom Haupttyp-Parameter übernehmen (hier z.B. "Logik-Operation") — Fallback: "Kanaltyp" -->
        <ParameterSeparator Id="%AID%_PS-nnn" Cell="1,2" UIHint="Headline" Text="Logik-Operation" />
        <ParameterSeparator Id="%AID%_PS-nnn" Cell="1,3" UIHint="Headline" Text="Beschreibung" />
    </ParameterBlock>

    <!-- Datenzeilen aus templ.xml (je Kanal eine eigene Tabelle) -->
    <op:include href="XxxModule.templ.xml"
        xpath="//Dynamic/ChannelIndependentBlock/ParameterBlock[@Name='Settings']/*"
        type="template" prefix="Xxx" IsInner="true" />

</ParameterBlock>

<!-- Kanal-Tabs aus templ.xml -->
<op:include href="XxxModule.templ.xml"
    xpath="//Dynamic/ChannelIndependentBlock/ParameterBlock[@Name='Channel']/*"
    type="template" prefix="Xxx" IsInner="true" />
```

---

## Änderungen templ.xml

### 1. TypeSelect Parameter (P-, kein Memory)

Im Static-Bereich unter `<Parameters>`, nach dem Haupttyp-Union:
```xml
<!-- Kein Union, keine <Memory>-Referenz → nur in ETS gespeichert, nicht auf Gerät -->
<!-- Text= vom Haupttyp-Parameter übernehmen (hier z.B. "Logik-Operation") — Fallback: "Kanaltyp" -->
<Parameter Id="%AID%_P-%TT%%CC%011" Name="Ch%C%TypeSelect"
    ParameterType="%AID%_PT-XxxTypeSelect"
    Text="Logik-Operation" Value="1" />
```

> **`Text=` vom Haupttyp-Parameter ableiten:** Den `Text=`-Wert immer aus dem `Text=` des zugehörigen Haupttyp-Parameters (`UP-`, memory-backed) übernehmen. Beispiel: Haupttyp hat `Text="Logik-Operation"` → TypeSelect erhält ebenfalls `Text="Logik-Operation"`. `Text="Kanaltyp"` ist nur ein Fallback, wenn der Haupttyp keinen sprechenden Text hat.

Dazugehöriger ParameterRef unter `<ParameterRefs>`:
```xml
<ParameterRef Id="%AID%_P-%TT%%CC%011_R-%TT%%CC%01101" RefId="%AID%_P-%TT%%CC%011" />
```

### 2. ParameterCalculations (nach ParameterRefs, vor ComObjectTable)

```xml
<ParameterCalculations>
    <ParameterCalculation Id="%AID%_PC-%TT%%CC%001" Language="JavaScript"
        Name="SyncType%CC%"
        RLTransformationFunc="BASE_SyncChannelType"
        LRTransformationFunc="BASE_SyncChannelType">
        <!-- Seiten sind vertauschbar, BASE_SyncChannelType ist symmetrisch -->
        <LParameters>
            <!-- TypeSelect: kein Memory, Kanal-Tab, ohne Deaktiviert -->
            <ParameterRefRef RefId="%AID%_P-%TT%%CC%011_R-%TT%%CC%01101"
                AliasName="TypeValue" />
        </LParameters>
        <RParameters>
            <!-- Haupttyp: memory-backed, Kanalauswahl-Tabelle, mit Deaktiviert -->
            <ParameterRefRef RefId="%AID%_UP-%TT%%CC%010_R-%TT%%CC%01001"
                AliasName="TypeValue" />
        </RParameters>
    </ParameterCalculation>
</ParameterCalculations>
```

> `AliasName="TypeValue"` ist **zwingend** — `BASE_SyncChannelType` sucht genau diesen Namen.
> Implementierung in `OGM-Common/src/Common.script.js`.

### 3. Dynamic: zwei benannte Blocks

```xml
<Dynamic>
    <ChannelIndependentBlock>

        <!-- Block 1: eine Tabellenzeile pro Kanal (eigene Inline-Tabelle je Instanz!) -->
        <ParameterBlock Id="%AID%_PB-nnn" Name="Settings">
            <ParameterBlock Id="%AID%_PB-nnn" Inline="true" Layout="Grid">
                <Rows><Row Id="%AID%_PB-nnn_R-1" /></Rows>
                <Columns>
                    <!-- Gleiche Breiten wie Kopfzeile in share.xml -->
                    <Column Id="%AID%_PB-nnn_C-1" Width="20%" />
                    <Column Id="%AID%_PB-nnn_C-2" Width="30%" />
                    <Column Id="%AID%_PB-nnn_C-3" Width="50%" />
                </Columns>
                <!-- Gleicher Text wie der Kanal-Tab — modul-spezifisch anpassen! -->
                <ParameterSeparator Id="%AID%_PS-nnn" Cell="1,1" Text="Kanal %Z%" />
                <!-- Haupttyp (memory-backed, mit Deaktiviert) — hier wird aktiviert/deaktiviert -->
                <ParameterRefRef RefId="%AID%_UP-%TT%%CC%010_R-%TT%%CC%01001"
                    Cell="1,2" HelpContext="%DOC%" />
                <!-- Beschreibung bleibt auch bei Deaktiviert sichtbar -->
                <ParameterRefRef RefId="%AID%_P-%TT%%CC%000_R-%TT%%CC%00001"
                    Cell="1,3" HelpContext="BASE-ChannelName" />
            </ParameterBlock>
        </ParameterBlock>

        <!-- Block 2: Kanal-Tab (nur sichtbar wenn Haupttyp > 0) -->
        <ParameterBlock Id="%AID%_PB-nnn" Name="Channel">
            <choose ParamRefId="%AID%_UP-%TT%%CC%010_R-%TT%%CC%01001">
                <when test=">0">
                    <ParameterBlock Id="%AID%_PB-nnn" Name="Channel%C%Page"
                        Text="Kanal %C%: {{0: ...}}"
                        TextParameterRefId="%AID%_P-%TT%%CC%000_R-%TT%%CC%00001"
                        Icon="..." ShowInComObjectTree="true" HelpContext="...">

                        <!-- Kanaldefinitions-Bereich: Reihenfolge ist festgelegt -->
                        <ParameterSeparator Id="%AID%_PS-nnn"
                            Text="Kanaldefinition" UIHint="Headline" />
                        <!-- 1. Beschreibung immer zuerst -->
                        <ParameterRefRef RefId="...Beschreibung..."
                            IndentLevel="1" HelpContext="BASE-ChannelName" />
                        <!-- 2. Kanaltyp optional — TypeSelect, OHNE Deaktiviert -->
                        <!-- <ParameterRefRef RefId="...TypeSelect..." IndentLevel="1" HelpContext="%DOC%" /> -->
                        <!-- 3. Startverzögerung optional -->
                        <!-- 4. Suspendiert optional — PT_Suspended, Aus/Ein -->
                        <!-- <ParameterRefRef RefId="...Suspendiert..." IndentLevel="1" HelpContext="BASE-ChannelSuspended" /> -->

                        <!-- Kein HorizontalRuler, kein "Konfiguration"-Header nach Kanaldefinition -->
                        <!-- typspezifische Parameter folgen direkt: -->

                    </ParameterBlock>
                </when>
            </choose>
        </ParameterBlock>

    </ChannelIndependentBlock>
</Dynamic>
```

> `choose test=">0"` auf den **Haupttyp** (R, memory-backed) — nicht auf TypeSelect (L).

---

## Variante: Nur Aktiv/Inaktiv (kein Typ-Dropdown)

Manche Module haben keine Typ-Auswahl. Unterschiede zur Typ-Variante:
- **Kein TypeSelect-Parameter**, kein `ParameterCalculations`
- Aktiv/Inaktiv-Dropdown steht **in der Kanalauswahl-Tabelle** und referenziert direkt den memory-backed Parameter
- Auf dem **Kanal-Tab erscheint das Dropdown nicht nochmal**
- `choose test="=1"` statt `>0`

### ParameterType — Schreibweise vereinheitlichen

```xml
<ParameterType Id="%AID%_PT-XxxChannelActive" Name="XxxChannelActive">
    <TypeRestriction Base="Value" SizeInBit="1">
        <Enumeration Text="Deaktiviert" Value="0" Id="%ENID%" />
        <Enumeration Text="Aktiviert"   Value="1" Id="%ENID%" />
    </TypeRestriction>
</ParameterType>
```

> **Kein `UIHint="RadioButton"`** — der Wert ist im KNX-Schema ungültig. Ein 1-Bit-TypeRestriction ohne UIHint wird von ETS automatisch als RadioButton dargestellt.

> **Schreibweise prüfen:** In vorhandenen Modulen variiert dies — z.B. "Inaktiv"/"Aktiv", "Aus"/"Ein". Immer auf **"Deaktiviert" / "Aktiviert"** vereinheitlichen.

### Dritter Zustand "Suspendiert" im Typ — entfernen

```xml
<!-- ALT — nicht mehr verwenden -->
<Enumeration Text="Suspendiert" Value="2" Id="%ENID%" />
```

"Suspendiert" als Typ-Wert entfernen. Es wird zur eigenständigen PT_Suspended-Checkbox auf dem Kanal-Tab.

### Settings-Block (Aktiv/Inaktiv-Variante)

```xml
<ParameterBlock Id="%AID%_PB-nnn" Name="Settings">
    <ParameterBlock Id="%AID%_PB-nnn" Inline="true" Layout="Grid">
        <Rows><Row Id="%AID%_PB-nnn_R-1" /></Rows>
        <Columns>
            <Column Id="%AID%_PB-nnn_C-1" Width="20%" />
            <Column Id="%AID%_PB-nnn_C-2" Width="30%" />
            <Column Id="%AID%_PB-nnn_C-3" Width="50%" />
        </Columns>
        <ParameterSeparator Id="%AID%_PS-nnn" Cell="1,1" Text="Kanal %Z%" />
        <!-- Direkt der memory-backed Parameter, kein TypeSelect -->
        <ParameterRefRef RefId="%AID%_UP-%TT%%CC%010_R-%TT%%CC%01001"
            Cell="1,2" HelpContext="%DOC%" />
        <!-- Beschreibung bleibt auch bei Deaktiviert sichtbar -->
        <ParameterRefRef RefId="%AID%_P-%TT%%CC%000_R-%TT%%CC%00001"
            Cell="1,3" HelpContext="BASE-ChannelName" />
    </ParameterBlock>
</ParameterBlock>
```

### Channel-Block (Aktiv/Inaktiv-Variante)

```xml
<ParameterBlock Id="%AID%_PB-nnn" Name="Channel">
    <choose ParamRefId="%AID%_UP-%TT%%CC%010_R-%TT%%CC%01001">
        <when test="=1">
            <ParameterBlock Id="%AID%_PB-nnn" Name="Channel%C%Page" ...>
                <ParameterSeparator Id="%AID%_PS-nnn"
                    Text="Kanaldefinition" UIHint="Headline" />
                <!-- Aktiv/Inaktiv-Dropdown erscheint hier NICHT nochmal -->
                <ParameterRefRef RefId="...Beschreibung..."
                    IndentLevel="1" HelpContext="BASE-ChannelName" />
                <!-- Suspendiert optional -->
                <!-- <ParameterRefRef RefId="...Suspendiert..." IndentLevel="1" HelpContext="BASE-ChannelSuspended" /> -->

                <!-- Kein HorizontalRuler, kein "Konfiguration"-Header nach Kanaldefinition -->
                <!-- kanalspezifische Parameter folgen direkt: -->
            </ParameterBlock>
        </when>
    </choose>
</ParameterBlock>
```

---

## HelpContext-Referenz

| Verwendung | HelpContext |
|---|---|
| Beschreibung / Channel Name | `BASE-ChannelName` |
| Kanalauswahl-Tab (ParameterBlock) | `BASE-ChannelSelect` |
| Suspendiert (PT_Suspended) | `BASE-ChannelSuspended` |
| Parameter ohne eigene Hilfeseite | `Empty` |

---

## Checkliste

### Typ-Variante
- [ ] `PT-XxxNumChannels` und `VisibleChannels` vollständig entfernt (Parameter, ParameterRef, ParameterType, op:config)
- [ ] `PT-XxxTypeSelect` ohne "Deaktiviert" angelegt
- [ ] Kanalauswahl-Tab: startet **direkt** mit Grid-Header — kein Kanaldefinition-Headline, keine HorizontalRuler davor
- [ ] Grid-Header Spalte 3: Text **"Beschreibung"** (nicht "Bezeichnung")
- [ ] `P-%TT%%CC%011` TypeSelect ohne Memory + ParameterRef
- [ ] `ParameterCalculations` mit `BASE_SyncChannelType`, beide `AliasName="TypeValue"`
- [ ] Settings-Block: eigene Inline-Grid-Tabelle pro Kanal-Instanz
- [ ] Kanal-Spalte: gleicher Text wie Kanal-Tab (modul-spezifisch)
- [ ] Beschreibungsfeld in Spalte 3: bleibt bei Deaktiviert sichtbar
- [ ] Channel-Block: `choose >0` auf Haupttyp
- [ ] Reihenfolge Kanalkopf: Beschreibung → Kanaltyp → Startverzögerung → Suspendiert

### Aktiv/Inaktiv-Variante (zusätzlich)
- [ ] Kein TypeSelect-Parameter, kein `ParameterCalculations`
- [ ] PT-XxxChannelActive: **kein** `UIHint="RadioButton"` (1-Bit-TypeRestriction wird von ETS automatisch als RadioButton gerendert), Werte "Deaktiviert"/"Aktiviert"
- [ ] "Suspendiert" als Typ-Wert entfernt (→ eigenständige PT_Suspended-Checkbox)
- [ ] Aktiv/Inaktiv-Dropdown nur in Kanalauswahl-Tabelle, **nicht** auf Kanal-Tab
- [ ] Channel-Block: `choose =1`

---

## Bestehendes Modul prüfen

Diesen Abschnitt verwenden, wenn ein bereits umgebautes Modul gegen das Kanalauswahl-Pattern geprüft werden soll. Jede Frage mit Ja/Nein beantworten; bei Nein ist Nacharbeit erforderlich.

### share.xml — Prüfliste

**VisibleChannels / Slider**
- [ ] `PT-XxxNumChannels`, `VisibleChannels`-Parameter, zugehöriger ParameterRef und `op:config XxxNumChannelsDefault` sind vollständig entfernt?

**TypeSelect (nur Typ-Variante)**
- [ ] Gibt es einen `PT-XxxTypeSelect` (o.ä.) ohne den Wert "Deaktiviert" (Wert 0)?

**Kanalauswahl-Tab**
- [ ] Beginnt der Tab **direkt** mit dem Grid-Block — kein `UIHint="Headline"`, kein `UIHint="HorizontalRuler"` davor?
- [ ] Gibt es eine Grid-Kopfzeile (`Inline="true" Layout="Grid"`) mit den Spalten Kanal / Kanaltyp / Beschreibung?
- [ ] Ist der Text von Spalte 2 der Kopfzeile vom `Text=` des Haupttyp-Parameters übernommen — **nicht** pauschal "Kanaltyp"?
- [ ] Heißt Spalte 3 in der Kopfzeile **"Beschreibung"** (nicht "Bezeichnung", nicht "Name")?
- [ ] Gibt es zwei `op:include` — eines für `[@Name='Settings']/*` und eines für `[@Name='Channel']/*`?
- [ ] Hat der Kanalauswahl-Tab (`ParameterBlock`) `HelpContext="BASE-ChannelSelect"`?

---

### templ.xml — Prüfliste

**TypeSelect-Parameter (nur Typ-Variante)**
- [ ] Gibt es einen `P-`-Parameter für den TypeSelect (kein `UP-`)?
- [ ] Hat dieser Parameter **keine** `<Memory>`-Referenz (nur ETS-seitig gespeichert)?
- [ ] Gibt es einen zugehörigen `ParameterRef` im Refs-Block?
- [ ] Ist `Text=` vom Haupttyp-Parameter übernommen — **nicht** pauschal "Kanaltyp"?

**ParameterCalculations (nur Typ-Variante)**
- [ ] Gibt es einen `ParameterCalculation`-Block mit `BASE_SyncChannelType` als Transformationsfunktion — in **beiden** Richtungen (`LRTransformationFunc` und `RLTransformationFunc`)?
- [ ] Haben **beide** `ParameterRefRef` (TypeSelect und Haupttyp) `AliasName="TypeValue"`?
- [ ] Welcher der beiden in `LParameters` bzw. `RParameters` steht, ist egal — die Funktion ist symmetrisch, hier ist nichts zu prüfen.

**Settings-Block (Kanalauswahl-Tabellenzeilen)**
- [ ] Hat der äußere `ParameterBlock` `Name="Settings"`?
- [ ] Ist jede Kanalzeile eine **eigene** `Inline="true" Layout="Grid"`-Tabelle (nicht eine gemeinsame)?
- [ ] Ist der Text in Spalte 1 (Kanal-Label) **zeichengleich** zum `Text=` des Channel-Tab-`ParameterBlock` (gleicher Platzhalter `%C%`/`%Z%`, im Code nachgeschlagen, nicht geraten)?
- [ ] Steht in Spalte 2 der **Haupttyp** (`UP-`, memory-backed, **mit** "Deaktiviert") — nicht der TypeSelect?
- [ ] Ist das Beschreibungsfeld in Spalte 3 **ohne** `<choose>`-Wrapper (bleibt immer sichtbar, auch bei Deaktiviert)?
- [ ] Hat das Beschreibungsfeld `HelpContext="BASE-ChannelName"`?

**Channel-Block (Kanal-Tabs)**
- [ ] Hat der äußere `ParameterBlock` `Name="Channel"`?
- [ ] Verwendet `<choose>` den **Haupttyp** (memory-backed, mit "Deaktiviert") — nicht den TypeSelect?
- [ ] Erscheint auf dem Kanal-Tab der **TypeSelect** (ohne "Deaktiviert") — nicht der Haupttyp (nur Typ-Variante)?
- [ ] Lautet die Bedingung `>0` (Typ-Variante) bzw. `=1` (Aktiv/Inaktiv-Variante)?
- [ ] Beginnt der Kanal-Tab mit `ParameterSeparator UIHint="Headline" Text="Kanaldefinition"`?
- [ ] Ist die Reihenfolge im Kanaldefinitions-Bereich: **Beschreibung → Kanaltyp → Startverzögerung → Suspendiert**?
- [ ] Kein `UIHint="HorizontalRuler"` und kein generischer Sub-Header wie "Konfiguration" nach dem Kanaldefinitions-Block?
- [ ] Erscheint das Aktiv/Inaktiv-Dropdown **nicht** auf dem Kanal-Tab (nur Aktiv/Inaktiv-Variante)?

**Terminologie**
- [ ] Wird durchgehend **"Beschreibung"** verwendet (nicht "Bezeichnung")?
- [ ] Hat das Beschreibungsfeld überall `HelpContext="BASE-ChannelName"`?
- [ ] Hat das Suspendiert-Feld überall `HelpContext="BASE-ChannelSuspended"`?
- [ ] Ist Suspendiert als `PT_Suspended` (Aus/Ein Radio-Button) implementiert?
- [ ] Gibt es keinen Typ-Enum-Wert "Suspendiert" (weder "Suspendiert" noch "Zu Testzwecken deaktiviert" o.ä.) mehr im TypeRestriction?
- [ ] Sind alle Schreibweisen "Inaktiv"/"Aktiv" durch "Deaktiviert"/"Aktiviert" ersetzt?
