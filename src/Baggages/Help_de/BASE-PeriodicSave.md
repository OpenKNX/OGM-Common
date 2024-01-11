### Zyklisches Speichern

Die integrierten Module können standardmäßig ihre Zustände automatisch auf dem internen Flashspeicher zwischenspeichern. Dies erfolgt beim Ausfall der Busspannung (bei TP-Geräten mit SAVEPIN) und bei einem Neustart des Geräts. Einige Updateskripte triggern außerdem das Speichern vor dem Upload.

Bei einem Absturz des Geräts, einem Neustart durch den Watchdog oder wenn es sich um ein IP-Gerät mit externer Stromversorgung handelt, kann das Speichern jedoch nicht mehr automatisch erfolgen.
Regelmäßiges Speichern trägt dazu bei, den Datenverlust gering zu halten.

**Warnung**: Ein Flashspeicher unterliegt begrenzten Schreibzyklen. Daher sollte das Speicherintervall so groß wie möglich und so klein wie nötig gewählt werden, um die Lebensdauer zu maximieren. Sollte die Firmware keine relevanten Daten speichern müssen, kann das zyklische Speichern komplett deaktiviert werden. Bei einer Firmware mit Zählermodul bietet sich beispielsweise ein Intervall von 4 Stunden an.