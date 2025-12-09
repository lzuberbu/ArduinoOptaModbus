/*
 * ==========================================================
 * Project: Arduino Modbus Controller
 * File: Heartbeat.h
 * Description:
 * Class representing a Modbus-exposed heartbeat variable.
 * Supports monitoring system alive state and notifying a handler
 * on state changes via an optional setter function.
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
#include "ModbusHandler.h"
#include <functional>

/**
 * @brief Class representing a Modbus-exposed heartbeat variable.
 * 
 * The heartbeat variable tracks the alive status of the system.
 * It maps to a Modbus Holding Register by default.  
 * An optional setter function can be provided to react when the heartbeat state changes
 * (e.g., to notify a ModbusHandler to enter or exit safe state).
 */
class Heartbeat : public IODevice {
private:
    ModbusHandler* _handler = nullptr;                /**< Optional pointer to the ModbusHandler */
    std::function<void(bool)> _setter = nullptr;     /**< Optional function called on state change */
    bool _isAlive = false;                            /**< Current alive state */
    unsigned long _lastChange = 0;                   /**< Timestamp of last change */
    uint16_t _val;                                   /**< Cached value for Modbus access */

public:
    /**
     * @brief Constructor
     * @param setter Optional function called when the heartbeat state changes
     */
    Heartbeat(std::function<void(bool)> setter = nullptr)
        : _setter(setter) {
        setType(ModbusType::HoldingRegister);
    }

    /**
     * @brief Attach a ModbusHandler to allow entering/exiting safe state
     * @param handler Pointer to ModbusHandler
     */
    void attachHandler(ModbusHandler* handler) {
        _handler = handler;
    }

    /**
     * @brief Called once during system setup to configure the variable
     * 
     * For Heartbeat this is a no-op.
     */
    void setup() override {}

    /**
     * @brief Called periodically to update internal state
     * 
     * If the heartbeat timeout has elapsed, marks the system as not alive
     * and calls the setter function and ModbusHandler safe state methods as appropriate.
     */
    void update() override {   
        if (millis() - _lastChange > HEARTBEAT_DELAY) {
            if (_isAlive) {
                _isAlive = false;
                if (_handler) _handler->enterSafeState();
                if (_setter) _setter(_isAlive);
            }
        } else {
            if (!_isAlive) {
                _isAlive = true;
                if (_handler) _handler->exitSafeState();
                if (_setter) _setter(_isAlive);
            }
        }
    }

    // --- Modbus Holding Register Access ---

    /**
     * @brief Read the value of the variable for Modbus Holding Register
     * @return The current cached value
     */
    uint16_t getHoldingValue() const override {
        #ifdef IDEBUG_HEARTBEAT
        Serial.print("Heartbeat read: ");
        Serial.println(_val);
        #endif
        return _val;
    }

    /**
     * @brief Write a value to the variable from Modbus Holding Register
     * @param val Value to write
     * 
     * Updates the internal cache and resets the last change timestamp.
     */
    void setFromHolding(uint16_t val) override { 
        _val = val;
        _lastChange = millis();
        #ifdef IDEBUG_HEARTBEAT
        Serial.print("Heartbeat write: ");
        Serial.println(val);
        #endif
    }
};