### Erweitertes "In Betrieb"

Der erweiterte "In Betrieb"-Modus liefert zusätzliche Informationen zum Gerätestatus. Dabei wir der Status nicht mehr als Boolesch (DPT-1) gesendet, sondern als Zahl (DPT-5). Mittels Bitmaske können so verschiedene Informationen ausgewertet werden.

- Das 1. Bit von rechts repräsentiert das normale Signal "In Betrieb" und ist immer gesetzt.
- Das 2. Bit von rechts repräsentiert den Startvorgang und wird einmalig nach Ablauf der Startverzögerung übermittelt.
- Das 3. Bit von rechts repräsentiert, ob das Gerät durch einen Watchdog neu gestartet wurde und wird nur in Verbindung mit dem Startup-Bit einmalig gesendet.
- Das 8. Bit von rechts repräsentiert, ob das Gerät mit einem Netzwerk verbunden ist. (Bei IP-Only-Geräten ist das Bit prinzipbedingt immer gesetzt)

**Tipp:** Bei Bedarf kann das Logikmodul daraus einzelne 1-Bit KOs machen.

**Hinweis:** Wenn eine neue Firmware auf das Gerät übertragen wird, kommt es in manchen Fällen (bei RP2040) dazu, dass das Flag für den "Neustart durch den Watchdog" gesetzt wurde.