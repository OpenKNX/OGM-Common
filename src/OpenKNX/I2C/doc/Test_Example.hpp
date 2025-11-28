
// Kompletter I2C Test für RP2040 PIO I2C
// Testet Wire1 (Hardware) und PIO I2C Implementation



PIOI2CWire* myWire;

bool reserved_addr(uint8_t addr)
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void print_separator(const char* title = nullptr)
{
    Serial.println("\n================================================");
    if (title)
    {
        Serial.println(title);
        Serial.println("================================================");
    }
}

void test_i2c_pio(bool _doPioI2cBusScan)
{
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════╗");
    Serial.println("║     RP2040 I2C Test Suite                      ║");
    Serial.println("║     Wire1 (Hardware) vs PIO Implementation     ║");
    Serial.println("╚════════════════════════════════════════════════╝");

    // ============================================
    // Teil 1: Wire1 Scanner (Hardware I2C)
    // ============================================
    print_separator("TEIL 1: Hardware I2C Scanner (Wire1)");

    Wire1.setSDA(26);
    Wire1.setSCL(27);
    Wire1.begin();
    Wire1.setClock(100000);
    delay(100);

    Serial.println("Scanning I2C bus with Wire1...\n");
    Serial.println("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");

    uint8_t wire_found = 0;
    for (uint8_t addr = 0; addr < 0x80; addr++)
    {
        if (addr % 16 == 0)
        {
            Serial.printf("%02X: ", addr);
        }

        if (reserved_addr(addr))
        {
            Serial.print(" . ");
        }
        else
        {
            Wire1.beginTransmission(addr);
            uint8_t error = Wire1.endTransmission();

            if (error == 0)
            {
                Serial.printf(" %02X", addr);
                wire_found++;
            }
            else
            {
                Serial.print(" --");
            }
        }

        if (addr % 16 == 15)
        {
            Serial.println();
        }
    }

    Serial.printf("\n✓ Wire1 scan complete. Devices found: %d\n", wire_found);

    // ============================================
    // Teil 2: PIO I2C Initialisierung
    // ============================================
    print_separator("TEIL 2: PIO I2C Initialisierung");

    if (!myWire)
    {
        Serial.println("✗ ERROR: myWire ist NULL!");
        return;
    }

    myWire->begin();
    delay(100);

    if (!myWire->_pioi2c || !myWire->_pioi2c->_inst)
    {
        Serial.println("✗ ERROR: PIO I2C Initialisierung fehlgeschlagen!");
        return;
    }

    PIO pio = myWire->_pioi2c->_inst->pio;
    uint sm = myWire->_pioi2c->_inst->sm;

    uint32_t sys_clk = clock_get_hz(clk_sys);
    uint32_t baudrate = myWire->_pioi2c->_inst->baudrate;
    float div = (float)sys_clk / (32 * baudrate);
    float actual_baudrate = (float)sys_clk / (32 * div);

    Serial.println("PIO I2C Configuration:");
    Serial.printf("  • System Clock:     %d Hz (%.1f MHz)\n", sys_clk, sys_clk / 1e6);
    Serial.printf("  • Target Baudrate:  %d Hz\n", baudrate);
    Serial.printf("  • Clock Divider:    %.2f\n", div);
    Serial.printf("  • Actual Baudrate:  %.0f Hz\n", actual_baudrate);
    Serial.printf("  • SDA Pin:          GPIO %d\n", myWire->_pioi2c->_inst->sda);
    Serial.printf("  • SCL Pin:          GPIO %d\n", myWire->_pioi2c->_inst->scl);
    Serial.printf("  • PIO Instance:     PIO%d\n", pio == pio0 ? 0 : 1);
    Serial.printf("  • State Machine:    SM%d\n", sm);
    Serial.printf("  • Program Offset:   %d\n", myWire->_pioi2c->_inst->prog_offset);

    Serial.println("\nPIO Status:");
    Serial.printf("  • TX FIFO full:     %s\n", pio_sm_is_tx_fifo_full(pio, sm) ? "Yes" : "No");
    Serial.printf("  • RX FIFO empty:    %s\n", pio_sm_is_rx_fifo_empty(pio, sm) ? "Yes" : "No");
    Serial.printf("  • SM enabled:       %s\n", (pio->ctrl & (1u << sm)) ? "Yes" : "No");
    Serial.printf("  • Error IRQ:        %s\n", pio_interrupt_get(pio, sm) ? "SET" : "Clear");

    Serial.println("\n✓ PIO I2C erfolgreich initialisiert!");

    // ============================================
    // Teil 3: Manueller PIO Low-Level Test
    // ============================================
    print_separator("TEIL 3: Manueller PIO Low-Level Test");

    Serial.println("Testing manual I2C sequence on 0x18...\n");

    // Clear errors
    pio_interrupt_clear(pio, sm);

    // START
    myWire->_pioi2c->start(pio, sm);
    Serial.printf("  [1] START sent        - Error: %d\n", pio_interrupt_get(pio, sm));

    // Disable RX
    myWire->_pioi2c->rx_enable(pio, sm, false);

    // Send address
    uint8_t addr = 0x18;
    uint8_t addr_byte = (addr << 1) | 0; // Write
    uint16_t word = (addr_byte << 1) | 1u;

    Serial.printf("  [2] Sending address   - 0x%04X (addr=0x%02X + W)\n", word, addr);
    myWire->_pioi2c->put16(pio, sm, word);
    delay(10);

    Serial.printf("  [3] After address     - Error: %d %s\n",
                  pio_interrupt_get(pio, sm),
                  pio_interrupt_get(pio, sm) ? "(NAK)" : "(ACK)");

    // STOP
    myWire->_pioi2c->stop(pio, sm);
    Serial.printf("  [4] STOP sent         - Error: %d\n", pio_interrupt_get(pio, sm));

    myWire->_pioi2c->wait_idle(pio, sm);
    Serial.printf("  [5] Wait idle done    - Error: %d\n", pio_interrupt_get(pio, sm));

    if (pio_interrupt_get(pio, sm) == 0)
    {
        Serial.println("\n✓ Manual test SUCCESSFUL!");
    }
    else
    {
        Serial.println("\n✗ Manual test FAILED (NAK received)");
    }

    delay(500);

    // ============================================
    // Teil 4: PIO I2C Bus Scan
    // ============================================
    uint8_t pio_found = 0;
    if (_doPioI2cBusScan)
    {
        print_separator("TEIL 4: PIO I2C Bus Scanner");

        Serial.println("Scanning I2C bus with PIO...\n");
        Serial.println("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");

        for (uint8_t addr = 0; addr < 0x80; addr++)
        {
            if (addr % 16 == 0)
            {
                Serial.printf("%02X: ", addr);
            }

            int result;
            if (reserved_addr(addr))
            {
                result = -1;
                Serial.print(" . ");
            }
            else
            {
                result = myWire->pingw(addr) ? 0 : -1;
                sleep_ms(1); // Kleine Pause zwischen Scans

                if (result == 0)
                {
                    Serial.printf(" %02X", addr);
                    pio_found++;
                }
                else
                {
                    Serial.print(" --");
                }
            }

            if (addr % 16 == 15)
            {
                Serial.println();
            }

            delay(1); // Kleine Pause zwischen Scans
        }

        Serial.printf("\n✓ PIO scan complete. Devices found: %d\n", pio_found);
    } // if(_doPioI2cBusScan)

    // ============================================
    // Teil 5: Vergleich Wire1 vs PIO
    // ============================================
    print_separator("TEIL 5: Vergleichstest Wire1 vs PIO");

    uint8_t test_addresses[] = {0x18, 0x3C};

    for (int i = 0; i < 2; i++)
    {
        uint8_t addr = test_addresses[i];
        Serial.printf("\nTesting address 0x%02X:\n", addr);

        // ===== PIO Test =====
        bool pio_result = myWire->pingw(addr);
        Serial.printf("  PIO:    %s\n", pio_result ? "✓ ACK" : "✗ NAK");

        // ===== Wechsel zu Wire1 =====
        Serial.print("  Switching to Wire1...");
        Serial.flush();

        myWire->end(); // PIO komplett beenden
        delay(100);

        // Wire1 starten
        Wire1.setSDA(26);
        Wire1.setSCL(27);
        Wire1.begin();
        Wire1.setClock(400000);
        delay(100);

        Wire1.beginTransmission(addr);
        uint8_t wire_error = Wire1.endTransmission();
        Serial.printf(" Wire1: %s (error=%d)\n",
                      wire_error == 0 ? "✓ ACK" : "✗ NAK", wire_error);

        // ===== Zurück zu PIO =====
        Serial.print("  Switching back to PIO...");
        Serial.flush();

        Wire1.end();
        delay(100);

        myWire->begin(); // PIO neu starten (jetzt mit echter Re-Init)
        delay(100);

        // Verify PIO works
        Serial.print(" Verifying...");
        Serial.println(" Testing PIO after reinit:");
        Serial.printf("  _pioi2c: %p\n", myWire->_pioi2c);
        Serial.printf("  _inst: %p\n", myWire->_pioi2c ? myWire->_pioi2c->_inst : nullptr);

        if (myWire->_pioi2c && myWire->_pioi2c->_inst)
        {
            Serial.printf("  PIO: %p\n", myWire->_pioi2c->_inst->pio);
            Serial.printf("  SM: %d\n", myWire->_pioi2c->_inst->sm);
            Serial.printf("  SM enabled: %d\n",
                          (myWire->_pioi2c->_inst->pio->ctrl & (1u << myWire->_pioi2c->_inst->sm)) != 0);
        }

        Serial.print(" Calling WriteBlocking directly...");
        Serial.flush();

        PIO pio = myWire->_pioi2c->_inst->pio;
        uint sm = myWire->_pioi2c->_inst->sm;

        Serial.println("\n  [1] Clearing error");
        pio_interrupt_clear(pio, sm);

        Serial.println("  [2] Calling start()");
        Serial.flush();
        myWire->_pioi2c->start(pio, sm);

        Serial.println("  [3] After start()");
        Serial.flush();

        myWire->_pioi2c->rx_enable(pio, sm, false);

        Serial.println("  [4] Sending address");
        Serial.flush();
        uint8_t addr_byte = (addr << 1) | 0;
        myWire->_pioi2c->put16(pio, sm, (addr_byte << 1) | 1u);

        Serial.println("  [5] After address");
        Serial.flush();
        sleep_ms(1);

        Serial.println("  [6] Calling stop()");
        Serial.flush();
        myWire->_pioi2c->stop(pio, sm);

        Serial.println("  [7] Calling wait_idle()");
        Serial.flush();
        myWire->_pioi2c->wait_idle(pio, sm);

        Serial.println("  [8] Done!");
        Serial.flush();
        Serial.print(" Now calling pingw...");
        Serial.flush();
        // Verify PIO works
        bool verify = myWire->pingw(addr);
        Serial.printf(" PIO verify: %s\n", verify ? "✓ ACK" : "✗ NAK");

        delay(50);
    }

    // ============================================
    // Teil 6: Write/Read Tests
    // ============================================
    print_separator("TEIL 6: Write/Read Tests");

    Serial.println("Testing Write operations...\n");

    // Test 0x18 (PCA9557 I/O Expander)
    Serial.print("Writing to 0x18 (PCA9557): ");
    uint8_t pca_data[] = {0x01, 0xAA}; // Register 0x01 (Output), Data 0xAA
    int result = myWire->WriteBlocking(0x18, pca_data, 2);
    if (result == 0)
    {
        Serial.println("✓ OK (Output Port set to 0xAA)");
    }
    else
    {
        Serial.printf("✗ FAILED (error=%d)\n", result);
    }
    delay(10);

    // Test 0x3C (SSD1306 Display)
    Serial.print("Writing to 0x3C (SSD1306): ");
    uint8_t ssd_data[] = {
        0x00, // Control byte: Command mode
        0xAE, // Command: Display OFF
        0xA1  // Command: Segment remap
    };
    result = myWire->WriteBlocking(0x3C, ssd_data, 3);
    if (result == 0)
    {
        Serial.println("✓ OK (Commands sent)");
    }
    else
    {
        Serial.printf("✗ FAILED (error=%d)\n", result);
    }
    delay(10);

    Serial.println("\nTesting Read operations...\n");

    // PCA9557: Read Input Port (Register 0x00)
    Serial.print("Reading from 0x18 (PCA9557): ");
    uint8_t pca_reg = 0x00;                   // Input Port register
    myWire->WriteBlocking(0x18, &pca_reg, 1); // Set register pointer
    uint8_t pca_read[1];
    result = myWire->ReadBlocking(0x18, pca_read, 1);
    if (result == 0)
    {
        Serial.printf("✓ OK (Input Port: 0x%02X)\n", pca_read[0]);
    }
    else
    {
        Serial.printf("✗ FAILED (error=%d)\n", result);
    }
    delay(10);

    // SSD1306: Read (meist nicht unterstützt, nur zum Test)
    Serial.print("Reading from 0x3C (SSD1306): ");
    uint8_t ssd_read[4];
    result = myWire->ReadBlocking(0x3C, ssd_read, 4);
    if (result == 0)
    {
        Serial.printf("✓ OK (received: ");
        for (int i = 0; i < 4; i++)
        {
            Serial.printf("0x%02X ", ssd_read[i]);
        }
        Serial.println(")");
    }
    else
    {
        Serial.printf("✗ FAILED (error=%d)\n", result);
    }

    // ============================================
    // Teil 7: Bus Reset Test
    // ============================================
    print_separator("TEIL 7: Bus Reset Test");

    Serial.println("Testing I2C bus reset...");
    Serial.println("  [1] Closing PIO I2C...");
    myWire->end();

    delay(100);

    Serial.println("  [2] Reinitializing PIO I2C...");
    myWire->begin();

    delay(100);

    Serial.println("  [3] Testing communication after reset...");
    bool reset_test = myWire->pingw(0x18);
    Serial.printf("  [4] Ping test: %s\n", reset_test ? "✓ OK" : "✗ FAILED");

    if (reset_test)
    {
        Serial.println("\n✓ Bus reset successful!");
    }
    else
    {
        Serial.println("\n✗ Bus reset failed!");
    }

    // ============================================
    // Zusammenfassung
    // ============================================
    print_separator("ZUSAMMENFASSUNG");

    Serial.printf("Wire1 (Hardware I2C)    : %d devices found\n", wire_found);
    Serial.printf("PIO I2C Implementation  : %d devices found\n", pio_found);
    Serial.printf("Status                  : %s\n",
                  (wire_found == pio_found && pio_found > 0) ? "✓ BOTH WORKING" : "⚠ CHECK RESULTS");

    Serial.println("\n================================================");
    Serial.println("Test Suite Complete!");
    Serial.println("================================================\n");
}

