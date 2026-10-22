#pragma once

#include "OpenKNX/Facade.h"
#ifdef OPENKNX_I2C_SCHEDULER
    #include "OpenKNX/I2C/ScheduledWire.h"
#endif
#include "OpenKNX/Led/Manager.h"
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
                openknx.console.printHelpLine("leds status", "Type and brightness per LED");
#ifdef OPENKNX_CONSOLE_DEV
                openknx.console.printHelpLine("leds <led> <action>", "Control one LED (see below)");
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("LEDs <led>:");
                openknx.logger.color(0);
                openknx.console.printHelpLine("  prog, p", "Programming LED");
                openknx.console.printHelpLine("  info1, 1", "Info LED 1");
                openknx.console.printHelpLine("  info2, 2", "Info LED 2");
                openknx.console.printHelpLine("  info3, 3", "Info LED 3");
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("Actions <action>:");
                openknx.logger.color(0);
                openknx.console.printHelpLine("  on / off", "Turn LED on / off");
                openknx.console.printHelpLine("  dim <0-255>", "Maximum brightness, then on");
                openknx.console.printHelpLine("  pulse [ms]", "Pulsing effect (default 1000 ms)");
                openknx.console.printHelpLine("  blink [ms]", "Blinking effect (default 500 ms)");
                openknx.console.printHelpLine("  flash [n]", "Flash LED n times (default 1, max 255)");
                openknx.console.printHelpLine("  forceon", "Force LED on (ignore other states)");
                openknx.console.printHelpLine("  e.g. leds 2 blink 200", "Info LED 2 blinks every 200 ms");
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("PWM test: leds pwm <cmd> [leds] [value]; [leds] = p,1,2,3 or all (default all)");
                openknx.logger.color(0);
                openknx.console.printHelpLine("  status [leds]", "PWM config, counters and LED states");
                openknx.console.printHelpLine("  set [leds] <0-255>", "Fixed brightness; overrides the LED functions");
                openknx.console.printHelpLine("  demo [leds] [ms]", "Fade 0-100-0% repeatedly, ms per ramp (1000)");
                openknx.console.printHelpLine("  blink [leds] [ms]", "Blink on/off, ms per half (500)");
                openknx.console.printHelpLine("  stress [leds] [bus]", "Own rate per LED; bus: plus expander reads");
                openknx.console.printHelpLine("  stop [leds]", "End the test, LEDs follow their functions");
#ifdef OPENKNX_I2C_SCHEDULER
                openknx.console.printHelpLine("  reset", "Clear the I2C scheduler counters");
                openknx.console.printHelpLine("  e.g. stress all bus", "Then 'leds pwm status' after some minutes");
#else
                openknx.console.printHelpLine("  cycle", "Show current soft-PWM cycle");
                openknx.console.printHelpLine("  steps <2-100>", "Set soft-PWM steps (default 10)");
                openknx.console.printHelpLine("  reset", "Reset soft-PWM to 10 steps");
