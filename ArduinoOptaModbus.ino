/*
 * ==========================================================
 * Project: Arduino Modbus Controller
 * File: ArduinoOptaModbus.ino
 * Description:
 *   Main Arduino sketch for an Opta-based Modbus TCP controller.
 *   Initializes all I/O backends and devices, configures the
 *   ModbusHandler, and runs the periodic synchronization loop.
 *
 * Author:  Lukas Zuberbühler
 * License: MIT License
 * ==========================================================
 *
 * Copyright (c) 2025 Lukas Zuberbühler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file ArduinoOptaModbus.ino
 * @brief Main application for an Arduino Opta Modbus TCP controller.
 *
 * @details
 *   - Initializes Opta hardware and expansion modules  
 *   - Creates local and expansion I/O backends  
 *   - Instantiates Modbus-mapped devices (Relay, Input, Variable, Watchdog)  
 *   - Sets up Ethernet and the ModbusHandler  
 *   - Runs the periodic synchronization loop in `loop()`  
 */

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoModbus.h>
#include <drivers/Watchdog.h>

#include "config.h"
#include "PinBackend.h"
#include "OptaBlue.h"
#include "Relay.h"
#include "Input.h"
#include "Variable.h"
#include "Heartbeat.h"
#include "ModbusItem.h"
#include "ModbusHandler.h"


using namespace Opta;

// -----------------------------------------------------------------------------
// Global timing state
// -----------------------------------------------------------------------------

/**
 * @brief Timestamp (ms) of the last Modbus update cycle.
 */
unsigned long lastUpdate = 0;

/**
 * @brief Period (ms) between ModbusHandler update calls.
 */
unsigned long updateInterval = 100;

/**
 * @brief Global error code published via Modbus.
 *        Individual bits are defined below via ERR_* flags.
 */
uint16_t errorCode = 0;

/**
 * @brief Heartbeat state tracking.
 *
 * @details
 *  - `isAlive` reflects the current heartbeat value received from the watchdog.
 *  - `wasAlive` stores the previous cycle's value to detect rising/falling edges.
 *
 *  These flags are used to set or clear ERR_HEARTBEAT when the watchdog signal
 *  switches from alive → dead or dead → alive.
 */
bool wasAlive = true;
bool isAlive  = true;


// -----------------------------------------------------------------------------
// Error flags
// -----------------------------------------------------------------------------

/// @brief Watchdog/heartbeat signal lost.
constexpr uint16_t ERR_HEARTBEAT = 1 << 4;

/// @brief Expansion module not detected or faulty.
constexpr uint16_t ERR_EXPANSION = 1 << 3;

/// @brief Modbus initialization or communication error.
constexpr uint16_t ERR_MODBUS    = 1 << 2;

/// @brief General-purpose system error.
constexpr uint16_t ERR_GENERAL   = 1 << 1;

/// @brief Sensor-related error.
constexpr uint16_t ERR_SENSOR    = 1 << 0;


// -----------------------------------------------------------------------------
// I/O Backends
// -----------------------------------------------------------------------------

/**
 * @brief Backend for on-board Opta GPIO.
 */
PinBackend* localBackend = nullptr;

/**
 * @brief Backend for digital expansion module GPIO.
 */
PinBackend* expBackend = nullptr;

// -----------------------------------------------------------------------------
// Devices (Relays, Inputs, Variables)
// -----------------------------------------------------------------------------

// Relay outputs (local Opta pins)
// Each relay gets enter/leave safe-state behaviour via enum in config.h
StableRelay heatPump(localBackend, RELAY1, LED_RELAY1, SWITCH_ON,  RESTORE);
StableRelay legionella(localBackend, RELAY2, LED_RELAY2, SWITCH_OFF, RESTORE);
StableRelay lightGarden(localBackend, RELAY3, LED_RELAY3, SWITCH_ON, RESTORE);

// Expansion relays (expansion module pins)
SafeRelay wateringValve1(expBackend, D0, 0, SWITCH_OFF, IGNORE);
SafeRelay wateringValve2(expBackend, D1, 0, SWITCH_OFF, IGNORE);
SafeRelay wateringValve3(expBackend, D2, 0, SWITCH_OFF, IGNORE);

// Discrete input (local)
DiscreteInput doorSensor(localBackend, I1);

// Modbus-exposed variables
Variable<unsigned long> updateFreq(
    [](){ return updateInterval; },          // getter: returns current update interval
    [](unsigned long val){ updateInterval = val; } // setter: allows remote change
);

