/*
 * ==========================================================
 * Project: Arduino Modbus Controller
 * File: PinBackend.h
 * Description:
 * Abstract interface for I/O backends (local pins, expansion modules, or null backend).
 * Provides uniform pinMode, digitalWrite, digitalRead, and analogRead access.
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

#include <Arduino.h>
#include "OptaBlue.h"

using namespace Opta;

/**
 * @brief Abstract base class for any pin backend (local MCU pins or expansion module)
 */
class PinBackend {
public:
    virtual void pinMode(pin_size_t pin, PinMode mode) = 0;
    virtual void digitalWrite(pin_size_t pin, PinStatus val) = 0;
    virtual void updateDigitalOutputs() = 0;
    virtual int digitalRead(pin_size_t pin) = 0;
    virtual int analogRead(pin_size_t pin) { return 0; }
    virtual ~PinBackend() = default;
};


/**
 * @brief Backend using the local MCU pins
 */
class LocalPinBackend : public PinBackend {
public:
    LocalPinBackend() = default;

    void pinMode(pin_size_t pin, PinMode mode) override { ::pinMode(pin, mode); }
    void digitalWrite(pin_size_t pin, PinStatus val) override { ::digitalWrite(pin, val); }
    int digitalRead(pin_size_t pin) override { return ::digitalRead(pin); }
    int analogRead(pin_size_t pin) override { return ::analogRead(pin); }
    void updateDigitalOutputs() override {}
};


/**
 * @brief Backend for Opta digital expansion module
 */
class ExpansionPinBackend : public PinBackend {
private:
    DigitalExpansion* _exp;

public:
    explicit ExpansionPinBackend(DigitalExpansion* exp) : _exp(exp) {}

    void pinMode(pin_size_t pin, PinMode mode) override {
        // Many expansion modules do not support pinMode
        (void)pin; (void)mode;
    }

    void updateDigitalOutputs() override { _exp->updateDigitalOutputs(); }
    void digitalWrite(pin_size_t pin, PinStatus val) override {
        _exp->digitalWrite(pin, val);
        _exp->updateDigitalOutputs();
    }

    int digitalRead(pin_size_t pin) override { return _exp->digitalRead(pin); }

    int analogRead(pin_size_t pin) override {
        // Expansion modules may not support analog input
        (void)pin;
        return 0;
    }
};


/**
 * @brief Null backend (safe default)
 */
class NullPinBackend : public PinBackend {
public:
    void pinMode(pin_size_t pin, PinMode mode) override {}
    void digitalWrite(pin_size_t pin, PinStatus val) override {}
    void updateDigitalOutputs() override {}
    int digitalRead(pin_size_t pin) override { return 0; }
    int analogRead(pin_size_t pin) override { return 0; }
};