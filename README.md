# Arduino Opta Modbus Controller

This project is a Modbus TCP controller running on **Arduino Opta**.
It controls relays and digital/analog inputs.


## Features

- Modbus TCP Server
- Configurable Relay Outputs
- Digital and Analog Inputs
- Modular Backend Architecture
- Debug output (optional via compile flags)
- Expandable backend architecture
- Designed for industrial automation projects

## Function Overview

This project implements a **Modbus TCP controller** for the Arduino Opta platform with a modular, device-oriented architecture.

Core functionality:

- Automatic initialization of the Opta controller and attached expansion modules
- Abstraction of local and expansion GPIO through backend interfaces
- Modbus mapping of real-world devices such as relays, digital inputs, variables, and a watchdog heartbeat
- Cyclic synchronization loop with configurable update interval
- Integrated watchdog supervision via hardware watchdog and Modbus heartbeat
- Fault monitoring with bit-coded error register (expansion, Modbus, sensor, heartbeat, general faults)

### Runtime Behavior

During startup, the firmware:

- Detects and initializes expansion modules (currently only mechanical expansions are supported)
- Falls back to a safe null-backend when no expansion is present, enabling safe compilation and runtime testing without hardware.
- Starts the Ethernet interface and Modbus TCP server
- Builds the Modbus register map dynamically from the device list

During operation, the main loop:

- Periodically updates all Modbus-mapped items
- Services the hardware watchdog
- Monitors expansion presence
- Tracks heartbeat state changes and updates error flags accordingly
- Automatically switches all relays to a predefined safe state if the network becomes unavailable or the heartbeat signal is missed

The project is designed for **robust industrial-style automation**

## Installation

Clone the repository:

```bash
git clone https://github.com/yourusername/your-repo-name.git
cd your-repo-name
```

## Configuration

This project uses a local configuration file that is not tracked by Git.

1.	Copy the example configuration:

```bash
cp conf.example.h conf.h
```

2.	Edit conf.h and adjust the settings to your environment.

## Build & Upload

Open the project in Arduino IDE or Arduino CLI and upload it to your Arduino Opta.


## License

MIT License