#endif
#endif // OPENKNX_CONSOLE_DEV
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("---------------------------------------------------------------------------------");
                openknx.logger.color(0);
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

            // ------ PWM test: leds pwm <sub> [leds] [value] ------

            inline const char* ledName(uint8_t type)
            {
                static const char* const names[] = {"prog", "info1", "info2", "info3"};
                return type <= Led::LED_TYPE_INFO3 ? names[type] : "?";
            }

            inline bool isNumber(const std::string& s)
            {
                if (s.empty())
                    return false;
                for (char c : s)
                    if (c < '0' || c > '9')
                        return false;
                return true;
            }

            // "p,1,2,3" or "all" -> LED types present on this device; false if the token is no LED list
            inline bool parseLedList(const std::string& token, std::vector<uint8_t>& types)
            {
                types.clear();
                if (token == "all")
                {
                    for (uint8_t t = Led::LED_TYPE_PROG; t <= Led::LED_TYPE_INFO3; t++)
                        if (openknx.leds.hasLed(t))
                            types.push_back(t);
                    return true;
                }
                size_t start = 0;
                while (start <= token.length())
                {
                    size_t end = token.find(',', start);
                    if (end == std::string::npos)
                        end = token.length();
                    Led::LedType type;
                    if (!getLedType(token.substr(start, end - start), type))
                        return false;
                    if (openknx.leds.hasLed(type))
                        types.push_back(type);
                    start = end + 1;
                }
                return true;
            }

            // One line per LED: kind, brightness, test mode. Inside an open logger block.
            inline void logLedRows(const std::vector<uint8_t>& types)
            {
#ifdef OPENKNX_CONSOLE_DEV
                static const char* const testNames[] = {"-", "level", "blink", "sweep"};
#endif
                for (uint8_t type : types)
                {
                    Led::Base* led = openknx.leds.getLed(type);
                    GPIO* gpio = led->asGPIO();
                    const char* kind = led->isColor() ? "RGB" : "LED";
                    if (gpio)
                    {
                        kind = gpio->isI2C() ? "I2C soft-PWM" : "GPIO";
#ifdef OPENKNX_I2C_SCHEDULER
                        if (gpio->isScheduled())
                            kind = "I2C scheduler";
#endif
                    }
#ifdef OPENKNX_CONSOLE_DEV
                    openknx.logger.logWithPrefixAndValues(ledName(type), "%-13s brightness %3d   test %s",
                                                          kind, (int)led->getCurrentBrightness(), testNames[(uint8_t)led->testMode()]);
#else
                    openknx.logger.logWithPrefixAndValues(ledName(type), "%-13s brightness %3d", kind, (int)led->getCurrentBrightness());
#endif
                }
            }

            inline void showLedStates(const std::vector<uint8_t>& types, const char* title)
            {
                openknx.logger.begin();
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.logWithValues("================================ %s ===================================", title);
                openknx.logger.color(0);
                logLedRows(types);
                openknx.logger.log("");
                openknx.logger.logDividingLine();
                openknx.logger.end();
            }

#ifdef OPENKNX_CONSOLE_DEV
            inline void showPwmStatus(const std::vector<uint8_t>& types)
            {
                openknx.logger.begin();
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("================================ PWM Status ===================================");
                openknx.logger.color(0);
#ifdef OPENKNX_I2C_SCHEDULER
                const uint32_t edgeUs = openknxScheduledWire.edgeUs();
                const uint32_t periodUs = OpenKNX::I2C::ScheduledWire::periodUs();
                const auto& st = openknxScheduledWire.stats();
                openknx.logger.logWithPrefixAndValues("Scheduler", "%d Hz, period %lu us, shortest pulse %lu us (%.1f%%)",
                                                      (int)OPENKNX_I2C_SCHEDULER_PWM_HZ, (unsigned long)periodUs, (unsigned long)edgeUs,
                                                      edgeUs * 100.0f / periodUs);
                openknx.logger.logWithPrefixAndValues("LED edges", "%lu, late >50us %lu (max %lu us), waited for bus %lu, errors %lu",
                                                      (unsigned long)st.events, (unsigned long)st.late, (unsigned long)st.maxLateUs,
                                                      (unsigned long)st.busyRetries, (unsigned long)st.edgeErrors);
                openknx.logger.logWithPrefixAndValues("Transfers", "%lu, max wait %lu us, forced %lu, errors %lu",
                                                      (unsigned long)st.transfers, (unsigned long)st.maxWaitUs,
                                                      (unsigned long)st.forced, (unsigned long)st.transferErrors);
                openknx.logger.logWithPrefixAndValues("Bus stress", "%s, reads %lu, errors %lu",
                                                      openknxScheduledWire.stressingBus() ? "on" : "off",
                                                      (unsigned long)st.stressReads, (unsigned long)st.stressErrors);
    #ifdef ARDUINO_ARCH_ESP32
                openknx.logger.logWithPrefixAndValues("Bus task", "stack never used %lu B", (unsigned long)openknxScheduledWire.busTaskStackFree());
    #endif
#else
                const uint8_t steps = Manager::getPwmSteps();
                const uint16_t updateHz = Manager::getTimerUpdateHz();
                openknx.logger.logWithPrefixAndValues("Soft-PWM", "cycle %d/%d, %d steps, %d Hz update, %.1f Hz effective",
                                                      (int)Manager::getPwmCycle(), (int)(steps - 1), (int)steps, (int)updateHz,
                                                      (float)updateHz / (float)steps);
#endif
                logLedRows(types);
                openknx.logger.log("");
                openknx.logger.logDividingLine();
                openknx.logger.end();
            }

