# IPC Automotive Hardware Baseline

Confirmed by the user on 2026-07-12.

## Runtime architecture

- Raspberry Pi 3B+ is the VCU and will run the Qt/QML dashboard.
- Arduino Uno is the hardware ECU.
- The Ubuntu ARM64 VM on the Mac M1 Pro is an independent build/development environment.
- Current Pi-to-Uno transport is UART at 9600 baud.
- Two MCP2515 CAN modules are available, but CAN migration is deferred until Phase 3 works reliably over UART.

## Arduino Uno I/O

- Shift buttons: D9 and D10, used with `INPUT_PULLUP`.
- Pedal potentiometer: outer terminals to 5V and GND, wiper to A0.
- Motor: 5V DC motor through an L298 V3 driver.
- L298 ENA/PWM: D5.
- L298 INA: D7.
- L298 INB: D8.
- L298 GND and Uno GND are connected.
- Uno USB debug UART remains at 115200 baud in the current firmware.

## Raspberry Pi UART

- A USB-UART adapter is plugged into a Raspberry Pi USB port for Pi-to-Uno communication.
- The Pi GPIO UART is reserved for debug logs.
- A 5V-to-3.3V logic level shifter is used for the UART signals.
- Uno D2 is the VCU UART RX pin and Uno D3 is the VCU UART TX pin.
- The Uno firmware uses `SoftwareSerial` on D2/D3 at 9600 baud.

## Reference image clarification

- The supplied reference image shows the electrical purpose of the 3.3V-to-5V level shifter, but its Arduino D0/D1 wiring is not the project's actual wiring.
- Arduino hardware UART D0/D1 is not used for the Pi-to-Uno VCU channel.
