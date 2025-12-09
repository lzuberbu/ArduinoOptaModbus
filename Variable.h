/*
 * ==========================================================
 * Project: Arduino Modbus Controller
 * File: Variable.h
 * Description:
 * Template class representing a Modbus-exposed variable.
 * Supports reading via a getter and optional writing via a setter.
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
#include <functional>

/**
 * @brief Template class representing a Modbus-exposed variable.
 * 
 * The variable can be read via a getter function and optionally written via a setter function.
 * It maps to a Modbus Holding Register by default.
 * 
 * @tparam T Type of the variable (e.g., uint16_t, int, unsigned long, etc.)
 */
template<typename T>
class Variable : public IODevice {
private:
    std::function<T()> _getter;        /**< Function to read the current value */
    std::function<void(T)> _setter;    /**< Optional function to write a new value */

public:
    /**
     * @brief Constructor
     * @param getter Function to read the current value (required)
     * @param setter Function to write a new value (optional)
     */
    Variable(std::function<T()> getter, std::function<void(T)> setter = nullptr)
        : _getter(getter), _setter(setter) {
        setType(ModbusType::HoldingRegister);
    }

    /**
     * @brief Called once during system setup to configure the variable
     * 
     * For `Variable` this is a no-op.
     */
    void setup() override {}

    /**
     * @brief Called periodically to update internal state
     * 
     * For `Variable` this is a no-op.
     */
    void update() override {}

    // --- Modbus Holding Register Access ---

    /**
     * @brief Read the value of the variable for Modbus Holding Register
     * @return The current value, cast to uint16_t. Returns INVALID_VALUE if getter is not set.
     */
    uint16_t getHoldingValue() const override {
        if (_getter) {
            uint16_t val = static_cast<uint16_t>(_getter());
            #ifdef IDEBUG_VARIABLE
            Serial.print("Variable read: ");
            Serial.println(val);
            #endif
            return val;
        }

        #ifdef IDEBUG_VARIABLE
        Serial.println("Variable read: INVALID_VALUE");
        #endif
        return INVALID_VALUE;
    }

    /**
     * @brief Write a value to the variable from Modbus Holding Register
     * @param val Value to write
     * 
     * If no setter function is provided, the write is ignored (read-only).
     */
    void setFromHolding(uint16_t val) override {
        if (_setter) {
            _setter(static_cast<T>(val));
            #ifdef IDEBUG_VARIABLE
            Serial.print("Variable write: ");
            Serial.println(val);
            #endif
        }
        #ifdef IDEBUG_VARIABLE
        else {
            Serial.print("Variable write ignored (read-only): ");
            Serial.println(val);
        }
        #endif
    }
};