### Erweitertes "In Betrieb"

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

