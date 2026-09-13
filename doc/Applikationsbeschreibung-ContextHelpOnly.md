<!-- DOC HelpContext="ChannelName"-->
### Beschreibung

Der hier angegebene Name wird an verschiedenen Stellen verwendet, um diesen Kanal wiederzufinden.

* Seitenbeschreibung des Kanals
* Name vom Kommunikationsobjekt

Eine aussagekräftige Benennung erlaubt eine einfachere Orientierung innerhalb der Applikation, vor allem wenn man viele Kanäle nutzt.

<!-- DOC HelpContext="Comment"-->
### Kommentar

Hier kann man einen Freitext eingeben, der den Kanal beschreibt. Dieser Text kann mehrzeilig sein. Leider unterstütz die ETS von sich aus keine mehrzeiligen Texte. Mit dem Button unter der Textbox kann man alle Eingaben der Zeichenfolge '\n' in neue Zeilen umwandeln lassen. 

<!-- DOC -->
### Verfügbare Kanäle

Um die Applikation übersichtlicher zu gestalten, kann hier ausgewählt werden, wie viele Kanäle in der Applikation verfügbar und editierbar sind. Die Maximalanzahl der Kanäle hängt von der Firmware des Gerätes ab, dass dieses Modul verwendet.

Die ETS ist auch schneller in der Anzeige, wenn sie weniger (leere) Kanäle darstellen muss. Insofern macht es Sinn, nur so viele Kanäle anzuzeigen, wie man wirklich braucht.

Hinweis: Dies ist ein älteres Verfahren. Neuere Module verwenden stattdessen die Kanalauswahl-Tabelle, in der alle Kanäle direkt aktiviert bzw. deaktiviert werden können.

<!-- DOC HelpContext="ChannelSelect" -->
### Kanalauswahl

Hier werden alle verfügbaren Kanäle in einer Übersichtstabelle angezeigt. Für jeden Kanal lässt sich direkt festlegen, ob und in welcher Ausprägung er aktiv ist, sowie eine Beschreibung hinterlegen. Nur aktivierte Kanäle erscheinen anschließend als eigener Reiter mit den zugehörigen Einstellungen und Kommunikationsobjekten. Die Beschreibung bleibt auch bei einem deaktivierten Kanal sichtbar und editierbar.

<!-- DOC HelpContext="ChannelSuspended" -->
### Suspendiert

Damit lässt sich ein einzelner Kanal vorübergehend abschalten, zum Beispiel für Testzwecke, ohne ihn komplett zu deaktivieren. Der Unterschied zum vollständigen Deaktivieren ist wichtig: Beim Deaktivieren verschwinden die Kommunikationsobjekte des Kanals aus der ETS, wodurch auch alle Gruppenadress-Verknüpfungen verloren gehen. Beim Suspendieren bleiben die Kommunikationsobjekte und ihre Verknüpfungen erhalten, der Kanal setzt seine Funktion nur vorübergehend aus. Nach dem Aufheben der Suspendierung ist der Kanal sofort wieder mit denselben Verknüpfungen einsatzbereit.

Das ist auch bei der Fehlersuche hilfreich: Verhält sich die Anlage unerwartet, lässt sich ein bereits fertig parametrierter Kanal gezielt suspendieren, um zu prüfen, ob er die Ursache ist, ohne danach die komplette Konfiguration und Verknüpfung erneut aufbauen zu müssen.