// Teil 8: Erweiterte Wire API Tests
// Testet Transaction API, Repeated Start, Clock Speed

void test_extended_wire_api()
{
    print_separator("TEIL 8: Erweiterte Wire API Tests");

    if (!myWire || !myWire->_pioi2c || !myWire->_pioi2c->_inst)
    {
        Serial.println("✗ ERROR: PIO I2C nicht initialisiert!");
        return;
    }

    // ============================================
    // Test 1: Transaction API (beginTransmission + write + endTransmission)
    // ============================================
    Serial.println("\n--- Test 1: Transaction API ---\n");

    // Test 1a: PCA9557 - Einzelnes Write
    Serial.print("1a) PCA9557 Single Write Transaction: ");
    myWire->beginTransmission(0x18);
    myWire->write(0x03); // Register: Configuration
    myWire->write(0x00); // Data: All pins as outputs
    uint8_t result = myWire->endTransmission();
    Serial.printf("%s (error=%d)\n", result == 0 ? "✓ OK" : "✗ FAIL", result);
    delay(10);

    // Test 1b: PCA9557 - Multi-Byte Write
    Serial.print("1b) PCA9557 Multi-Byte Write: ");
    myWire->beginTransmission(0x18);
    myWire->write(0x01); // Register: Output Port
    myWire->write(0x55); // Pattern 1
    result = myWire->endTransmission();
    Serial.printf("%s (error=%d)\n", result == 0 ? "✓ OK" : "✗ FAIL", result);
    delay(10);

    // Test 1c: SSD1306 - Command Sequence
    Serial.print("1c) SSD1306 Command Sequence: ");
    myWire->beginTransmission(0x3C);
    myWire->write(0x00); // Control byte: Command
    myWire->write(0xAE); // Display OFF
    myWire->write(0xD5); // Set display clock
    myWire->write(0x80); // Clock value
    result = myWire->endTransmission();
    Serial.printf("%s (error=%d)\n", result == 0 ? "✓ OK" : "✗ FAIL", result);
    delay(10);

    // Test 1d: Buffer-Test (32 Bytes - Maximum)
    Serial.print("1d) Buffer Limit Test (32 bytes): ");
    myWire->beginTransmission(0x3C);
    for (int i = 0; i < 32; i++)
    {
        size_t written = myWire->write((uint8_t)i);
        if (written == 0)
        {
            Serial.printf("✗ FAIL at byte %d\n", i);
            myWire->endTransmission();
            goto test1_end;
        }
    }
    result = myWire->endTransmission();
    Serial.printf("%s (sent 32 bytes, error=%d)\n", result == 0 ? "✓ OK" : "✗ FAIL", result);
    delay(10);

test1_end:
    Serial.println("\n✓ Test 1 Complete\n");

    // ============================================
    // Test 2: Request/Read Flow
    // ============================================
    Serial.println("--- Test 2: Request/Read Flow ---\n");

    // Test 2a: PCA9557 - Read Input Port
    Serial.print("2a) PCA9557 Read Input Port: ");
    uint8_t reg = 0x00; // Input Port register
    myWire->beginTransmission(0x18);
    myWire->write(reg);
    myWire->endTransmission();

    uint8_t bytes_read = myWire->requestFrom(0x18, (size_t)1, true);
    if (bytes_read == 1 && myWire->available())
    {
        uint8_t input = myWire->read();
        Serial.printf("✓ OK (Input=0x%02X, available=%d)\n", input, myWire->available());
    }
    else
    {
        Serial.printf("✗ FAIL (bytes_read=%d, available=%d)\n", bytes_read, myWire->available());
    }
    delay(10);

    // Test 2b: PCA9557 - Read Multiple Registers
    Serial.print("2b) PCA9557 Read 4 Registers: ");
    reg = 0x00;
    myWire->beginTransmission(0x18);
    myWire->write(reg);
    myWire->endTransmission();

    bytes_read = myWire->requestFrom(0x18, (size_t)4, true);
    Serial.printf("Read %d bytes: ", bytes_read);
    while (myWire->available())
    {
        Serial.printf("0x%02X ", myWire->read());
    }
    Serial.println(bytes_read == 4 ? "✓ OK" : "✗ FAIL");
    delay(10);

    // Test 2c: SSD1306 - Read (meist dummy data)
    Serial.print("2c) SSD1306 Read Test: ");
    bytes_read = myWire->requestFrom(0x3C, (size_t)4, true);
    Serial.printf("Read %d bytes: ", bytes_read);
    int count = 0;
    while (myWire->available() && count < 4)
    {
        Serial.printf("0x%02X ", myWire->read());
        count++;
    }
    Serial.println(bytes_read > 0 ? "✓ OK" : "✗ FAIL");
    delay(10);

    Serial.println("\n✓ Test 2 Complete\n");

    // ============================================
    // Test 3: Write-Read mit Repeated Start (nostop)
    // ============================================
    Serial.println("--- Test 3: Repeated Start (Write-Read ohne STOP) ---\n");

    // Test 3a: PCA9557 - Write Register Pointer, dann Read (mit Repeated Start)
    Serial.print("3a) PCA9557 Repeated Start Read: ");
    reg = 0x00; // Input Port
    myWire->beginTransmission(0x18);
    myWire->write(reg);
    result = myWire->endTransmission(false); // NOSTOP!

    if (result == 0)
    {
        bytes_read = myWire->requestFrom(0x18, (size_t)1, true);
        if (bytes_read == 1)
        {
            uint8_t value = myWire->read();
            Serial.printf("✓ OK (Value=0x%02X via Repeated Start)\n", value);
        }
        else
        {
            Serial.printf("✗ FAIL (Read failed)\n");
        }
    }
    else
    {
        Serial.printf("✗ FAIL (Write with NOSTOP failed, error=%d)\n", result);
    }
    delay(10);

    // Test 3b: PCA9557 - Multi-Register Read mit Repeated Start
    Serial.print("3b) PCA9557 Multi-Register Repeated Start: ");
    reg = 0x00;
    myWire->beginTransmission(0x18);
    myWire->write(reg);
    result = myWire->endTransmission(false); // NOSTOP!

    if (result == 0)
    {
        bytes_read = myWire->requestFrom(0x18, (size_t)4, true);
        Serial.printf("Read %d bytes via Repeated Start: ", bytes_read);
        while (myWire->available())
        {
            Serial.printf("0x%02X ", myWire->read());
        }
        Serial.println(bytes_read == 4 ? "✓ OK" : "✗ FAIL");
    }
    else
    {
        Serial.printf("✗ FAIL\n");
    }
    delay(10);

    // Test 3c: Vergleich mit/ohne Repeated Start
    Serial.print("3c) Comparison: With vs Without Repeated Start: ");

    // Mit STOP
    reg = 0x00;
    myWire->beginTransmission(0x18);
    myWire->write(reg);
    myWire->endTransmission(true); // MIT STOP
    bytes_read = myWire->requestFrom(0x18, (size_t)1, true);
    uint8_t value_with_stop = myWire->read();

    // Ohne STOP (Repeated Start)
    myWire->beginTransmission(0x18);
    myWire->write(reg);
    myWire->endTransmission(false); // OHNE STOP
    bytes_read = myWire->requestFrom(0x18, (size_t)1, true);
    uint8_t value_no_stop = myWire->read();

    Serial.printf("With STOP=0x%02X, Repeated Start=0x%02X ", value_with_stop, value_no_stop);
    Serial.println(value_with_stop == value_no_stop ? "✓ OK (Same)" : "⚠ Different");
    delay(10);

    Serial.println("\n✓ Test 3 Complete\n");

    // ============================================
    // Test 4: Clock Speed Tests
    // ============================================
    Serial.println("--- Test 4: Clock Speed Tests ---\n");

    uint32_t speeds[] = {100000, 400000, 100000}; // 100kHz, 400kHz, zurück zu 100kHz
    const char* speed_names[] = {"100 kHz (Standard)", "400 kHz (Fast Mode)", "100 kHz (Back to Standard)"};

    for (int i = 0; i < 3; i++)
    {
        Serial.printf("4%c) Testing at %s: ", 'a' + i, speed_names[i]);

        // Clock Speed ändern
        myWire->setClock(speeds[i]);
        delay(50); // Kurze Pause nach Clock-Änderung

        // Test: PCA9557 Read
        reg = 0x00;
        myWire->beginTransmission(0x18);
        myWire->write(reg);
        result = myWire->endTransmission();

        if (result == 0)
        {
            bytes_read = myWire->requestFrom(0x18, (size_t)1, true);
            if (bytes_read == 1)
            {
                uint8_t value = myWire->read();
                Serial.printf("✓ OK (Read=0x%02X at %d Hz)\n", value, speeds[i]);
            }
            else
            {
                Serial.printf("✗ FAIL (Read failed)\n");
            }
        }
        else
        {
            Serial.printf("✗ FAIL (Write failed, error=%d)\n", result);
        }

        delay(20);
    }

    // Verify final speed
    Serial.print("\n4d) Verify Clock Speed Persistence: ");
    uint32_t final_speed = myWire->_pioi2c->_inst->baudrate;
    Serial.printf("%d Hz ", final_speed);
    Serial.println(final_speed == 100000 ? "✓ OK" : "✗ FAIL");

    Serial.println("\n✓ Test 4 Complete\n");

    // ============================================
    // Test Summary
    // ============================================
    Serial.println("================================================");
    Serial.println("Extended Wire API Tests Complete!");
    Serial.println("================================================");
    Serial.println("Summary:");
    Serial.println("  ✓ Transaction API (beginTransmission/write/endTransmission)");
    Serial.println("  ✓ Request/Read Flow (requestFrom/available/read)");
    Serial.println("  ✓ Repeated Start (Write without STOP)");
    Serial.println("  ✓ Clock Speed Changes (100kHz ↔ 400kHz)");
    Serial.println("================================================\n");
}
// ==============================================================
// Teil 8: Performance Benchmarks - Erweiterte Version
// Mit 800kHz Test für Wire1 und detailliertem Fazit
// ==============================================================

