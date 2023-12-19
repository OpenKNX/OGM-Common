### Erweitertes "In Betrieb"

Der erweiterte "In Betrieb"-Modus liefert zusätzliche Informationen zum Gerätestatus. Dabei wir der Status nicht mehr als Boolesch (DPT-1) gesendet, sondern als Zahl (DPT-5). Mittels Bitmaske können so verschiedene Informationen ausgewertet werden.

- Das 8. Bit repräsentiert das normale Signal "In Betrieb" (immer aktiv).
- Das 7. Bit repräsentiert den Startvorgang und wird einmalig nach Ablauf der Startverzögerung übermittelt.
- Das 6. Bit repräsentiert, ob das Gerät durch einen Watchdog neu gestartet wurde und wird nur in Verbindung mit dem Startup-Bit einmalig gesendet.

Daraus ergeben sich aktuell 3 Werte ohne die Bits auswerten zu müssen.

- 1 = Normales "In Betrieb"
- 3 oder 7 = Das Gerät ist gerade hochgefahren
- 7 = Es gab einen Neustart durch den Watchdog initiert

**Tipp:** Bei Bedarf kann das Logikmodul daraus einzelne 1-Bit KOs machen.

**Hinweis:** Wenn eine neue Firmware auf das Gerät übertragen wird, kommt es in manchen Fällen dazu, dass das Flag für den "Neustart durch den Watchdog" gesetzt wurde.