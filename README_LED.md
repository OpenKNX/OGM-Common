
# LED behaviour settings

| define                          | default | unit  | function                                                                                                                             |
| ------------------------------- | ------: | :---: | ------------------------------------------------------------------------------------------------------------------------------------ |
| OPENKNX_LEDEFFECT_PULSE_FREQ    |    1000 |  ms   |                                                                                                                                      |
| OPENKNX_LEDEFFECT_BLINK_FREQ    |    1000 |  ms   |                                                                                                                                      |
| OPENKNX_HEARTBEAT               |    1000 |  ms   | enable heartbeat mode (optional with with specific failure time)                                                                     |
| OPENKNX_HEARTBEAT_PRIO          |    3000 |  ms   | enable heartbeat prio mode (optional with with specific failure time)                                                                |
| OPENKNX_HEARTBEAT_FREQ          |     200 |  ms   |                                                                                                                                      |
| OPENKNX_HEARTBEAT_PRIO_ON_FREQ  |     200 |  ms   |                                                                                                                                      |
| OPENKNX_HEARTBEAT_PRIO_OFF_FREQ |    1000 |  ms   |                                                                                                                                      |

## Heartbeat (Mode: Normal)
You can enable a debug heartbeat to see if a loop is stuck. The progLed (for loop) and infoLed (for loop1) will blinking if the loop hangs.

## Heartbeat (Mode: Prio)
In the prio mode the leds blinking (`OPENKNX_HEARTBEAT_PRIO_OFF_FREQ`) and stop as soon as the relevant loop hangs.
If programing mode is active, the progLed will blink faster (`OPENKNX_HEARTBEAT_PRIO_ON_FREQ`).

So, if the device is NOT blinking, anything is wrong.

# LED-Manager

OpenKNX Common provides a LED-Manager accessible over `openknx.leds`

There are 3 ways of defining the LEDs controlled by the LED-Manager

## LED_INIT Mode

If the LED_INIT macro is defined (this should be done in the hardware configuration) this macro is called in the LED-Manager initialization.
Example for LED_INIT which defines
1) "normal" LED connected to GPIO 17, which is active when GPIO is LOW as Prog-LED
2) a RGB-LED connected to GPIO 18, 19, 20 which is active when GPIO is HIGH and the default collor is green as Info1-LED
```
#define LED_INIT() \
    openknx.leds.addLed(new OpenKNX::Led::GPIO(17, LOW), OpenKNX::Led::LED_TYPE_PROG); \
    openknx.leds.addLed(new OpenKNX::Led::GPIO_RGB(18, 19, 20, HIGH, OpenKNX::Led::Color::Green), OpenKNX::Led::LED_TYPE_INFO1);
```
## Legacy Mode

If LED_INIT is not defined, the previously used defines are used to configure the LEDs in the LED Manager initialization.

| define                          | default | unit  | function                                                                                                                             |
| ------------------------------- | ------: | :---: | ------------------------------------------------------------------------------------------------------------------------------------ |
| OPENKNX_SERIALLED_ENABLE        |   undef |       | activate the usage of Serial LEDs (WS2812, Neopixel, ARGB-LEDs), only on ESP32 and RP2040 (all LEDs must be of this type )           |
| OPENKNX_SERIALLED_PIN           |   undef |       | the GPIO to drive the Serial LEDs                                                                                                    |
| PROG_LED_PIN                    |   undef |       | the GPIO to drive the LED, if SERIALLED is enabled, the number of the LED in the strip (zero-based)                                  |
| PROG_LED_PIN_ACTIVE_ON          |   undef |       | values: LOW or HIGH, indicates at which GPIO state the LED is active (no function with SERIALLED)                                    |
| PROG_LED_COLOR                  |  63,0,0 |       | set the color for the LED, default: 50% Red - only for SERIALLED                                                                     |
| INFO1_LED_PIN                   |   undef |       | the GPIO to drive the LED, if SERIALLED is enabled, the number of the LED in the strip (zero-based)                                  |
| INFO1_LED_PIN_ACTIVE_ON         |   undef |       | values: LOW or HIGH, indicates at which GPIO state the LED is active (no function with SERIALLED)                                    |
| INFO1_LED_COLOR                 |  0,63,0 |       | set the color for the LED, default: 50% green - only for SERIALLED                                                                   |
| INFO2_LED_PIN                   |   undef |       | the GPIO to drive the LED, if SERIALLED is enabled, the number of the LED in the strip (zero-based)                                  |
| INFO2_LED_PIN_ACTIVE_ON         |   undef |       | values: LOW or HIGH, indicates at which GPIO state the LED is active (no function with SERIALLED)                                    |
| INFO2_LED_COLOR                 |  0,63,0 |       | set the color for the LED, default: 50% green - only for SERIALLED                                                                   |
| INFO3_LED_PIN                   |   undef |       | the GPIO to drive the LED, if SERIALLED is enabled, the number of the LED in the strip (zero-based)                                  |
| INFO3_LED_PIN_ACTIVE_ON         |   undef |       | values: LOW or HIGH, indicates at which GPIO state the LED is active (no function with SERIALLED)                                    |
| INFO3_LED_COLOR                 |  0,63,0 |       | set the color for the LED, default: 50% green - only for SERIALLED                                                                   |

## Custom Mode

By defining OPENKNX_LED_NO_AUTOCONF no LEDs are configured in the LED-Manager initialization.

Use `openknx.leds.addLed()` before `openknx.init()` to configure your LEDs.

# LED-Functions

OpenKNX Common provides a LED-FunctionManager accessible over `openknx.ledFunctions`