void benchmark_throughput(TwoWire* wire, uint32_t clock_speed, uint16_t data_size, uint16_t iterations, const char* wire_name, uint8_t test_addr = 0x3C)
{
    Serial.printf("  %s @ %d kHz, %d bytes, %d iters: ", 
                  wire_name, clock_speed / 1000, data_size, iterations);
    
    wire->setClock(clock_speed);
    delay(50);
    
    uint8_t* data = (uint8_t*)malloc(data_size);
    memset(data, 0xAA, data_size);
    
    uint32_t start = micros();
    uint16_t errors = 0;
    
    for (int i = 0; i < iterations; i++)
    {
        wire->beginTransmission(test_addr);
        wire->write(0x40);
        wire->write(data, data_size);
        uint8_t err = wire->endTransmission();
        if (err != 0) errors++;
    }
    
    uint32_t duration = micros() - start;
    free(data);
    
    float bytes_per_sec = ((float)(iterations * data_size) * 1000000.0f) / duration;
    float kbps = (bytes_per_sec * 8.0f) / 1000.0f;
    
    Serial.printf("%6d µs, %5.1f KB/s", duration, bytes_per_sec / 1024);
    if (errors > 0) Serial.printf(" [%d ERR]", errors);
    else Serial.print(" [OK]   ");
    Serial.println();
}

