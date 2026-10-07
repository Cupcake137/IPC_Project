# IPC Automotive Hardware Baseline

Confirmed by the user on 2026-07-12.

## Runtime architecture

- Raspberry Pi 3B+ is the VCU and runs the terminal C++ backend.
- Arduino Uno is the hardware ECU.
- ESP32 is the 4x4 keypad controller and publishes key events to the Pi Mosquitto broker over Wi-Fi.
- The Ubuntu ARM64 VM on the Mac M1 Pro is an independent build/development environment.
- Current Pi-to-Uno transport is UART at 9600 baud.
- Two MCP2515 CAN modules are available, but CAN migration is deferred until the UART implementation is stable.

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
- Arduino hardware UART D0/D1 is not used for the Pi-to-Uno VCU channel.

## ESP32 Keypad I/O

- PlatformIO board profile: `esp32dev`.
- Matrix drive pins: GPIO16, GPIO4, GPIO2, GPIO15.
- Matrix sense pins: GPIO19, GPIO18, GPIO5, GPIO17.
- MQTT CAN topic: `ipc/can/swc` at QoS 1.
- MQTT retained status topic: `ipc/swc/status`.
- Wi-Fi and MQTT credentials live only in ignored `firmware/esp32_keypad_controller/include/secrets.h`.
- GPIO2 and GPIO15 are boot-strapping pins; do not hold keypad keys during ESP32 reset or power-on.

## CAN Data Contract

- The source of truth is `can_database/ipc_virtual_can.dbc`.
- Uno UART and ESP32 MQTT carry canonical CAN frames even though MCP2515 hardware is not installed.
- Each CAN payload ends with CRC8 and includes a 4-bit alive counter.
- UART adds framing, protocol version, and CRC16; MQTT uses candump text such as `300#...`.

## Motor Electrical Safety

- The L298 motor supply should be separate from logic power where possible, while retaining a common signal ground.
- Fit a 100 nF ceramic suppression capacitor directly across the motor terminals.
- Fit 470-1000 uF bulk capacitance close to the L298 motor-supply input.
- Keep motor leads short and twisted, and route them away from UART and potentiometer wiring.
- Firmware mitigation is secondary: the Uno command watchdog stops the motor after 350 ms without a valid Pi command.
