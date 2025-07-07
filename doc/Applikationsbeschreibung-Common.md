<!-- DOC -->
# OpenKNX

OpenKNX ist eine offene Gemeinschaft von Hobbyisten die freie und quelloffene Software für KNX-Geräte erstellen. Um eine nachhaltige und professionelle Integration ins Smarthome zu erreichen streben wir eine weitgehende Kompatibilität zum KNX-Standard an. Mit OpenKNX hast Du die Möglichkeit bereits fertige Lösungen einzusetzen, diese individuell anzupassen oder ganz neue Lösungen zu realisieren - der modulare Ansatz bietet schnelle Erfolge durch den Einsatz bewährter Softwaremodule.

<!-- DOCCONTENT
Weitere Informationen findest Du unter: www.openknx.de - wiki.openknx.de - forum.openknx.de
DOCCONTENT -->

<!-- DOCEND -->

## Inhalte

Hier werden die Geräteübergreifenden Parameter und Kommunikationsobjekte beschrieben, die man in fast allen OpenKNX Geräten findet. 

* [Allgemein](#allgemein)
  * [Startverzögerung](#startverzögerung)
  * [In Betrieb senden alle](#in-betrieb-senden-alle)
* [Uhrzeit & Datum](#uhrzeit--datum)
  * [Empfangen über](#empfangen-über)
  * [Bei Neustart vom Bus lesen](#bei-neustart-vom-bus-lesen)
  * [Zeitzone](#zeitzone)
  * [Sommerzeit ermitteln durch](#sommerzeit-ermitteln-durch)
* [Gerätestandort](#gerätestandort)
  * [Breitengrad](#breitengrad)
  * [Längengrad](#längengrad)
* [Erweitert](#erweitert)
  * [Watchdog aktivieren](#watchdog-aktivieren)
  * [Diagnoseobjekt anzeigen](#diagnoseobjekt-anzeigen)

## **Allgemein**

<kbd>![Allgemein](pics/Allgemein.png)</kbd>

Hier werden Einstellungen getroffen, die die generelle Arbeitsweise des Gerätes bestimmen.

Die Seite "Allgemein" wird bei fast allen OpenKNX-Applikationen verwendet. Sie dient dazu, Einstellungen vorzunehmen, die bei allen OpenKNX-Geräten gleichermaßen benötigt werden.

<!-- DOC  HelpContext="Startup" -->
### **Startverzögerung**

Hier kann man festlegen, wie viel Zeit vergehen soll, bis das Gerät nach einem Neustart seine Funktion aufnimmt. Dabei ist es egal, ob der Neustart durch einen Busspannungsausfall, einen Reset über den Bus, durch ein Drücken der Reset-Taste oder durch den Watchdog ausgelöst wurde.

Da das Gerät prinzipiell (sofern parametriert) auch Lesetelegramme auf den Bus senden kann, kann mit dieser Einstellung verhindert werden, dass bei einem Busneustart von vielen Geräten viele Lesetelegramme auf einmal gesendet werden und so der Bus überlastet wird.

**Anmerkung:** Auch wenn man hier technisch bis zu 16.000 Stunden Verzögerung angeben kann, sind nur Einstellungen im Sekundenbereich sinnvoll.

<!-- DOC HelpContext="Heartbeat" -->
### **In Betrieb senden alle**

Das Gerät kann einen Status "Ich bin noch in Betrieb" über das KO 1 senden. 
Diese Option ermöglicht das periodische Senden einer Nachricht. Dadurch kann überprüft werden, ob ein Gerät noch funktioniert und erreichbar ist.

Hier wird das Sendeintervall eingestellt.

Sollte hier eine 0 angegeben werden, wird kein "In Betrieb"-Signal gesendet und das KO 1 steht nicht zur Verfügung.

<!-- DOCEND -->
## **Uhrzeit & Datum**

Die Einstellungen für Uhrzeit, Datum und zeitabhängige Berechnungen werden hier vorgenommen. 

<!-- DOC -->
### **Empfangen über**

Dieses Gerät kann Uhrzeit und Datum vom Bus empfangen. Dabei kann man wählen, ob man Uhrzeit über ein Kommunikationsobjekt und das Datum über ein anders empfangen will oder beides, Uhrzeit und Datum, über ein kombiniertes Kommunikationsobjekt.

#### **Ein kombiniertes KO**

Wählt man diesen Punkt, wird ein kombiniertes Kommunikationsobjekt für Uhrzeit/Datum (DPT 19) bereitgestellt. Der KNX-Zeitgeber im System muss die kombinierte Uhrzeit/Datum entsprechend liefern können.

#### **Zwei getrennte KOs**

Wählt man diesen Punkt, wird je ein Kommunikationsobjekt für Uhrzeit (DPT 10) und Datum (DPT 11) bereitgestellt. Der KNX-Zeitgeber im System muss die Uhrzeit und das Datum für die beiden Kommunikationsobjekte liefern können.

<!-- DOC -->
### **Bei Neustart vom Bus lesen**

Nach einem Neustart können Uhrzeit und Datum auch aktiv über Lesetelegramme abgefragt werden. Mit diesem Parameter wird bestimmt, ob Uhrzeit und Datum nach einem Neustart aktiv gelesen werden.

Wenn dieser Parameter gesetzt ist, wird die Uhrzeit und das Datum alle 20-30 Sekunden über ein Lesetelegramm vom Bus gelesen, bis eine entsprechende Antwort kommt. Falls keine Uhr im KNX-System vorhanden ist oder die Uhr nicht auf Leseanfragen antworten kann, sollte dieser Parameter auf "Nein" gesetzt werden.

<!-- DOC -->
### **Zeitzone**

Für die korrekte Berechnung der Zeit wird die Zeitzone des Standortes benötigt.

<!-- DOC -->
#### **POSIX TZ-String***

<!-- DOC Skip="2" -->
Diese Einstellung wird angezeigt, wenn bei Zeitzone "Benutzerdefiniert" ausgwählt wurde.

**Allgemeiner Aufbau:**

`STD[+/-]hh[:mm[:ss]][DST[+/-]hh[:mm[:ss]][,Start[/Time],End[/Time]]]`

**Bedeutung der einzelnen Teile:**

- `STD`  
  Abkürzung der Standardzeit (z. B. `CET` für Mitteleuropäische Zeit).

- `[+/-]hh[:mm[:ss]]`  
  Zeitverschiebung zur UTC. Positive Werte sind westlich von Greenwich (z. B. USA), negative Werte östlich (z. B. Europa).  
  Beispiel: `-1` für Mitteleuropa (eine Stunde östlich von UTC).

- `DST`  
  Abkürzung der Sommerzeit (z. B. `CEST` für Mitteleuropäische Sommerzeit).

- `[+/-]hh[:mm[:ss]]`  
  (Optional) Abweichung der Sommerzeit zur Standardzeit.

- `,Start[/Time],End[/Time]`  
  (Optional) Regeln, wann die Sommerzeit beginnt und endet.  
  Format: `M<m>.<w>.<d>` (Monat, Woche, Wochentag), z. B. `M3.5.0` = letzter Sonntag im März.


**Beispiel für Mitteleuropa (Deutschland):**

`CET-1CEST,M3.5.0/2:00:00,M10.5.0/3:00:00`

- `CET` = Standardzeit (Central European Time)
- `-1` = 1 Stunde östlich von UTC
- `CEST` = Sommerzeit (Central European Summer Time)
- `M3.5.0/2:00:00` = Sommerzeit beginnt am letzten Sonntag im März um 2:00 Uhr
- `M10.5.0/3:00:00` = Sommerzeit endet am letzten Sonntag im Oktober um 3:00 Uhr


**Weitere Beispiele:**

- UTC (keine Sommerzeit):  
  `UTC0`

- New York (USA, mit Sommerzeit):  
  `EST5EDT,M3.2.0/2,M11.1.0/2`

<!-- DOC -->
### **Sommerzeit ermitteln durch**

Hier kann man eine der verfügbaren Möglichkeiten auswählen, mit der das Gerät ermitteln kann, ob gerade die Sommerzeit aktiv ist.

#### **Kommunikationsobjekt 'Sommerzeit aktiv'**

Wird diese Option ausgewählt, muss über das Kommunikationsobjekt 'Sommerzeit aktiv' dem Gerät mitgeteilt werden, ob gerade die Sommerzeit aktiv ist.

#### **Kombiniertem Datum/Zeit-KO (DPT 19)**

Erscheint nur, wenn der Datum- bzw. Zeitempfang über ein kombiniertes Datum/Zeit-KO (DPT 19) gewählt worden ist.

Wenn der Datum- bzw. Zeitempfang über ein kombiniertes Datum/Zeit-KO (DPT 19) gewählt worden ist, kann dieses Zeittelegramm auch die Information enthalten, ob gerade die Sommerzeit aktiv ist. Wenn der Zeitgeber im System diese Information mit dem DPT 19-Telegramm mitschicken kann, sollte diese Option gewählt werden.

#### **Interne Berechnung**

Diese Option berechnet anhand der eingestellten Zeitzone die Sommerzeit.

<!-- DOC -->
## **Gerätestandort**

Für die korrekte Berechnung der Zeit für Sonnenauf- und -untergang werden die genauen Koordinaten des Standorts benötigt sowie auch die Zeitzone und die Information, ob gerade die Sommerzeit aktiv ist.

Die Geo-Koordinaten können bei Google Maps nachgeschaut werden, indem man mit der rechten Maustaste auf das Objekt klickt und die unten erscheinenden Koordinaten benutzt.

Die Standard-Koordinaten stehen für Frankfurt am Main, Innenstadt.

### **Breitengrad**

In dem Feld wird der Breitengrad des Standortes eingegeben.

### **Längengrad**

In dem Feld wird der Längengrad des Standortes eingegeben.

## Erweitert

Im folgenden können Einstellungen vorgenommen werden, die eher für erfahrene Benutzer sind.

<!-- DOC -->
### **Watchdog aktivieren**

Trotz hohen Qualitätsansprüchen, vielfältigen Tests und langem produktiven Einsatz kann man nie ausschließen, dass noch Fehler in der Firmware enthalten sind. Besonders ärgerlich sind Fehler, die ein Hardwaremodul zum hängen bringen und so die Funktion eingestellt wird.

Das Gerät bringt einen Watchdog mit, welcher es erlaubt, in Situationen, die einem "Hänger" entsprechen, die Hardware automatisch neu zu starten.

Der Vorteil eines Watchdog ist, dass er vor allem sporadische und selten vorkommende "Hänger" beseitigt, meist ohne dass man es merkt.

Der Nachteil ist, dass damit Fehler/Probleme verschleiert und umgangen werden, die besser an die Entwickler gemeldet und von ihnen gelöst werden sollten.

Mit einem 'Ja' wird der Watchdog eingeschaltet.

<!-- DOC -->
### **Diagnoseobjekt anzeigen**

Man kann bei diesem Gerät ein Diagnoseobjekt (KO 7) einschalten. Dieses Diagnoseobjekt ist primär zum Debuggen vorhanden, kann aber auch einem User bei einigen Fragen weiter helfen.

Die Grundidee vom Diagnoseobjekt: Man sendet mit der ETS Kommandos an das KO 7 und bekommt eine entsprechende Antwort. Derzeit sind nur wenige Kommandos für die Nutzung durch den Enduser geeignet, allerdings werden im Laufe der Zeit immer weitere Kommandos hinzukommen. Die Kommandos sind von den verwendeten OpenKNX-Modulen abhängig und werden in den dortigen Applikationsbeschreibungen beschrieben.

Mit einem 'Ja' wird das KO 7 'Diagnoseobjekt' freigeschaltet.

<!-- DOC -->
### **Erweitertes "In Betrieb"**

Der erweiterte "In Betrieb"-Modus liefert zusätzliche Informationen zum Gerätestatus.
Dabei wird der Status nicht mehr als einzelnes Bit (DPT-1) gesendet, sondern als Byte (DPT-5).
Mittels Bitmaske können so verschiedene Informationen ausgewertet werden.

Struktur: `0b 0000_0WS1`

* Das Bit **1** (`1 << 0`) repräsentiert das normale Signal "In Betrieb" (immer aktiv).
* Das Bit **S** (`1 << 1`) repräsentiert den Startvorgang und wird einmalig nach Ablauf der Startverzögerung übermittelt.
* Das Bit **W** (`1 << 2`) repräsentiert, ob das Gerät durch einen Watchdog neu gestartet wurde und wird nur in Verbindung mit dem Startup-Bit einmalig gesendet.

Daraus ergeben sich aktuell 3 Werte ohne die Bits auswerten zu müssen:

* 1 = Normales "In Betrieb"
* 3 oder 7 = Das Gerät ist gerade hochgefahren
* 7 = Es gab einen Neustart, verursacht durch den Watchdog

**Tipp:** Bei Bedarf kann das Logikmodul daraus einzelne 1-Bit KOs machen.

**Hinweis:** Wenn eine neue Firmware auf das Gerät übertragen wird, kommt es in manchen Fällen dazu, dass das Flag für den "Neustart durch den Watchdog" gesetzt wurde.

<!-- DOC -->
### Erweitertes Speichern

Die integrierten Module können standardmäßig ihre Zustände automatisch auf dem internen Flashspeicher zwischenspeichern. Dies erfolgt beim Ausfall der Busspannung (bei TP-Geräten mit entsprechendem SAVEPIN) und bei einem Neustart des Geräts. Einige Updateskripte triggern außerdem das Speichern vor dem Aktualisieren.

Bei einem Reset durch den Watchdog oder die Reset-Taste, bei einem Absturz oder bei einem Stromausfall (ohne entsprechenden SAVEPIN), kann das rechtzeitige Speichern jedoch nicht mehr durchgeführt werden. Hier bietet sich bei Bedarf an, die Daten zyklisch oder manuell (per KO) zu speichern. Folgende Punkte sind zu beachten:

#### Flashspeicher
Ein Flashspeicher unterliegt begrenzten Schreibzyklen. Ein zu häufiges Speichern führt zu einer verkürzten Lebensdauer. Die Anzahl der Schreibzyklen sind Flashspeicher abhängig. Eine pauschale Aussage zur Beständigkeit kann somit nicht getroffen werden. Allerdings kann man bei einem RP2040 davon ausgehen, dass dieser ca. 100000 Schreibzyklen verkraftet. Um den Flashspeicher zu schützen, kann man beim zyklischen Speichern maximal "Stündlich" auswählen. Unsere Empfehlung ist aber **nicht** mehr als 4x pro Tag. Beim manuellen Speichern gibt es ebenfalls einen zeitlichen Schreibschutz.

#### Auswirkung beim RP2040/RP2350

Bei einem RP2040/RP2350 wird während des Schreibvorgangs die Verarbeitung pausiert.
Während dieser Pause können KNX-Telegramme verloren gehen. Daher sollte man sich gut überlegen, ob ein zyklisches Schreiben nötig ist. Wir empfehlen diese Option nur zu verwenden, wenn dies tatsächlich nötig ist (z.B. beim Zählermodul). Alternativ ist auch das manuelle Speichern per KO möglich, so dass man dies erst bei einer Änderung auslöst. Außerdem kann man mithilfe einer Zeitschaltuhr das zyklische Schreiben in die Nacht verlegen.

<!-- DOC -->
#### Zyklisches speichern

<!-- DOC Skip="2" -->
Dies Option wird eingeblendet, wenn "Erweitertes Speichern" auf "Ja" gestellt ist.

Auswahl:

- Deaktiviert
- Jede Stunde
- Alle 2 Stunden
- Alle 4 Stunden
- Alle 6 Stunden
- Täglich
- Wöchentlich

<!-- DOC -->
#### Manuelles speichern

<!-- DOC Skip="2" -->
Dies Option wird eingeblendet, wenn "Erweitertes Speichern" auf "Ja" gestellt ist.

Über diese Einstellung kann ein Gruppenobjekt eingeblendet werden, über das die Speicherung über Bus Telegramm mit dem Wert 1 ausgelöst werden kann.

Auswahl:

- Deaktiviert
- Aktiv mit 5 min. Schreibschutz
  Die Anzahl der Speicheroperation werden auf maximal einmal pro 5 Minuten begrenzt
- Aktiv mit 15 min. Schreibschutz
  Die Anzahl der Speicheroperation werden auf maximal einmal pro 15 Minuten begrenzt
- Aktiv mit 60 min. Schreibschutz
  Die Anzahl der Speicheroperation werden auf maximal einmal pro 60 Minuten begrenzt