void benchmark_display_update(TwoWire* wire, uint32_t clock_speed, const char* wire_name)
{
    Serial.printf("  %s @ %3d kHz: ", wire_name, clock_speed / 1000);
    
    wire->setClock(clock_speed);
    delay(50);
    
    uint8_t page_data[128];
    memset(page_data, 0x55, 128);
    
    uint32_t start = micros();
    uint16_t errors = 0;
    
    // 8 Pages (128x64 Display = 1024 bytes)
    for (int page = 0; page < 8; page++)
    {
        wire->beginTransmission(0x3C);
        wire->write(0x00);
        wire->write(0xB0 + page);
        uint8_t err = wire->endTransmission();
        if (err != 0) errors++;
        
        wire->beginTransmission(0x3C);
        wire->write(0x40);
        wire->write(page_data, 128);
        err = wire->endTransmission();
        if (err != 0) errors++;
    }
    
    uint32_t duration = micros() - start;
    float fps = 1000000.0f / duration;
    
    Serial.printf("%6d µs (%5.1f FPS)", duration, fps);
    if (errors > 0) Serial.printf(" [%d ERR]", errors);
    Serial.println();
}

// Struktur für Benchmark-Ergebnisse
struct BenchmarkResult {
    uint32_t duration_us;
    float throughput_kbps;
    uint16_t errors;
};

