 /*
 * ==========================================================
 * Project: Arduino Modbus Controller
 * File: ModbusHandler.h
 * Description:
 *   Handles Modbus TCP communication for a list of mapped IODevices
 *   (Relays, Inputs, Variables). Manages Ethernet initialization,
 *   DHCP/fallback IP, LED status indicators, Modbus register setup,
 *   and periodic update cycles to synchronize device state with Modbus clients.
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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "config.h"
#include "ModbusItem.h"
#include <ArduinoModbus.h>
#include <Ethernet.h>

/**
 * @class ModbusHandler
 * @brief Manages Modbus TCP server, Ethernet link, LEDs, and IODevice synchronization.
 *
 * @details
 *   - Initializes Ethernet (DHCP/fallback)  
 *   - Configures Modbus registers for mapped IODevices  
 *   - Handles TCP client connections and polls Modbus requests  
 *   - Updates device states periodically  
 *   - Provides safe-state mechanisms for critical devices  
 */
class ModbusHandler {

private:
    ModbusItem*       _items;          ///< Array of mapped Modbus items
    size_t            _numItems;       ///< Number of items in array
    EthernetServer    _ethServer;      ///< TCP server for incoming clients
    uint8_t           ledGreenPin;     ///< Green LED (OK) pin
    uint8_t           ledRedPin;       ///< Red LED (Error) pin
    byte              _mac[6] = MAC_ADDRESS; ///< MAC address for Ethernet
    const char*       _hostname = HOSTNAME;  ///< Hostname for DHCP

    ModbusTCPServer   _server;         ///< Modbus TCP server instance
    EthernetClient    _ethClient;      ///< Active TCP client
    int               _status = 0;     ///< Internal status code
    bool              _linkWasDown = false; ///< Tracks previous Ethernet link state
    bool              _isSafeState = false; ///< Safe-state active flag
    bool              _wasSafeState = false;

public:
    /**
     * @brief Constructor
     * @param items Array of ModbusItem objects
     * @param numItems Number of items
     * @param greenPin Pin for green LED
     * @param redPin Pin for red LED
     * @param port TCP port (default 502)
     */
    ModbusHandler(ModbusItem* items, size_t numItems,
                  uint8_t greenPin, uint8_t redPin, uint16_t port = 502)
        : _items(items), _numItems(numItems),
          _ethServer(port), ledGreenPin(greenPin), ledRedPin(redPin) {}

    /**
     * @brief Initialize Ethernet, LEDs, and Modbus TCP server.
     * @return true on success
     */
    bool begin() {
        #ifdef IDEBUG
        Serial.println("Starting ModbusHandler...");
        #endif

        pinMode(ledGreenPin, OUTPUT);
        pinMode(ledRedPin, OUTPUT);
        digitalWrite(ledGreenPin, HIGH);
        digitalWrite(ledRedPin, HIGH);
        delay(1000);

        // Start Ethernet with DHCP
        if (Ethernet.begin(_mac) == 0) {
            digitalWrite(ledGreenPin, LOW);
            digitalWrite(ledRedPin, HIGH);
            
            if (Ethernet.hardwareStatus() == EthernetNoHardware) {
                _linkWasDown = true;
                #ifdef IDEBUG
                Serial.println("Ethernet hardware not found!");
                #endif
            } else if (Ethernet.linkStatus() == LinkOFF) {
                #ifdef IDEBUG
                Serial.println("Ethernet cable not connected.");
                #endif
                _linkWasDown = true;
            }

            // Fallback IP
            Ethernet.begin(_mac, fallbackIP);
            #ifdef IDEBUG
            Serial.println("DHCP failed, using fallback IP");
            #endif
        } else {
            _linkWasDown = false;
        }

        Ethernet.setHostname(_hostname);
        delay(1000);

        _ethServer.begin();
        return startModbusServer();
    }

