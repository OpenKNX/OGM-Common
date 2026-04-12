### Erweitertes "In Betrieb"

Der erweiterte ‚In-Betrieb‘-Modus liefert zusätzliche Informationen zum Gerätestatus.
Statt als einzelnes Bit (DPT-1) wird der Status nun als Byte (DPT-5) übertragen.
Der erweiterte Status wird nicht nur zyklisch, sondern auch bei Änderungen gesendet – so können Probleme wie Netzwerkfehler oder Übertemperatur sofort gemeldet werden.
Durch eine Bitmaske lassen sich dabei verschiedene Zustandsinformationen gezielt auswerten.

Struktur: `0b NRRR_TWSB`

* Das Bit **B** (`1`) repräsentiert das normale Signal "In Betrieb" (immer aktiv).
* Das Bit **S** (`2`) signalisiert den Startvorgang und wird einmalig nach Ablauf der Startverzögerung übermittelt.
* Das Bit **W** (`4`) signalisiert einen durch den Watchdog ausgelösten Neustart und wird nur in Verbindung mit dem **S**-Bit einmalig gesendet.
* Das Bit **T** (`8`) zeigt einen Übertemperaturalarm der BCU an.
* Das Bit **R** (`16`) ist für zukünftige Anwendungen reserviert.
* Das Bit **R** (`32`) ist für zukünftige Anwendungen reserviert.
* Das Bit **R** (`64`) ist für zukünftige Anwendungen reserviert.
* Das Bit **N** (`128`) zeigt an, ob eine Netzwerkverbindung besteht.

**Hinweis:** Wenn eine neue Firmware auf das Gerät übertragen wird, kommt es in manchen Fällen dazu, dass das Flag für den "Neustart durch den Watchdog" gesetzt wurde.

**Tipp:** Bei Bedarf kann das Logikmodul daraus einzelne 1-Bit-KOs erzeugen. Ein entsprechendes Beispiel lässt sich über den Konfigurationstransfer importieren und anschließend über Eingang 2 anpassen.

```
OpenKNX,cv1,*/LOG/*§f~Name=Bit%20aus%20erweitertem%20Betrieb%20ausmakieren§f~Logic=1§f~Calculate=1§f~Trigger=1§f~TriggerE1=1§f~NameInput1=Erweiterter%20Betriebsstatus§f~E1=1§f~E1Dpt=2§f~E1OtherKO:2=1§f~E1UseOtherKO=1§f~E1LowDpt5:1=0§f~NameInput2=Bitmaske%20(dezimal)§f~E2ConvertInt=5§f~E2=1§f~E2Dpt=2§f~E2LowDpt5Fix=128§f~NameOutput=ausmaskiertes%20Bit§f~OOn=8§f~OOnAll=8§f~OOnFunction=9§>Wert für Eingang 2 passend setzen!§;OpenKNX
```

