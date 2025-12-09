/*
 * ==========================================================
 * Project: Arduino Modbus Controller
 * File: ModbusItem.h
 * Description:
 * Represents a single Modbus-mapped device or variable.
 * Synchronizes between physical IODevice and Modbus registers.
 * Author: Lukas Zuberbühler
 * License: MIT License
 * ==========================================================
 *
 * Copyright (c) 2025 Lukas Zuberbühler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#pragma once
#include "IODevice.h"
#include <ArduinoModbus.h>

/**
 * @brief Represents a single Modbus-mapped device or variable.
 * 
 * This class handles synchronization between a physical or virtual IODevice
 * and the Modbus registers (coils, discrete inputs, holding registers, input registers).
 */
class ModbusItem {
private:
    uint16_t _baseAddress = 0;        /**< Base Modbus address for the device */
    IODevice* _device;            /**< Pointer to the underlying physical device or variable */
    uint16_t _registerCount = 1;  /**< Number of registers used (default: 1) */
    uint16_t _lastValue = 0;      /**< Cached last value to prevent redundant writes */
    uint16_t _lastValue2 = 0;     /**< Optional second cache for devices using multiple registers */

public:
    /**
     * @brief Constructor
     * @param baseAddress The base Modbus address of the device
     * @param device Pointer to the physical IODevice or variable
     */
    ModbusItem(IODevice* device)
        : _device(device) {}

    /**
     * @brief Initialize the underlying IODevice
     */
    void setup(uint16_t baseAddress) {
        _baseAddress = baseAddress;
        if (_device) _device->setup();
    }

    /**
     * @brief enter safe state of device
     */
    void enterSafeState() {
        if (_device) _device->enterSafeState();
    }

    /**
     * @brief enter safe state of device
     */
    void exitSafeState() {
        if (_device) _device->leaveSafeState();
    }



    /**
     * @brief Update the internal device logic (timeouts, auto-off, debouncing, etc.)
     */
    void updateDevice() {
        if (_device) _device->update();
    }

    /**
     * @brief Synchronize values from Modbus client to the device
     * @param server Reference to the Modbus TCP server
     */
    void updateFromModbus(ModbusTCPServer& server) {
        if (!_device) return;

        switch (_device->getType()) {
            case ModbusType::Coil: {
                bool val = server.coilRead(_baseAddress + MODBUS_COIL_OFFSET);
                if (val != static_cast<bool>(_lastValue)) {
                    _device->setFromCoil(val);
                    _lastValue = val;
                    #ifdef IDEBUG_RELAY
                    Serial.print("Coil UpdateFromModbus: Address ");
                    Serial.print(_baseAddress + MODBUS_COIL_OFFSET);
                    Serial.print(", Value ");
                    Serial.println(val);
                    #endif
                }

                // Optional: holding register for extended data (e.g., relay on-time)
                uint16_t val2 = server.holdingRegisterRead(_baseAddress + MODBUS_HOLDING_OFFSET);
                if (val2 != _lastValue2) {
                    _device->setFromHolding(val2);
                    _lastValue2 = val2;
                    #ifdef IDEBUG_RELAY
                    Serial.print("Holding UpdateFromModbus: Address ");
                    Serial.print(_baseAddress);
                    Serial.print(", Value ");
                    Serial.println(val2);
                    #endif
                }
                break;
            }

            case ModbusType::HoldingRegister: {
                uint16_t val = server.holdingRegisterRead(_baseAddress + MODBUS_HOLDING_OFFSET);
                if (val != _lastValue) {
                    _device->setFromHolding(val);
                    _lastValue = val;
                    #ifdef IDEBUG_VARIABLE
                    Serial.print("Holding UpdateFromModbus: Address ");
                    Serial.print(_baseAddress);
                    Serial.print(", Value ");
                    Serial.println(val);
                    #endif
                }
                break;
            }

            default:
                break;
        }
    }

    /**
     * @brief Synchronize device state to the Modbus server
     * @param server Reference to the Modbus TCP server
     */
    void updateToModbus(ModbusTCPServer& server) {
        if (!_device) return;

        switch (_device->getType()) {
            case ModbusType::Coil: {
                bool state = _device->getCoilValue();
                if (state != static_cast<bool>(_lastValue)) {
                    server.coilWrite(_baseAddress + MODBUS_COIL_OFFSET, state);
                    _lastValue = state;
                    #ifdef IDEBUG_RELAY
                    Serial.print("Coil UpdateToModbus: Address ");
                    Serial.print(_baseAddress + MODBUS_COIL_OFFSET);
                    Serial.print(", State ");
                    Serial.println(state);
                    #endif
                }

                // Always export extended relay data
                uint16_t tmp = _device->getHoldingValue();
                if (tmp != _lastValue2) {
                    _lastValue2 = tmp;
                    server.holdingRegisterWrite(_baseAddress + MODBUS_HOLDING_OFFSET, tmp);
                    #ifdef IDEBUG_RELAY
                    Serial.print("Holding UpdateToModbus: Value ");
                    Serial.println(tmp);
                    #endif
                }
                break;
            }

            case ModbusType::DiscreteInput: {
                bool state = _device->getDiscreteValue();
                if (state != static_cast<bool>(_lastValue)) {
                    server.discreteInputWrite(_baseAddress  + MODBUS_DISCRETE_OFFSET, state);
                    _lastValue = state;
                    #ifdef IDEBUG_INPUT
                    Serial.print("DiscreteInput UpdateToModbus: Address ");
                    Serial.print(_baseAddress + MODBUS_DISCRETE_OFFSET);
                    Serial.print(", Value ");
                    Serial.println(state);
                    #endif
                }
                break;
            }

            case ModbusType::HoldingRegister: {
                uint16_t state = _device->getHoldingValue();
                if (state != _lastValue) {
                    server.holdingRegisterWrite(_baseAddress + MODBUS_HOLDING_OFFSET, state);
                    _lastValue = state;
                    #ifdef IDEBUG_VARIABLE
                    Serial.print("Holding UpdateToModbus: Address ");
                    Serial.print(_baseAddress + MODBUS_HOLDING_OFFSET);
                    Serial.print(", Value ");
                    Serial.println(state);
                    #endif
                }
                break;
            }

            case ModbusType::InputRegister: {
                uint16_t state = _device->getInputValue();
                if (state != _lastValue) {
                    server.inputRegisterWrite(_baseAddress + MODBUS_INPUT_OFFSET, state);
                    _lastValue = state;
                    #ifdef IDEBUG_INPUT
                    Serial.print("InputRegister UpdateToModbus: Address ");
                    Serial.print(_baseAddress + MODBUS_INPUT_OFFSET);
                    Serial.print(", Value ");
                    Serial.println(state);
                    #endif
                }
                break;
            }

            default:
                break;
        }
    }

    /**
     * @brief Perform a full synchronized update cycle
     * 
     * This updates the internal device, synchronizes values from the Modbus client
     * to the device, and then updates the device state back to the Modbus server.
     * @param server Reference to the Modbus TCP server
     */
    void update(ModbusTCPServer& server) {
        updateDevice();           // Local device update
        updateFromModbus(server); // Modbus client → device
        updateToModbus(server);   // Device → Modbus client
    }
};