// Error code variable (read-only via Modbus)
Variable<uint16_t> errorCodeVar(
    [](){ return errorCode; }                 // getter: exposes errorCode
    // read-only → setter intentionally omitted
);

// Heartbeat object: writes to isAlive via provided setter
Heartbeat hb([](bool val){ isAlive = val; });


// -----------------------------------------------------------------------------
// Modbus item list
// -----------------------------------------------------------------------------

/**
 * @brief List of all IODevice instances exposed via Modbus mapping.
 *
 * Note: ModbusItem constructor assigns sequential base addresses during setup(),
 * so the array order determines the mapped (internal) register index 0..N-1.
 * Offsets for external addressing (e.g. 40000 for holdings) are added in ModbusItem.
 */
ModbusItem modbusList[] = {
    { &wateringValve1 },    // internal index 0  -> external coil offset + 0
    { &wateringValve2 },    // internal index 1
    { &wateringValve3 },    // internal index 2
    { &heatPump },          // internal index 3
    { &legionella },        // internal index 4
    { &lightGarden },       // internal index 5
    { &doorSensor },        // internal index 6  -> discrete input region
    { &updateFreq },        // internal index 7  -> holding region
    { &errorCodeVar },      // internal index 8  -> holding region          
    { &hb }                 // internal index 9  -> holding region (heartbeat)
};


// --- ModbusHandler ---
ModbusHandler modbusHandler(modbusList, sizeof(modbusList) / sizeof(ModbusItem), LEDG, LEDR);


// -------------------- Setup --------------------
void setup() {
    pinMode(LEDR, OUTPUT);
    digitalWrite(LEDR, HIGH);
    delay(500); // Warte auf USB-Enumeration
    Serial.begin(115200);
    delay(5000);
    #ifdef IDEBUG
    Serial.println("Sketch gestartet");
    Serial.print("Linking Expansion...");
    #endif

    OptaController.begin();
    delay(50);

    OptaController.update();

    localBackend = new LocalPinBackend();

    // Digitale Expansion holen
    ExpansionType_t t = OptaController.getExpansionType(0);

    if (t == EXPANSION_OPTA_DIGITAL_MEC) {
        // Typ ist digital, also sicheren Cast
        DigitalMechExpansion* digExp = static_cast<DigitalMechExpansion*>(&OptaController.getExpansion(0));
        expBackend = new ExpansionPinBackend(digExp);
  
         #ifdef IDEBUG
         Serial.println("ok.");
         #endif
    } else {
        #ifdef IDEBUG
        Serial.println("No valid Digital Expansion found.");
        #endif
        errorCode |= ERR_EXPANSION;
        expBackend = new NullPinBackend();
    }

    #ifdef IDEBUG
    Serial.println("Starting Ethernet + ModbusHandler...");
    #endif

    // Handler initialisieren: Ethernet + ModbusTCP-Server
    if (!modbusHandler.begin()) {
        errorCode |= ERR_MODBUS;
        #ifdef IDEBUG
        Serial.println("ModbusHandler init failed!");
        #endif
    }
    #ifdef IDEBUG
    Serial.print("Setting up items...");
    #endif
    // Alle Items einrichten
    modbusHandler.setupItems();

    #ifdef IDEBUG
    Serial.println("ok. Modbus TCP ready");
    #endif


    hb.attachHandler(&modbusHandler);
    #ifdef IDEBUG
    Serial.println("Heartbeat attached");
    #endif

    mbed::Watchdog::get_instance().start(WATCHDOG_TIMEOUT);
    
}

// -------------------- Loop --------------------
void loop() {

    OptaController.update();
 
    if (digitalRead(OPTA_CONTROLLER_DETECT_PIN) == LOW) {
        errorCode |= ERR_EXPANSION;
    }

    unsigned long now = millis();
    if (now - lastUpdate >= updateInterval) {

        mbed::Watchdog::get_instance().kick();

        lastUpdate = now;

        modbusHandler.update();
    }

    OptaController.checkForExpansions();

    // Update heartbeat error bit on edges of the watchdog state
    if (wasAlive && !isAlive) errorCode |=  ERR_HEARTBEAT;  // rising error: heartbeat lost
    if (!wasAlive && isAlive) errorCode &= ~ERR_HEARTBEAT;  // recovered: clear heartbeat error

    // Save previous heartbeat for edge detection next cycle
    wasAlive = isAlive;

}