    /**
     * @brief Configure Modbus registers and start server
     * @return true if server started successfully
     */
    bool startModbusServer() {
        if (!_server.begin()) {
            #ifdef IDEBUG
            Serial.println("Failed to start Modbus TCP server!");
            #endif
            digitalWrite(ledGreenPin, LOW);
            digitalWrite(ledRedPin, HIGH);
            return false;
        }

        _server.configureCoils(MODBUS_COIL_OFFSET, _numItems);
        _server.configureHoldingRegisters(MODBUS_HOLDING_OFFSET, _numItems);
        _server.configureInputRegisters(MODBUS_INPUT_OFFSET, _numItems);
        _server.configureDiscreteInputs(MODBUS_DISCRETE_OFFSET, _numItems);

        // Clear all registers
        for (size_t i = MODBUS_COIL_OFFSET; i < MODBUS_COIL_OFFSET + _numItems; i++)
            _server.coilWrite(i, false);
        for (size_t i = MODBUS_HOLDING_OFFSET; i < MODBUS_HOLDING_OFFSET + _numItems; i++)
            _server.holdingRegisterWrite(i, 0);
        for (size_t i = MODBUS_INPUT_OFFSET; i < MODBUS_INPUT_OFFSET + _numItems; i++)
            _server.inputRegisterWrite(i, 0);
        for (size_t i = MODBUS_DISCRETE_OFFSET; i < MODBUS_DISCRETE_OFFSET + _numItems; i++)
            _server.discreteInputWrite(i, false);

        digitalWrite(ledRedPin, LOW);
        digitalWrite(ledGreenPin, HIGH);
        return true;
    }

    /**
     * @brief Initialize all mapped Modbus items
     */
    void setupItems() {
        for (size_t i = 0; i < _numItems; ++i) {
            _items[i].setup(i);
        }
    }

    /**
     * @brief Check Ethernet link and maintain DHCP
     */
    void checkEthernet() {

        static unsigned long nextCheck = 0;
        const unsigned long interval = 500;
        if (millis() < nextCheck) return;
        nextCheck = millis() + interval;

        if (Ethernet.linkStatus() == LinkOFF) {
            _linkWasDown = true;
            digitalWrite(ledGreenPin, LOW);
            static bool toggle = false;
            toggle = !toggle;
            digitalWrite(ledRedPin, toggle);
            enterSafeState();
            return;
        }

        int result = Ethernet.maintain();
        switch (result) {
            case 0: case 1: case 2: case 4:
                digitalWrite(ledGreenPin, HIGH);
                digitalWrite(ledRedPin, LOW);
                
                if (_linkWasDown) {
                    exitSafeState();
                    _linkWasDown = false;
                }
                break;
            case 3:
                if (Ethernet.begin(_mac) == 0) {
                    Ethernet.begin(_mac, fallbackIP);
                    Ethernet.setHostname(_hostname);
                    digitalWrite(ledGreenPin, LOW);
                    digitalWrite(ledRedPin, HIGH);
                } else {
                    if (_linkWasDown) {
                        _ethServer.begin();
                        startModbusServer();
                        _linkWasDown = false;
                        exitSafeState();   
                    }
                    digitalWrite(ledGreenPin, HIGH);
                    digitalWrite(ledRedPin, LOW);
                }
                break;
        }
    }

    /**
     * @brief Main update: handle client connections and refresh items
     */
    void update() {

        checkEthernet();

        if (!_ethClient || !_ethClient.connected()) {
            EthernetClient newClient = _ethServer.accept();
            if (newClient) {
                _ethClient = newClient;
                _server.accept(_ethClient);
            }
        }

        if (_ethClient && _ethClient.connected()) {
            _server.poll();
        } else {
            _ethClient.stop();
        }

        updateItems();
    }

    /**
     * @brief Update all mapped Modbus items
     */
    void updateItems() {
        for (size_t i = 0; i < _numItems; ++i) {
            _items[i].update(_server);
        }
    }

    /**
     * @brief Enter safe state on all devices
     */
    void enterSafeState() {
        if (_isSafeState) return;
        _isSafeState = true;
        for (size_t i = 0; i < _numItems; ++i) {
            _items[i].enterSafeState();
        }
    }

    /**
     * @brief Exit safe state on all devices
     */
    void exitSafeState() {
        if (!_isSafeState) return;
        _isSafeState = false;
        for (size_t i = 0; i < _numItems; ++i) {
            _items[i].exitSafeState();
        }
    }

    /**
     * @brief Get pointer to Modbus TCP server
     * @return ModbusTCPServer*
     */
    ModbusTCPServer* server() { return &_server; }

};