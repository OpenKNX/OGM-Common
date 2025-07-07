### POSIX TZ-String

**Allgemeiner Aufbau:**

`STD[+/-]hh[:mm[:ss]][DST[+/-]hh[:mm[:ss]][,Start[/Time],End[/Time]]]`

**Bedeutung der einzelnen Teile:**

- `STD`  
  Abkürzung der Standardzeit (z. B. `CET` für Mitteleuropäische Zeit).

- `[+/-]hh[:mm[:ss]]`  
  Zeitverschiebung zur UTC. Positive Werte sind westlich von Greenwich (z. B. USA), negative Werte östlich (z. B. Europa).  
  Beispiel: `-1` für Mitteleuropa (eine Stunde östlich von UTC).

- `DST`  
  Abkürzung der Sommerzeit (z. B. `CEST` für Mitteleuropäische Sommerzeit).

- `[+/-]hh[:mm[:ss]]`  
  (Optional) Abweichung der Sommerzeit zur Standardzeit.

- `,Start[/Time],End[/Time]`  
  (Optional) Regeln, wann die Sommerzeit beginnt und endet.  
  Format: `M<m>.<w>.<d>` (Monat, Woche, Wochentag), z. B. `M3.5.0` = letzter Sonntag im März.


**Beispiel für Mitteleuropa (Deutschland):**

`CET-1CEST,M3.5.0/2:00:00,M10.5.0/3:00:00`

- `CET` = Standardzeit (Central European Time)
- `-1` = 1 Stunde östlich von UTC
- `CEST` = Sommerzeit (Central European Summer Time)
- `M3.5.0/2:00:00` = Sommerzeit beginnt am letzten Sonntag im März um 2:00 Uhr
- `M10.5.0/3:00:00` = Sommerzeit endet am letzten Sonntag im Oktober um 3:00 Uhr


**Weitere Beispiele:**

- UTC (keine Sommerzeit):  
  `UTC0`

- New York (USA, mit Sommerzeit):  
  `EST5EDT,M3.2.0/2,M11.1.0/2`