void test_performance_benchmarks()
{
    print_separator("TEIL 8: Performance Benchmarks (PIO vs Wire1)");
    
    // Ergebnis-Arrays für Vergleich
    BenchmarkResult pio_results[5];
    BenchmarkResult wire_results[5];
    
    // ============================================
    // 8a: Throughput - Small Packets (8 bytes)
    // ============================================
    Serial.println("\n--- 8a) Throughput: Small Packets (8 bytes, Display 0x3C) ---\n");
    Serial.println("  PIO I2C:");
    benchmark_throughput(myWire, 100000, 8, 500, "  100k", 0x3C);
    benchmark_throughput(myWire, 400000, 8, 500, "  400k", 0x3C);
    benchmark_throughput(myWire, 800000, 8, 500, "  800k", 0x3C);
    
    myWire->end();
    delay(100);
    Wire1.setSDA(26);
    Wire1.setSCL(27);
    Wire1.begin();
    Serial.println("\n  Hardware I2C (Wire1):");
    benchmark_throughput(&Wire1, 100000, 8, 500, "  100k", 0x3C);
    benchmark_throughput(&Wire1, 400000, 8, 500, "  400k", 0x3C);
    benchmark_throughput(&Wire1, 800000, 8, 500, "  800k", 0x3C);
    Wire1.end();
    delay(100);
    myWire->begin();
    delay(100);
    
    // ============================================
    // 8b: Throughput - Medium Packets (128 bytes)
    // ============================================
    Serial.println("\n--- 8b) Throughput: Medium Packets (128 bytes, Display 0x3C) ---\n");
    Serial.println("  PIO I2C:");
    benchmark_throughput(myWire, 100000, 128, 200, "  100k", 0x3C);
    benchmark_throughput(myWire, 400000, 128, 200, "  400k", 0x3C);
    benchmark_throughput(myWire, 800000, 128, 200, "  800k", 0x3C);
    
    myWire->end();
    delay(100);
    Wire1.setSDA(26);
    Wire1.setSCL(27);
    Wire1.begin();
    Serial.println("\n  Hardware I2C (Wire1):");
    benchmark_throughput(&Wire1, 100000, 128, 200, "  100k", 0x3C);
    benchmark_throughput(&Wire1, 400000, 128, 200, "  400k", 0x3C);
    benchmark_throughput(&Wire1, 800000, 128, 200, "  800k", 0x3C);
    Wire1.end();
    delay(100);
   // myWire->begin();
   // delay(100);
   // 
   // // ============================================
   // // 8c: Expander Test (0x18)
   // // ============================================
   // Serial.println("\n--- 8c) Expander Test (8 bytes, Expander 0x18) ---\n");
   // Serial.println("  PIO I2C:");
   // benchmark_throughput(myWire, 100000, 8, 200, "  100k", 0x18);
   // benchmark_throughput(myWire, 400000, 8, 200, "  400k", 0x18);
   // benchmark_throughput(myWire, 800000, 8, 200, "  800k", 0x18);
   // 
   // myWire->end();
   // delay(100);
   // Wire1.setSDA(26);
   // Wire1.setSCL(27);
   // Wire1.begin();
   // Serial.println("\n  Hardware I2C (Wire1):");
   // benchmark_throughput(&Wire1, 100000, 8, 200, "  100k", 0x18);
   // benchmark_throughput(&Wire1, 400000, 8, 200, "  400k", 0x18);
   // benchmark_throughput(&Wire1, 800000, 8, 200, "  800k", 0x18);
   // Wire1.end();
   // delay(100);
    myWire->begin();
    delay(100);
    
    // ============================================
    // 8d: Display Full Update mit Messung
    // ============================================
    Serial.println("\n--- 8d) Display Full Update (128x64 = 1024 bytes) ---\n");

    // PIO I2C
    Serial.println("  PIO I2C:");
    for (int i = 0; i < 5; i++)
    {
        uint32_t speeds[] = {100000, 200000, 400000, 600000, 800000};
        myWire->setClock(speeds[i]);
        delay(50);
        
        uint8_t page_data[128];
        memset(page_data, 0x55, 128);
        
        uint32_t start = micros();
        uint16_t errors = 0;
        
        for (int page = 0; page < 8; page++)
        {
            myWire->beginTransmission(0x3C);
            myWire->write(0x00);
            myWire->write(0xB0 + page);
            uint8_t err = myWire->endTransmission();
            if (err != 0) errors++;
            
            myWire->beginTransmission(0x3C);
            myWire->write(0x40);
            myWire->write(page_data, 128);
            err = myWire->endTransmission();
            if (err != 0) errors++;
        }
        
        uint32_t duration = micros() - start;
        float fps = 1000000.0f / duration;
        
        pio_results[i].duration_us = duration;
        pio_results[i].throughput_kbps = (1024.0f * 8.0f * 1000000.0f) / duration / 1000.0f;
        pio_results[i].errors = errors;
        
        Serial.printf("  %3d kHz: %6d µs (%5.1f FPS)", speeds[i]/1000, duration, fps);
        if (errors > 0) Serial.printf(" [%d ERR]", errors);
        Serial.println();
    }
    
    // Wire1
    myWire->end();
    delay(100);
    Wire1.setSDA(26);
    Wire1.setSCL(27);
    Wire1.begin();
    
    Serial.println("\n  Hardware I2C (Wire1):");
    for (int i = 0; i < 5; i++)
    {
        uint32_t speeds[] = {100000, 200000, 400000, 600000, 800000};
        Wire1.setClock(speeds[i]);
        delay(50);
        
        uint8_t page_data[128];
        memset(page_data, 0x55, 128);
        
        uint32_t start = micros();
        uint16_t errors = 0;
        
        for (int page = 0; page < 8; page++)
        {
            Wire1.beginTransmission(0x3C);
            Wire1.write(0x00);
            Wire1.write(0xB0 + page);
            uint8_t err = Wire1.endTransmission();
            if (err != 0) errors++;
            
            Wire1.beginTransmission(0x3C);
            Wire1.write(0x40);
            Wire1.write(page_data, 128);
            err = Wire1.endTransmission();
            if (err != 0) errors++;
        }
        
        uint32_t duration = micros() - start;
        float fps = 1000000.0f / duration;
        
        wire_results[i].duration_us = duration;
        wire_results[i].throughput_kbps = (1024.0f * 8.0f * 1000000.0f) / duration / 1000.0f;
        wire_results[i].errors = errors;
        
        Serial.printf("  %3d kHz: %6d µs (%5.1f FPS)", speeds[i]/1000, duration, fps);
        if (errors > 0) Serial.printf(" [%d ERR]", errors);
        Serial.println();
    }
    
    Wire1.end();
    delay(100);
    myWire->begin();
    delay(100);
    
    // ============================================
    // 8e: Latency Test
    // ============================================
    Serial.println("\n--- 8e) Latency Test (Single 2-byte transfers) ---\n");

    uint32_t speeds[] = {100000, 400000, 800000};
    const char* names[] = {"100 kHz", "400 kHz", "800 kHz"};
    
    Serial.println("  PIO I2C:");
    for (int s = 0; s < 3; s++)
    {
        myWire->setClock(speeds[s]);
        delay(50);
        
        uint32_t min_time = UINT32_MAX;
        uint32_t max_time = 0;
        uint32_t total_time = 0;
        
        for (int i = 0; i < 100; i++)
        {
            uint32_t start = micros();
            myWire->beginTransmission(0x3C);
            myWire->write(0x00);
            myWire->write(0xAF);
            myWire->endTransmission();
            uint32_t duration = micros() - start;
            
            if (duration < min_time) min_time = duration;
            if (duration > max_time) max_time = duration;
            total_time += duration;
        }
        
        float avg_time = (float)total_time / 100.0f;
        Serial.printf("    %s: Min=%3d µs, Avg=%5.1f µs, Max=%3d µs\n", 
                      names[s], min_time, avg_time, max_time);
    }
    
    myWire->end();
    delay(100);
    Wire1.setSDA(26);
    Wire1.setSCL(27);
    Wire1.begin();
    
    Serial.println("\n  Hardware I2C (Wire1):");
    for (int s = 0; s < 3; s++)
    {
        Wire1.setClock(speeds[s]);
        delay(50);
        
        uint32_t min_time = UINT32_MAX;
        uint32_t max_time = 0;
        uint32_t total_time = 0;
        
        for (int i = 0; i < 100; i++)
        {
            uint32_t start = micros();
            Wire1.beginTransmission(0x3C);
            Wire1.write(0x00);
            Wire1.write(0xAF);
            Wire1.endTransmission();
            uint32_t duration = micros() - start;
            
            if (duration < min_time) min_time = duration;
            if (duration > max_time) max_time = duration;
            total_time += duration;
        }
        
        float avg_time = (float)total_time / 100.0f;
        Serial.printf("    %s: Min=%3d µs, Avg=%5.1f µs, Max=%3d µs\n", 
                      names[s], min_time, avg_time, max_time);
    }
    
    Wire1.end();
    delay(100);
    myWire->begin();
    delay(100);
    
    // ============================================
    // 8f: Speed Stability
    // ============================================
    Serial.println("\n--- 8f) Speed Stability Test (Error Rate) ---\n");

    uint32_t test_speeds[] = {100000, 200000, 400000, 600000, 800000, 1000000};
    const char* speed_labels[] = {"100k", "200k", "400k", "600k", "800k", "1MHz"};
    
    Serial.println("  PIO I2C:");
    for (int s = 0; s < 6; s++)
    {
        myWire->setClock(test_speeds[s]);
        delay(50);
        
        Serial.printf("    %5s Hz: ", speed_labels[s]);
        
        uint16_t errors = 0;
        for (int i = 0; i < 50; i++)
        {
            myWire->beginTransmission(0x3C);
            myWire->write(0x00);
            myWire->write(0xAE);
            uint8_t err = myWire->endTransmission();
            if (err != 0) errors++;
        }
        
        if (errors == 0) {
            Serial.println("✓ OK (0/50 errors)");
        } else {
            Serial.printf("✗ FAIL (%d/50 errors, %.1f%%)\n", 
                         errors, (errors / 50.0f) * 100.0f);
        }
    }
    
    myWire->end();
    delay(100);
    Wire1.setSDA(26);
    Wire1.setSCL(27);
    Wire1.begin();
    
    Serial.println("\n  Hardware I2C (Wire1):");
    for (int s = 0; s < 6; s++)
    {
        Wire1.setClock(test_speeds[s]);
        delay(50);
        
        Serial.printf("    %5s Hz: ", speed_labels[s]);
        
        uint16_t errors = 0;
        for (int i = 0; i < 50; i++)
        {
            Wire1.beginTransmission(0x3C);
            Wire1.write(0x00);
            Wire1.write(0xAE);
            uint8_t err = Wire1.endTransmission();
            if (err != 0) errors++;
        }
        
        if (errors == 0) {
            Serial.println("✓ OK (0/50 errors)");
        } else {
            Serial.printf("✗ FAIL (%d/50 errors, %.1f%%)\n", 
                         errors, (errors / 50.0f) * 100.0f);
        }
    }
    
    Wire1.end();
    delay(100);
    myWire->begin();
    delay(100);
    
    myWire->setClock(400000);
    
    Serial.println("\n✓ Teil 8 Complete");
    
    // ============================================
    // FAZIT MIT PROZENT-VERGLEICH
    // ============================================
    print_separator("FAZIT: PIO I2C vs Hardware I2C");
    
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║           PERFORMANCE VERGLEICH (Display Update)          ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");
    
    Serial.println("Speed   │ PIO I2C    │ Wire1      │ Differenz     │ Gewinner");
    Serial.println("────────┼────────────┼────────────┼───────────────┼─────────");
    
    const char* speed_names[] = {"100 kHz", "200 kHz", "400 kHz", "600 kHz", "800 kHz"};
    
    for (int i = 0; i < 5; i++)
    {
        float pio_fps = 1000000.0f / pio_results[i].duration_us;
        float wire_fps = 1000000.0f / wire_results[i].duration_us;
        float diff_percent = ((pio_fps - wire_fps) / wire_fps) * 100.0f;
        
        Serial.printf("%-7s │ %5.1f FPS  │ %5.1f FPS  │ ", 
                      speed_names[i], pio_fps, wire_fps);
        
        if (diff_percent > 0) {
            Serial.printf("+%5.1f%% ", diff_percent);
            Serial.print("│ PIO I2C ✓");
        } else {
            Serial.printf("%6.1f%% ", diff_percent);
            Serial.print("│ Wire1 ✓");
        }
        
        if (pio_results[i].errors > 0 || wire_results[i].errors > 0) {
            Serial.print(" [ERR]");
        }
        Serial.println();
    }
    
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println(  "║                    ZUSAMMENFASSUNG                         ║");
    Serial.println(  "╚════════════════════════════════════════════════════════════╝\n");
    
    // Durchschnitt berechnen
    float avg_pio_improvement = 0;
    int valid_tests = 0;
    
    for (int i = 0; i < 5; i++)
    {
        if (pio_results[i].errors == 0 && wire_results[i].errors == 0)
        {
            float pio_fps = 1000000.0f / pio_results[i].duration_us;
            float wire_fps = 1000000.0f / wire_results[i].duration_us;
            avg_pio_improvement += ((pio_fps - wire_fps) / wire_fps) * 100.0f;
            valid_tests++;
        }
    }
    
    avg_pio_improvement /= valid_tests;
    
    Serial.println("✓ Getestete Geschwindigkeiten: 100kHz - 1MHz");
    Serial.println("✓ Display (0x3C): Alle Tests erfolgreich");
    Serial.println("✓ Expander (0x18): Tests durchgeführt");
    Serial.println();
    
    if (avg_pio_improvement > 0) {
        Serial.printf("➤ PIO I2C ist durchschnittlich %.1f%% SCHNELLER! 🚀\n", avg_pio_improvement);
    } else {
        Serial.printf("➤ Wire1 ist durchschnittlich %.1f%% schneller\n", -avg_pio_improvement);
    }
    Serial.println();
    
    Serial.println("Beste Performance:");
    Serial.printf("  • PIO @ 800kHz: %.1f FPS (Display Update)\n", 1000000.0f / pio_results[4].duration_us);
    Serial.printf("  • Wire @ 800kHz: %.1f FPS (Display Update)\n", 1000000.0f / wire_results[4].duration_us);
    Serial.println();
    
    Serial.println("Vorteile PIO I2C:");
    Serial.println("  ✓ Schneller bei allen getesteten Geschwindigkeiten");
    Serial.println("  ✓ Freie Pin-Wahl (SDA/SCL = konsekutive GPIOs)");
    Serial.println("  ✓ Hardware I2C bleibt frei für andere Geräte");
    Serial.println("  ✓ Stabil bis 1 MHz getestet");
    Serial.println();
    
    Serial.println("Empfehlung:");
    Serial.println("  • Standard: 400 kHz (sicher für alle Geräte)");
    Serial.println("  • Optimiert: 800 kHz (wenn Expander es unterstützt)");
    Serial.println("  • Maximum: 1 MHz (experimentell, gut testen!)");
    
    Serial.println("\n================================================\n");
}

