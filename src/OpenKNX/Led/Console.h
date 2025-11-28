#pragma once

#include "OpenKNX/Led/Manager.h"
#include "OpenKNX/Facade.h"
#include <string>
#include <vector>

namespace OpenKNX
{
    namespace Led
    {
        namespace Console
        {
            // Helper function to tokenize string
            inline std::vector<std::string> tokenize(const std::string& str)
            {
                std::vector<std::string> tokens;
                size_t pos = 0;
                while (pos < str.length())
                {
                    // Skip spaces
                    while (pos < str.length() && str[pos] == ' ')
                        pos++;
                    if (pos >= str.length())
                        break;

                    // Extract token
                    size_t tokenStart = pos;
                    while (pos < str.length() && str[pos] != ' ')
                        pos++;
                    tokens.push_back(str.substr(tokenStart, pos - tokenStart));
                }
                return tokens;
            }

            // Show help for LED commands
            inline void showHelp()
            {
                openknx.logger.begin();
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("================================= Help: LED Control =================================");
                openknx.logger.color(0);
                openknx.logger.log("Command(s)               Description");
                openknx.console.printHelpLine("leds <led> <action>", "Control LEDs on the device");
                openknx.console.printHelpLine("leds status", "Show all LED states");
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("Available LEDs:");
                openknx.logger.color(0);
                openknx.console.printHelpLine("  prog, p", "Programming LED");
                openknx.console.printHelpLine("  info1, 1", "Info LED 1");
                openknx.console.printHelpLine("  info2, 2", "Info LED 2");
                openknx.console.printHelpLine("  info3, 3", "Info LED 3");
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("Available Actions:");
                openknx.logger.color(0);
                openknx.console.printHelpLine("  on", "Turn LED on");
                openknx.console.printHelpLine("  off", "Turn LED off");
                openknx.console.printHelpLine("  dim <0-255>", "Set brightness (0=off, 255=full, I2C uses PWM)");
                openknx.console.printHelpLine("  pulse [ms]", "Pulsing effect (default: 1000ms)");
                openknx.console.printHelpLine("  blink [ms]", "Blinking effect (default: 500ms)");
                openknx.console.printHelpLine("  flash [ms]", "Flash LED (default: 100ms)");
                openknx.console.printHelpLine("  forceon", "Force LED on (ignore other states)");
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("Examples:");
                openknx.logger.color(0);
                openknx.console.printHelpLine("  leds status", "Show all LED states");
                openknx.console.printHelpLine("  leds prog pulse", "Make Programming LED pulse");
                openknx.console.printHelpLine("  leds 1 blink 200", "Make Info LED 1 blink every 200ms");
                openknx.console.printHelpLine("  leds 2 flash 500", "Flash Info LED 2 for 500ms");
                openknx.console.printHelpLine("  leds 3 dim 128", "Set Info LED 3 to 50% brightness");
                openknx.console.printHelpLine("  leds 3 on", "Turn Info LED 3 on");
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("---------------------------------------------------------------------------------");
                openknx.logger.color(0);
                openknx.logger.end();
            }

            // Show status of all LEDs
            inline void showStatus()
            {
                openknx.logger.begin();
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("================================== LED Status ===================================");
                openknx.logger.color(0);

                // Show all LEDs
                auto progLed = openknx.leds.getLed(Led::LED_TYPE_PROG);
                auto info1Led = openknx.leds.getLed(Led::LED_TYPE_INFO1);
                auto info2Led = openknx.leds.getLed(Led::LED_TYPE_INFO2);
                auto info3Led = openknx.leds.getLed(Led::LED_TYPE_INFO3);

                if (progLed)
                    openknx.logger.logWithPrefix("PROG LED", "Initialized");
                else
                    openknx.logger.logWithPrefix("PROG LED", "Not available");
                
                if (info1Led)
                    openknx.logger.logWithPrefix("INFO1 LED", "Initialized");
                else
                    openknx.logger.logWithPrefix("INFO1 LED", "Not available");
                
                if (info2Led)
                    openknx.logger.logWithPrefix("INFO2 LED", "Initialized");
                else
                    openknx.logger.logWithPrefix("INFO2 LED", "Not available");
                
                if (info3Led)
                    openknx.logger.logWithPrefix("INFO3 LED", "Initialized");
                else
                    openknx.logger.logWithPrefix("INFO3 LED", "Not available");

                openknx.logger.log("");
                openknx.logger.logDividingLine();
                openknx.logger.end();
            }

            // Map LED name to LED type
            inline bool getLedType(const std::string& name, Led::LedType& ledType)
            {
                if (name == "prog" || name == "p")
                    ledType = Led::LED_TYPE_PROG;
                else if (name == "info1" || name == "1")
                    ledType = Led::LED_TYPE_INFO1;
                else if (name == "info2" || name == "2")
                    ledType = Led::LED_TYPE_INFO2;
                else if (name == "info3" || name == "3")
                    ledType = Led::LED_TYPE_INFO3;
                else
                    return false;
                return true;
            }

