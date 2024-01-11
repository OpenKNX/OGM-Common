### Speichern on Demand

Die integrierten Module können standardmäßig ihre Zustände automatisch auf dem internen Flashspeicher zwischenspeichern. Dies erfolgt beim Ausfall der Busspannung (bei TP-Geräten mit SAVEPIN) und bei einem Neustart des Geräts. Einige Updateskripte triggern außerdem das Speichern vor dem Upload.

Bei einem Absturz des Geräts, einem Neustart durch den Watchdog oder wenn es sich um ein IP-Gerät mit externer Stromversorgung handelt, kann das Speichern jedoch nicht mehr automatisch erfolgen.
Diese Option ermöglicht es, das Speichern über ein Trigger-KO bei Bedarf manuell auszulösen.

**Warnung**: Ein Flashspeicher unterliegt begrenzten Schreibzyklen. Zu häufiges Speichern führt zu verkürzter Lebensdauer. Daher ist das KO mit einem zeitlichen Schreibschutz versehen. Der letzte Schreibvorgang darf nicht kürzer als die angegebene Zeit hersein, ansonsten wird der Schreibvorgang ignoriert. Überprüfe sorgfältig, dass dieses KO nicht zu häufig (z.B. aufgrund eines Logikfehlers) aufgerufen wird.