#endif

            // leds status: one line per present LED
            inline void showStatus()
            {
                std::vector<uint8_t> types;
                parseLedList("all", types);
                showLedStates(types, "LED Status");
            }

#ifdef OPENKNX_CONSOLE_DEV
            inline void processPwm(const std::vector<std::string>& tokens)
            {
                const std::string sub = tokens.empty() ? "status" : tokens[0];
                std::vector<uint8_t> types;

                // first argument: LED list if it parses as one, otherwise all LEDs
                size_t next = 1;
                if (sub == "set" && tokens.size() < 3)
                    parseLedList("all", types); // "set <level>": the only argument is the level
                else if (tokens.size() > 1 && parseLedList(tokens[1], types))
                    next = 2;
                else
                    parseLedList("all", types);

                const bool hasValue = tokens.size() > next && isNumber(tokens[next]);
                uint32_t value = hasValue ? (uint32_t)std::atol(tokens[next].c_str()) : 0;

                if (sub == "status")
                {
                    showPwmStatus(types);
                    return;
                }

                Led::Base::TestMode mode = Led::Base::TestMode::Off;
                uint8_t level = 255;
                uint16_t periodMs = 0;
                if (sub == "set")
                {
                    if (!hasValue || value > 255)
                    {
                        openknx.logger.logWithPrefix("PWM", "Usage: leds pwm set [leds] <0-255>");
                        return;
                    }
                    mode = Led::Base::TestMode::Level;
                    level = (uint8_t)value;
                }
                else if (sub == "demo" || sub == "blink")
                {
                    mode = sub == "demo" ? Led::Base::TestMode::Sweep : Led::Base::TestMode::Blink;
                    if (!hasValue)
                        value = sub == "demo" ? 1000 : 500;
                    periodMs = (uint16_t)(value < 10 ? 10 : (value > 60000 ? 60000 : value));
                }
                else if (sub == "stress")
                {
                    // each LED its own pattern, so edges of all LEDs drift against each other
                    static const Led::Base::TestMode modes[] = {Led::Base::TestMode::Sweep, Led::Base::TestMode::Blink,
                                                                Led::Base::TestMode::Sweep, Led::Base::TestMode::Blink};
                    static const uint16_t periods[] = {700, 90, 1900, 330};
                    for (size_t i = 0; i < types.size(); i++)
                        openknx.leds.getLed(types[i])->test(modes[i % 4], 255, periods[i % 4]);
#ifdef OPENKNX_I2C_SCHEDULER
                    if (tokens.back() == "bus")
                        openknxScheduledWire.stressBus(true); // plus continuous expander reads from the loop
#endif
                    showPwmStatus(types);
                    return;
                }
                else if (sub == "stop")
                {
                    mode = Led::Base::TestMode::Off;
#ifdef OPENKNX_I2C_SCHEDULER
                    openknxScheduledWire.stressBus(false);
#endif
                }
#ifdef OPENKNX_I2C_SCHEDULER
                else if (sub == "reset")
                {
                    openknxScheduledWire.resetStats();
                    showPwmStatus(types);
                    return;
                }
#endif
#ifndef OPENKNX_I2C_SCHEDULER
                else if (sub == "cycle")
                {
                    openknx.logger.logWithPrefixAndValues("PWM", "Current Cycle: %d/%d", (int)Manager::getPwmCycle(), (int)(Manager::getPwmSteps() - 1));
                    return;
                }
                else if (sub == "steps")
                {
                    if (!hasValue || value < 2 || value > 100)
                    {
                        openknx.logger.logWithPrefixAndValues("PWM", "Current steps: %d (set: leds pwm steps <2-100>)", (int)Manager::getPwmSteps());
                        return;
                    }
                    Manager::setPwmSteps((uint8_t)value);
                    showPwmStatus(types);
                    return;
                }
                else if (sub == "reset")
                {
                    Manager::setPwmSteps(10);
                    showPwmStatus(types);
                    return;
                }
#endif
                else
                {
                    openknx.logger.logWithPrefix("PWM", "Usage: leds pwm [status|set|demo|blink|stress|stop] [leds] [value]");
                    return;
                }

                for (uint8_t type : types)
                    openknx.leds.getLed(type)->test(mode, level, periodMs);
                showPwmStatus(types);
            }

