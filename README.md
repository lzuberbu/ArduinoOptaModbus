# Arduino Opta Modbus Controller

This project is a Modbus TCP controller running on **Arduino Opta**.
It controls relays and digital/analog inputs and supports Docker-based backends.


## Features

- Modbus TCP Server
- Configurable Relay Outputs
- Digital and Analog Inputs
- Modular Backend Architecture
- Debug output (optional via compile flags)
- Expandable backend architecture
- Designed for industrial automation projects


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
