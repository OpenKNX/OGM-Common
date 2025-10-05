Dieses Release kann unter Windows 10/11 folgendermaßen installiert werden:
- falls noch nicht geschehen: Das gesamte zip in ein Verzeichnis entpacken
- das Verzeichnis, in das entpackt wurde, öffnen

Das Release besteht aus 2 Teilen, einem Teil um die ETS-Applikation zu erstellen und einem Firmware-Teil, der auf das passende Gerät übertragen werden muss.

Bauen der ETS-Applikation:
- In das Verzeichnis ETS-Applikation wechseln
- rechte Maustaste auf "Build-knxprod.ps1"
- "Mit PowerShell ausführen" wählen, ggf. die Sicherheitswarnung mit "Datei öffnen" bestätigen
    (jetzt wird eine zum Release passende Produktdatenbank *.knxprod gebaut)
- Sollten hier Warnungen oder Fehlermeldungen in der Powershell kommen, bitte im Wiki nachlesen: 
  https://openknx.atlassian.net/wiki/spaces/OpenKNX/pages/19169358/PowerShell-Scripte

Übertragen der Firmware:
- In das Verzeichnis Firmware wechseln
- In ein weiteres Verzeichnis wechseln, dass so heißt, wie das Gerät, das man in Betrieb nehmen oder aktualisieren möchte

Es gibt 3 Upload-Methoden, wobei nur die angeboten werden, die auch das Zielgerät unterstützt: 

USB-Upload (bei neuer Hardware kann man nur diese Methode nutzen):
    - Hardware an den USB-Port stecken (Hinweis: Es darf nur ein OpneKNX-Device am USB stecken),
    - rechte Maustaste auf "USB-Upload-Firmware.ps1"
    - "Mit PowerShell ausführen" wählen
        (jetzt wird die Firmware auf die Hardware geladen)
    - sobald die Firmware erfolgreich hochgeladen wurde, startet sich das Modul neu

KNX-Upload (Upload über den KNX-Bus, nur für Firmware-Update möglich):
    - Hardware muss am KNX-Bus angeschlossen sein und eine OpenKNX-Firmware muss bereits laufen (man beabsichtigt ein Firmwaere-Update),
    - rechte Maustaste auf "KNX-Upload-Firmware.ps1"
    - "Mit PowerShell ausführen" wählen und erforderliche Parameter angeben (werden abgefragt)
        (jetzt wird die Firmware auf die Hardware geladen, dauert 10-20 Minuten!!!)
    - sobald die Firmware erfolgreich hochgeladen wurde, startet sich das Modul neu

OTA-Upload (Upload über IP, nur für IP-Geräte und nur für Firmware-Update möglich):
    - Hardware muss an IP angeschlossen sein und eine OpenKNX-Firmware muss bereits laufen (man beabsichtigt ein Firmwaere-Update),
    - rechte Maustaste auf "OTA-Upload-Firmware.ps1"
    - "Mit PowerShell ausführen" wählen und erforderliche Parameter angeben (werden abgefragt)
        (jetzt wird die Firmware auf die Hardware geladen)
    - sobald die Firmware erfolgreich hochgeladen wurde, startet sich das Modul neu

Jetzt kann man die erzeugte knxprod in die ETS über den Katalog importieren und
danach wie gewohnt zuerst die Physikalische Adresse und nach der Parametrierung die Applikation programmieren.
Bitte noch die Applikationsbeschreibung beachten, dort stehen Hinweise zum update (ob man z.B. nur Firmware- oder nur ETS-Update braucht, 
normalerweise braucht man beides).
Fertig.
