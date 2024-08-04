### Maximale APDU-Länge

Die APDU-Länge gibt an, wie viele Daten in einem KNX-Telegramm versendet werden können. Ein Standardframe kann maximal 15 Bytes übertragen. Dies ist das Minimum, das alle Geräte unterstützen müssen. Ein Extended-Frame kann sogar bis zu 254 Bytes übertragen. Allerdings ist die maximale Größe von der verwendeten Hardware abhängig. Mit der Funktion "Ermitteln" wird automatisch versucht, die maximale APDU-Länge zu bestimmen, indem verschieden große Telegramme gesendet werden. Der ermittelte (oder manuell eingegebene) Wert hat auf die Programmierung mit der ETS keinen Einfluss. Er wird von den OpenKNX-Modulen genutzt, die direkt mit dem Gerät kommunizieren, um eine passende Telegrammlänge zu wählen, damit die Kommunikation klappt.

Wichtig ist, dass die APDU-Länge nicht nur gerätespezifisch ist. Auch verwendete Router, Interfaces oder Koppler können die maximale APDU-Länge beeinflussen. Der Wert kann also je nach verwendeter Schnittstelle in der ETS variieren.

Beachte auch, dass die Ermittlung einige Zeit in Anspruch nehmen kann. Je kleiner die maximale APDU-Länge ist, desto länger dauert die Prüfung.