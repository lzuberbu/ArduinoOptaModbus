/*
 * ==========================================================
 * Project: Arduino Modbus Controller
 * File: config.h
 * Description:
 * Central configuration file for the Arduino Modbus project.
 * Defines constants for relay maximum ON time, network settings (MAC address, hostname),
 * Modbus register counts, and fallback IP address for DHCP failure.
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

//#define IDEBUG
//#define IDEBUG2
//#define IDEBUG_RELAY
//#define IDEBUG_VARIABLE
//#define IDEBUG_INPUT
//#define IDEBUG_HEARTBEAT

/**
 * @brief Maximum ON time (in milliseconds) for SafeRelay devices.
 * 
 * SafeRelay automatically turns off after this period to prevent hardware damage.
 */
#define RELAY_MAX_ON 300000

/**
 * @brief Duration in milliseconds for the hardware timer
 * 
 * The hardware timer resets the device in case of malfunction.
 */
#define WATCHDOG_TIMEOUT 300000

/**
 * @brief Maximum delay between heartbeat signal (in milliseconds).
 * 
 * Watchdog will indicate failure if no heartbeat is recieved within this delay.
 */
#define HEARTBEAT_DELAY 300000


/**
 * @brief MAC address for the device.
 * 
 * Used for Ethernet initialization.
 */
#define MAC_ADDRESS { 0xA8, 0x61, 0x0A, 0x50, 0xA7, 0xD4 }

/**
 * @brief Hostname for network identification.
 */
#define HOSTNAME "opta01"

/**
 * @brief Fallback static IP address used if DHCP fails.
 */
IPAddress fallbackIP(192, 168, 1, 100);


/**
 * @brief Modbus register base offsets.
 *
 * These offsets define the base address ranges for the different
 * Modbus data areas. An offset of 0 would also work, but the
 * values are used to keep the address spaces clearly separated.
 */
#define MODBUS_COIL_OFFSET     0    
#define MODBUS_DISCRETE_OFFSET 10000
#define MODBUS_INPUT_OFFSET    30000
#define MODBUS_HOLDING_OFFSET  40000