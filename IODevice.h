/*
 * ==========================================================
 * Project: Arduino Modbus Controller
 * File: IODevice.h
 * Description:
 * Base class for all I/O devices (digital, analog, variables) mapped to Modbus registers.
 * Provides common interface for setup, update, and Modbus accessors. * Author: Lukas Zuberbühler
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

// IODevice.h
// Public API for hardware/virtual I/O devices used in the Modbus project.

#include <cstdint>

/**
 * @brief Special constant returned when a register value is invalid or not available.
 */
static constexpr uint16_t INVALID_VALUE = 0xFFFFu;

/**
 * @brief Types of Modbus mappings a device can expose.
 */
enum class ModbusType {
    Undefined,       ///< No Modbus mapping assigned.
    Coil,            ///< Single-bit read/write.
    DiscreteInput,   ///< Single-bit read-only.
    HoldingRegister, ///< 16-bit read/write.
    InputRegister    ///< 16-bit read-only.
};

/**
 * @brief Types of actions in fail state.
 */
enum SafeAction : uint8_t {
    IGNORE = 0,
    SWITCH_ON = 1,
    SWITCH_OFF = 2,
    RESTORE = 3
};

/**
 * @brief Abstract base class for any device that can be mapped to Modbus.
 *
 * This includes local I/O pins, expansion pins, virtual variables, timers,
 * and other software-defined Modbus endpoints.
 *
 * Subclasses override only the relevant methods for their Modbus type.
 */
class IODevice {
protected:
    ModbusType _type = ModbusType::Undefined; ///< Current Modbus mapping type.
    /**
     * @brief Set the Modbus type of this device (used by subclasses).
     */
    void setType(ModbusType t) { _type = t; }

public:
    IODevice() = default;

    // ---------------------------------------------------------------------
    // Lifecycle
    // ---------------------------------------------------------------------

    /**
     * @brief Initialize hardware or internal state.
     *
     * Called once during startup.
     */
    virtual void setup() {}

    /**
     * @brief Periodic update routine.
     *
     * Called from the main loop; used for polling, timing, auto-off logic, etc.
     */
    virtual void update() {}

    // ---------------------------------------------------------------------
    // Modbus mapping information
    // ---------------------------------------------------------------------

    /**
     * @brief Returns the Modbus mapping type (e.g., Coil, HoldingRegister).
     */
    ModbusType getType() const { return _type; }

    // ---------------------------------------------------------------------
    // Modbus read/write API
    //
    // Default implementations return INVALID_VALUE or no-op.
    // Subclasses override only the types they support.
    // ---------------------------------------------------------------------

    /**
     * @brief Called if network is down.
     */
    virtual void enterSafeState() {}
 
    /**
     * @brief Called if network is up again.
     */
    virtual void leaveSafeState() {}

    /**
     * @brief Read the coil (single-bit) value.
     */
    virtual bool getCoilValue() const { return false; }

    /**
     * @brief Write a single-bit coil value.
     */
    virtual void setFromCoil(bool /*value*/) {}

    /**
     * @brief Read a 16-bit holding register value.
     */
    virtual uint16_t getHoldingValue() const { return INVALID_VALUE; }

    /**
     * @brief Write a 16-bit holding register value.
     */
    virtual void setFromHolding(uint16_t /*value*/) {}

    /**
     * @brief Read a single-bit discrete input value.
     */
    virtual bool getDiscreteValue() const { return false; }

    /**
     * @brief Read a 16-bit input register value.
     */
    virtual uint16_t getInputValue() const { return INVALID_VALUE; }

    /**
     * @brief Virtual destructor (required for polymorphic base class).
     */
    virtual ~IODevice() = default;
};