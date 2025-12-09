/*
 * ==========================================================
 * Project: Arduino Modbus Controller
 * File: Input.h
 * Description:
 *   Implements digital and analog input devices exposed through
 *   Modbus Discrete Inputs and Input Registers.
 *
 *   Provides:
 *     - DiscreteInput  : boolean digital input via PinBackend
 *     - AnalogInput    : analog 10/12-bit input via PinBackend
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

#pragma once

#include <Arduino.h>
#include "IODevice.h"
#include "PinBackend.h"

/**
 * @class DiscreteInput
 * @brief Digital input mapped to a Modbus Discrete Input.
 *
 * Reads a hardware digital pin via a PinBackend and provides
 * the value to the ModbusHandler. The pin is configured as INPUT
 * on setup().
 */
class DiscreteInput : public IODevice {
private:
    PinBackend*& _backend;       ///< Reference to the pin backend (local or expansion)
    uint8_t      _pin;           ///< Hardware pin number
    bool         _state = false; ///< Last sampled digital state

public:
    /**
     * @brief Construct a new DiscreteInput.
     * @param backend Reference to active PinBackend
     * @param pin     Digital input pin number
     */
    DiscreteInput(PinBackend*& backend, uint8_t pin)
        : _backend(backend), _pin(pin) {

        setType(ModbusType::DiscreteInput);
    }

    /**
     * @brief Initialize hardware pin mode.
     */
    void setup() override {
        _backend->pinMode(_pin, INPUT);
    }

    /**
     * @brief Sample the current digital input state.
     */
    void update() override {
        _state = _backend->digitalRead(_pin);

        #ifdef IDEBUG_INPUT
        Serial.print("DiscreteInput pin ");
        Serial.print(_pin);
        Serial.print(" read: ");
        Serial.println(_state);
        #endif
    }

    /**
     * @brief Return the current value for Modbus Discrete Input.
     * @return true if HIGH, false if LOW
     */
    bool getDiscreteValue() const override {
        return _state;
    }
};


/**
 * @class AnalogInput
 * @brief Analog input mapped to a Modbus Input Register.
 *
 * Reads a hardware analog pin via a PinBackend and exposes the
 * sampled 16-bit value to Modbus. The pin is configured as INPUT
 * on setup().
 */
class AnalogInput : public IODevice {
private:
    PinBackend*& _backend;     ///< Reference to the pin backend
    uint8_t      _pin;         ///< Analog pin number
    uint16_t     _state = 0;   ///< Last sampled analog value

public:
    /**
     * @brief Construct a new AnalogInput.
     * @param backend Reference to active PinBackend
     * @param pin     Analog input pin number
     */
    AnalogInput(PinBackend*& backend, uint8_t pin)
        : _backend(backend), _pin(pin) {

        setType(ModbusType::InputRegister);
    }

    /**
     * @brief Initialize hardware pin mode.
     */
    void setup() override {
        _backend->pinMode(_pin, INPUT);
    }

    /**
     * @brief Sample current analog value.
     */
    void update() override {
        _state = _backend->analogRead(_pin);

        #ifdef IDEBUG_INPUT
        Serial.print("AnalogInput pin ");
        Serial.print(_pin);
        Serial.print(" read: ");
        Serial.println(_state);
        #endif
    }

    /**
     * @brief Return the current value for Modbus Input Register.
     * @return 16-bit analog reading
     */
    uint16_t getInputValue() const override {
        return _state;
    }
};