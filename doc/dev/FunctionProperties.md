# Function Properties

PropertyId = **8**

| data[0] | Funktion  | Beschreibung                                         |
|---------|-----------|------------------------------------------------------|
| 0x00    | Status    | Liefert einen kurzen Überblick über den Gerätestatus |
| 0x01    | Info      | Modul und Versions-Info                              |
| 0x10    | KO-Access | Ermöglicht Zugriff auf KOs auch ohne Verknüpfung     |


## 0x00 Status

| Funktion                      | date[1] | date[2..3]  | date[4] | date[5..4+$len] | response | Erklärung          |
|-------------------------------|---------|-------------|---------|-----------------|----------|--------------------|
|                               |         |             |         |                 |          |                    |
| time getUptime()              | 0x00    |             |         |                 | uint32_t | Uptime in Sekunden |
| bool isConfigured()           | 0x01    |             |         |                 |          |                    |
| bool usesDualCore()           | 0x02    |             |         |                 |          |                    |
| DEVICE_ID                     | 0x03    |             |         |                 |          |                    |
|                               |         |             |         |                 |          |                    |
| number getTemperature()       | 0x10    |             |         |                 |          |                    |
| number getFreeMemory()        | 0x11    |             |         |                 |          |                    |
| number getFreeStackSize(core) | 0x12    | -/0x00/0x01 |         |                 |          |                    |
| number getPSRAM()             | 0x13    |             |         |                 |          |                    |
|                               |         |             |         |                 |          |                    |
| number getBcuStat(valtype)    | 0x2* ?  |             |         |                 |          |                    |
|                               |         |             |         |                 |          |                    |



## 0x01 Info

````
> Device
> ID:                      PiPico-BCU-Connector
> Name:                    OpenKNX PiPico BCU Connector
> Serial number:           00FA:xxxxxxxx
> Firmware
> Name:                    CK-xxxx-NO-PROD
> Version:                 0.3.0
> Number:                  $AF79
> KNX-Type:                TP (07B0)
> CPU-Mode:                Dual-Core (Temperature 16.8 °C)
> Programming
> Address:                 1.1.3 (Configured)
> Version:                 0.3
> Number:                  $AF79
> Runtime
> Free memory:             136.227 KiB (min. 136.023 KiB)
> Free stack size:         Core0: 1832 bytes - Core1: 2848 bytes
> Watchdog:                Unsupported
````


| Funktion               | date[1] | date[2..3] | date[4] | date[5..4+$len] | response | Erklärung                 |
|------------------------|---------|------------|---------|-----------------|----------|---------------------------|
|                        |         |            |         |                 |          |                           |
| getInternVersion(type) |         |            |         |                 | version  | type of {this,knx,common} |
| getModuleCount()       | 0x40    |  i         |         |                 | count    |                           |
| getModuleName(midx)    | 0x41    |  i         |         |                 | str      |                           |
| getModuleVersion(midx) | 0x42    |  i         |         |                 | ver,hash |                           |
|                        |         |            |         |                 |          |                           |
| getBuildDateTime()     |         |            |         |                 | datetime |                           |
|                        |         |            |         |                 |          |                           |
| getHwId()              |         |            |         |                 |          |                           |
| getHwName()            |         |            |         |                 | str      |                           |
|                        |         |            |         |                 | str      |                           |
|                        |         |            |         |                 |          |                           |
|                        |         |            |         |                 |          |                           |


## 0x10 KO-Access

| Funktion                      | date[1] | date[2..3] | date[4] | date[5..4+$len] | response               | Erklärung                                                       |
|-------------------------------|---------|------------|---------|-----------------|------------------------|-----------------------------------------------------------------|
| (flags,count) getConfig(koNo) | 0x00    | Ko-Number  |         |                 |                        |                                                                 |
| misc[] getCurrentValue(koNo)  | 0x02    | Ko-Number  |         |                 | 0xSS ; 0xLL ; 0xVV ... | Flag ob gesetzt, falls ja mit nachfolgender Wert-Länge und Wert |
| void sendReadRequest(koNo)    | 0x03    | Ko-Number  |         |                 |                        |                                                                 |
| bool setValue(koNo, value)    | 0x04    | Ko-Number  | len     |                 |                        |                                                                 |
| void writeToBus(koNo)         | 0x05    | Ko-Number  |         |                 |                        |                                                                 |



Beispiele:

Wert ermitteln: Zeit
16,2,0,4

Wert ermitteln: Logic
16,2,3,234

Update in-Betrieb
16,4,0,1,1,0