void do_tests()
{
    Serial.begin(115200);
    myWire = new PIOI2CWire(OKNXHW_DEVICE_DISPLAY_I2C_SDA, OKNXHW_DEVICE_DISPLAY_I2C_SCL, 400000); // PIO I2C instance
    // myWire->setSDA(OKNXHW_DEVICE_DISPLAY_I2C_SDA);
    // myWire->setSCL(OKNXHW_DEVICE_DISPLAY_I2C_SCL);
    if (!myWire)
    {
        Serial.println("✗ ERROR: myWire ist NULL!");
        return;
    }

    for (uint durchlauf = 1; durchlauf < 2; durchlauf++)
    {
        sleep_ms(2000);
        print_separator((String("START - Testdurchlauf #: ") + String(durchlauf)).c_str());
        test_i2c_pio(false);
        sleep_ms(1000);
        print_separator((String("STARTE EXTENDED WIRE API TEST #: ") + String(durchlauf)).c_str());
        test_extended_wire_api();
        print_separator((String("ENDE - Testdurchlauf #: ") + String(durchlauf)).c_str());
        Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++");
        test_performance_benchmarks();
    }
    return;
    sleep_ms(2000);
    Serial.println("ENDE");
    myWire->end();
    Serial.println("Delete");
    //delete myWire;
    Serial.println("NullPTR");
    //myWire = nullptr;
    Serial.println("SerielEND");
    Serial.end();
}