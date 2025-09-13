### Erweitertes Speichern

Die integrierten Module können standardmäßig ihre Zustände automatisch auf dem internen Flashspeicher zwischenspeichern. Dies erfolgt beim Ausfall der Busspannung (bei TP-Geräten mit entsprechendem SAVEPIN) und bei einem Neustart des Geräts. Einige Updateskripte triggern außerdem das Speichern vor dem Aktualisieren.

Bei einem Reset durch den Watchdog oder die Reset-Taste, bei einem Absturz oder bei einem Stromausfall (ohne entsprechenden SAVEPIN), kann das rechtzeitige Speichern jedoch nicht mehr durchgeführt werden. Hier bietet sich bei Bedarf an, die Daten zyklisch oder manuell (per KO) zu speichern. Folgende Punkte sind zu beachten:

#### Flashspeicher
Ein Flashspeicher unterliegt begrenzten Schreibzyklen. Ein zu häufiges Speichern führt zu einer verkürzten Lebensdauer. Die Anzahl der Schreibzyklen sind Flashspeicher abhängig. Eine pauschale Aussage zur Beständigkeit kann somit nicht getroffen werden. Allerdings kann man bei einem RP2040 davon ausgehen, dass dieser ca. 100000 Schreibzyklen verkraftet. Um den Flashspeicher zu schützen, kann man beim zyklischen Speichern maximal "Stündlich" auswählen. Unsere Empfehlung ist aber **nicht** mehr als 4x pro Tag. Beim manuellen Speichern gibt es ebenfalls einen zeitlichen Schreibschutz.

#### Auswirkung beim RP2040/RP2350

Bei einem RP2040/RP2350 wird während des Schreibvorgangs die Verarbeitung pausiert.
Während dieser Pause können KNX-Telegramme verloren gehen. Daher sollte man sich gut überlegen, ob ein zyklisches Schreiben nötig ist. Wir empfehlen diese Option nur zu verwenden, wenn dies tatsächlich nötig ist (z.B. beim Zählermodul). Alternativ ist auch das manuelle Speichern per KO möglich, so dass man dies erst bei einer Änderung auslöst. Außerdem kann man mithilfe einer Zeitschaltuhr das zyklische Schreiben in die Nacht verlegen.

#### Zyklisches speichern

Auswahl:

- Deaktiviert
- Jede Stunde
- Alle 2 Stunden
- Alle 4 Stunden
- Alle 6 Stunden
- Täglich
- Wöchentlich

#### Manuelles speichern

Über diese Einstellung kann ein Gruppenobjekt eingeblendet werden, über das die Speicherung über Bus Telegramm mit dem Wert 1 ausgelöst werden kann.

Auswahl:

- Deaktiviert
- Aktiv mit 5 min. Schreibschutz
  Die Anzahl der Speicheroperation werden auf maximal einmal pro 5 Minuten begrenzt
- Aktiv mit 15 min. Schreibschutz
  Die Anzahl der Speicheroperation werden auf maximal einmal pro 15 Minuten begrenzt
- Aktiv mit 60 min. Schreibschutz
  Die Anzahl der Speicheroperation werden auf maximal einmal pro 60 Minuten begrenzt