#endif

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

#ifndef OPENKNX_CONSOLE_DEV
                openknx.logger.logWithPrefix("leds", "Unknown command: only 'leds status' (more with OPENKNX_CONSOLE_DEV)");
#else
                // ------ PWM COMMANDS ------
                if (args.substr(0, 3) == "pwm")
                {
                    processPwm(tokenize(args.substr(3)));
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
                if (tokens.size() > 2 && !isNumber(tokens[2]))
                {
                    openknx.logger.logWithPrefix("leds", "Error: value must be a number");
                    return;
                }
                const long value = (tokens.size() > 2) ? std::atol(tokens[2].c_str()) : 0;
                uint16_t duration = value > 65535 ? 65535 : (uint16_t)value;

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
                        duration = 1; // flash count
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
                    if (tokens.size() < 3)
                    {
                        openknx.logger.logWithPrefix("leds", "Error: dim requires brightness value (0-255)");
                        return;
                    }
                    if (duration > 255) duration = 255;
                    led->brightness(duration);
                    led->on(); // Activate LED with new brightness
                    openknx.logger.logWithPrefixAndValues("leds", "LED %s -> BRIGHTNESS %d (%.1f%%)",
                        ledNameCStr, (int)duration, (duration * 100.0f / 255.0f));
                }
                else if (action == "pulse")
                {
                    led->pulsing(duration);
                    openknx.logger.logWithPrefixAndValues("leds", "LED %s -> PULSING (%dms)", ledNameCStr, (int)duration);
                }
                else if (action == "blink")
                {
                    led->blinking(duration);
                    const float blinkFreqHz = 1000.0f / (2.0f * duration); // Full cycle = 2x duration (on+off)
                    openknx.logger.logWithPrefixAndValues("leds", "LED %s -> BLINKING (%dms = %.1fHz)", ledNameCStr, (int)duration, blinkFreqHz);
                }
                else if (action == "forceon")
                {
                    led->forceOn();
                    openknx.logger.logWithPrefixAndValues("leds", "LED %s -> FORCE ON", ledNameCStr);
                }
                else if (action == "flash")
                {
                    const uint8_t count = duration > 255 ? 255 : (uint8_t)duration;
                    led->flash(count);
                    openknx.logger.logWithPrefixAndValues("leds", "LED %s -> FLASH (%d x)", ledNameCStr, (int)count);
                }
                else
                {
                    openknx.logger.logWithPrefixAndValues("leds", "Error: Unknown action '%s'", actionCStr);
                    openknx.logger.logWithPrefix("leds", "Valid actions: on, off, pulse, blink, flash, forceon");
                    openknx.logger.logWithPrefix("leds", "Type 'leds ?' for help");
                    return;
                }
#endif // OPENKNX_CONSOLE_DEV
            }

        } // namespace Console
    }     // namespace Led
} // namespace OpenKNX