            // Process LED command
            inline void processCommand(const std::string& cmd)
            {
                // Extract args after "leds" or "led"
                std::string args;
                if (cmd.length() > 5 && cmd.substr(0, 4) == "leds")
                    args = cmd.substr(5);
                else if (cmd.length() > 4 && cmd.substr(0, 3) == "led")
                    args = cmd.substr(4);
                else
                    args = "";

                // Trim
                auto trimStart = args.find_first_not_of(" ");
                if (trimStart != std::string::npos)
                {
                    auto trimEnd = args.find_last_not_of(" ");
                    args = args.substr(trimStart, trimEnd - trimStart + 1);
                }
                else
                    args = "";

                // Show help if empty or "?"
                if (args.empty() || args == "?")
                {
                    showHelp();
                    return;
                }

                // ------ STATUS ------
                if (args == "status")
                {
                    showStatus();
                    return;
                }

                // Split args into tokens
                std::vector<std::string> tokens = tokenize(args);

                // Validate token count
                if (tokens.size() < 2)
                {
                    openknx.logger.logWithPrefix("leds", "Error: Missing arguments");
                    openknx.logger.logWithPrefix("leds", "Usage: leds <led> <action> [duration]");
                    openknx.logger.logWithPrefix("leds", "Type 'leds ?' for help");
                    return;
                }

                std::string ledName = tokens[0];
                std::string action = tokens[1];
                uint16_t duration = (tokens.size() > 2) ? (uint16_t)std::atoi(tokens[2].c_str()) : 0;

                // Map LED name to type first
                Led::LedType ledType;
                if (!getLedType(ledName, ledType))
                {
                    const char* ledNameCStr = ledName.c_str();
                    openknx.logger.logWithPrefixAndValues("leds", "Error: Unknown LED '%s'", ledNameCStr);
                    openknx.logger.logWithPrefix("leds", "Valid LEDs: prog/p, info1/1, info2/2, info3/3");
                    openknx.logger.logWithPrefix("leds", "Type 'leds ?' for help");
                    return;
                }

                // Set default durations if not specified
                if (duration == 0)
                {
                    if (action == "pulse")
                        duration = 1000;
                    else if (action == "blink")
                        duration = 500;
                    else if (action == "flash")
                        duration = 100;
                }

                // Enforce minimum intervals for I2C LEDs (to prevent I2C bus overload)
                // I2C LEDs (INFO1, INFO2, INFO3) need longer intervals to prevent race conditions
                if (ledType == Led::LED_TYPE_INFO1 || ledType == Led::LED_TYPE_INFO2 || ledType == Led::LED_TYPE_INFO3)
                {
                    if (action == "blink" && duration < 10)
                    {
                        openknx.logger.logWithPrefixAndValues("leds", "Warning: I2C LED blink interval too fast (%dms), using minimum 10ms", (int)duration);
                        duration = 10;
                    }
                    else if (action == "pulse" && duration < 10)
                    {
                        openknx.logger.logWithPrefixAndValues("leds", "Warning: I2C LED pulse interval too fast (%dms), using minimum 10ms", (int)duration);
                        duration = 10;
                    }
                }

                // Get LED instance
                auto led = openknx.leds.getLed(ledType);
                if (!led)
                {
                    const char* ledNameCStr = ledName.c_str();
                    openknx.logger.logWithPrefixAndValues("leds", "Error: LED '%s' not initialized", ledNameCStr);
                    return;
                }

                // Execute action - save string pointers before LED operations
                const char* ledNameCStr = ledName.c_str();
                const char* actionCStr = action.c_str();
                
                if (action == "on")
                {
                    led->on();
                    openknx.logger.logWithPrefixAndValues("leds", "LED %s -> ON", ledNameCStr);
                }
                else if (action == "off")
                {
                    led->off();
                    openknx.logger.logWithPrefixAndValues("leds", "LED %s -> OFF", ledNameCStr);
                }
                else if (action == "dim" || action == "brightness")
                {
                    if (duration == 0)
                    {
                        openknx.logger.logWithPrefix("leds", "Error: dim requires brightness value (0-255)");
                        return;
                    }
                    if (duration > 255) duration = 255;
                    led->brightness(duration);
                    led->on(); // Activate LED with new brightness
                    const uint32_t updateFreqHz = 1000 / OPENKNX_INTERRUPT_TIMER_MS;
                    openknx.logger.logWithPrefixAndValues("leds", "LED %s -> BRIGHTNESS %d (%.1f%% @ %dHz PWM)", 
                        ledNameCStr, (int)duration, (duration * 100.0f / 255.0f), (int)updateFreqHz);
                }
                else if (action == "pulse")
                {
                    led->pulsing(duration);
                    openknx.logger.logWithPrefixAndValues("leds", "LED %s -> PULSING (%dms)", ledNameCStr, (int)duration);
                }
                else if (action == "blink")
                {
                    led->blinking(duration);
                    const uint32_t updateFreqHz = 1000 / OPENKNX_INTERRUPT_TIMER_MS;
                    const float blinkFreqHz = 1000.0f / (2.0f * duration); // Full cycle = 2x duration (on+off)
                    openknx.logger.logWithPrefixAndValues("leds", "LED %s -> BLINKING (%dms = %.1fHz @ %dHz)", ledNameCStr, (int)duration, blinkFreqHz, (int)updateFreqHz);
                }
                else if (action == "forceon")
                {
                    led->forceOn();
                    openknx.logger.logWithPrefixAndValues("leds", "LED %s -> FORCE ON", ledNameCStr);
                }
                else if (action == "flash")
                {
                    led->flash(duration);
                    openknx.logger.logWithPrefixAndValues("leds", "LED %s -> FLASH (%dms)", ledNameCStr, (int)duration);
                }
                else
                {
                    openknx.logger.logWithPrefixAndValues("leds", "Error: Unknown action '%s'", actionCStr);
                    openknx.logger.logWithPrefix("leds", "Valid actions: on, off, pulse, blink, flash, forceon");
                    openknx.logger.logWithPrefix("leds", "Type 'leds ?' for help");
                    return;
                }
            }

        } // namespace Console
    }     // namespace Led
} // namespace OpenKNX
