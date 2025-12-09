/*
 * ==========================================================
 * Project: Arduino Modbus Controller
 * File: Relay.h
 * Description:
 * Implements relay devices for Modbus control.
 * SafeRelay supports automatic timeout, StableRelay is standard digital output.
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
#include "config.h"
#include "PinBackend.h"

/**
 * @brief Base class for a digital relay-type output device.
 * 
 * Handles basic Modbus coil mapping and provides a unified interface
 * for safe and stable relay implementations.
 */
class Relay : public IODevice {
protected:
    PinBackend*& _backend;
    uint8_t _pin;
    uint8_t _ledPin;
    bool _state = false;
    SafeAction _enterSafeState;
    SafeAction _leaveSafeState;
    bool _stateBeforeSafeState = false;
    bool _inSafeState = false;


    void triggerUpdate() {
        // Ensure backend flushes output changes
        _backend->updateDigitalOutputs();
    }

public:
    Relay(PinBackend*& backend, uint8_t pin, uint8_t ledPin = 0, SafeAction enterSafeState = IGNORE, SafeAction leaveSafeState = IGNORE)
        : _backend(backend), _pin(pin), _ledPin(ledPin), _enterSafeState(enterSafeState), _leaveSafeState(leaveSafeState)
    {
        setType(ModbusType::Coil);
    }

    void setup() override {
        _backend->pinMode(_pin, OUTPUT);
        _backend->digitalWrite(_pin, LOW);

        if (_ledPin) {
            _backend->pinMode(_ledPin, OUTPUT);
            _backend->digitalWrite(_ledPin, LOW);
        }

        triggerUpdate();
    }

    void enterSafeState() override {
 // Already in safe state → do nothing
        if (_inSafeState) return;

        #ifdef IDEBUG_RELAY
            Serial.print("Entering Safe State on pin: ");
            Serial.println(_pin);
            Serial.print("Mode: ");
            Serial.println(_enterSafeState);
            Serial.print("State to be saved: ");
            Serial.println(_state);
        #endif

        // No safe mode → nothing to do
        if (_enterSafeState == IGNORE) return;

        // Mark safe state active
        _inSafeState = true;

        // Save previous state once
        _stateBeforeSafeState = _state;

        switch (_enterSafeState) {
            case SWITCH_ON: on(); break;
            case SWITCH_OFF: off(); break;
            default: break;
        }

    }

    void leaveSafeState() override {
        // Only act if safe state was previously active
        if (!_inSafeState) return;

        _inSafeState = false;

        #ifdef IDEBUG_RELAY
            Serial.print("Leaving Safe State on pin: ");
            Serial.println(_pin);
            Serial.print("Mode: ");
            Serial.println(_leaveSafeState);
            Serial.print("State before Safe State: ");
            Serial.println(_stateBeforeSafeState);
        #endif

        switch (_leaveSafeState) {
            case IGNORE: return;
            case SWITCH_ON: on(); break;
            case SWITCH_OFF: off(); break;
            case RESTORE: _stateBeforeSafeState ? on() : off(); break;
            default: break;
        }
    }




    // Must be implemented by derived classes
    virtual void on() = 0;
    virtual void off() = 0;

    // Modbus coil read
    bool getCoilValue() const override { return _state; }

    // Modbus coil write
    void setFromCoil(bool val) override {
        if (val) on();
        else off();
    }
};


/**
 * @brief Relay with an automatic safety timeout.
 * 
 * Turns itself off after RELAY_MAX_ON milliseconds or a configurable
 * holding register value (in seconds).
 */
class SafeRelay : public Relay {
protected:
    unsigned long _startTime = 0;
    unsigned long _maxOnTime = RELAY_MAX_ON; // default safety window

public:
    SafeRelay(PinBackend*& backend, uint8_t pin, uint8_t ledPin = 0, SafeAction enterSafeState = IGNORE, SafeAction leaveSafeState = IGNORE)
        : Relay(backend, pin, ledPin, enterSafeState, leaveSafeState) {}

    void update() override {
        if (_state && (millis() - _startTime > _maxOnTime)) {
            off();
            #ifdef IDEBUG_RELAY
            Serial.print("SafeRelay auto-off: Pin ");
            Serial.println(_pin);
            #endif
        }
        triggerUpdate();
    }

    uint16_t getHoldingValue() const override {
        return static_cast<uint16_t>(_maxOnTime / 1000UL);
    }

    void setFromHolding(uint16_t val) override {
        _maxOnTime = static_cast<unsigned long>(val) * 1000UL;
    }

    void on() override {
        _backend->digitalWrite(_pin, HIGH);
        if (_ledPin) _backend->digitalWrite(_ledPin, HIGH);

        _state = true;
        _startTime = millis();
        triggerUpdate();

        #ifdef IDEBUG_RELAY
        Serial.print("Relay ON: Pin ");
        Serial.println(_pin);
        if (_ledPin) {
        Serial.print("Relay LED ON: Pin ");
        Serial.println(_ledPin);
        }
        #endif
    }

    void off() override {
        _backend->digitalWrite(_pin, LOW);
        if (_ledPin) _backend->digitalWrite(_ledPin, LOW);

        _state = false;
        _startTime = 0;
        triggerUpdate();

        #ifdef IDEBUG_RELAY
        Serial.print("Relay OFF: Pin ");
        Serial.println(_pin);
        if (_ledPin) {
            Serial.print("Relay LED OFF: Pin ");
            Serial.println(_ledPin);
        }
        #endif
    }
};


/**
 * @brief Simple stable relay that can be switched on/off without timeout.
 */
class StableRelay : public Relay {
public:
    StableRelay(PinBackend*& backend, uint8_t pin, uint8_t ledPin = 0, SafeAction enterSafeState = IGNORE, SafeAction leaveSafeState = IGNORE)
        : Relay(backend, pin, ledPin, enterSafeState, leaveSafeState) {}

    void update() override {
        triggerUpdate();
    }

    void on() override {
        _backend->digitalWrite(_pin, HIGH);
        if (_ledPin) _backend->digitalWrite(_ledPin, HIGH);
        _state = true;
        triggerUpdate();
    }

    void off() override {
        _backend->digitalWrite(_pin, LOW);
        if (_ledPin) _backend->digitalWrite(_ledPin, LOW);
        _state = false;
        triggerUpdate();
    }
};