# C++ VCU Backend

This directory contains the Raspberry Pi VCU backend only. It has no Qt Quick,
QML, graphics, assets, or display code. `VehicleModel`, `DTCManager`, and
`VCUStateMachine` are normal C++ classes. Qt is kept only at the UART, MQTT,
simulation, timer, and test boundaries.

## Data Flow

```text
Arduino Uno ECU -> UART -> SerialManager -> VCUStateMachine -> VehicleModel
                                      \-> DTCManager
VCUStateMachine -> SerialManager -> Arduino Uno motor PWM command
ESP32 keypad -> Wi-Fi/MQTT -> SteeringInputManager -> backend command log
SimulationManager --------------------^ (replaces UART in simulation mode)
```

## Module Ownership

- `CanDatabase`: pack/unpack the six messages defined by the DBC.
- `Protocol.h`: declares the binary UART envelope and parser.
- `Protocol.cpp`: implements CRC16, encoding, and byte-stream parsing.
- `SerialManager`: CH340 connection, reconnect, RX parsing, and motor-command heartbeat.
- `VCUStateMachine`: OFF/ACC/READY/CHARGING/FAULT transitions and PWM authorization.
- `DTCManager`: raw and latched diagnostic trouble codes.
- `VehicleModel`: small container for current inputs, state, PWM, and UART counters.
- `SimulationManager`: deterministic telemetry source for tests without hardware.
- `SteeringInputManager`: MQTT candump parsing, counter validation, and keypad events.

The standalone C application in `../c_core/` is the readable reference core.
Do not run it together with this backend because only one process may own the
CH340 serial port.

New readers should start with `../../docs/CODING_GUIDE.md`. It provides a
recommended reading order and separates core logic from Qt and MQTT details.

The source of truth is `../../can_database/ipc_virtual_can.dbc`. The C core,
Uno, ESP32, and this backend use the same IDs, byte positions, CRC8, and alive
counters.

## UART Contract

```text
AA 55 | version | ID_H | ID_L | DLC | data[0..DLC-1] | CRC16_H CRC16_L
```

The UART CRC is CRC16-CCITT over `version`, ID, DLC, and CAN data. The CAN data
also contains the DBC CRC8 in its final byte. The hardware baud rate is fixed at
`9600`.

## Build And Test

```bash
cmake -S . -B build-pi -G Ninja -DBUILD_TESTING=ON
cmake --build build-pi
ctest --test-dir build-pi --output-on-failure
```

Required Qt modules are `Qt5Core`, `Qt5SerialPort`, and `Qt5Test` for tests.
MQTT support is enabled when `libmosquitto-dev` is available.

## Run Simulation

```bash
./build-pi/ipc_vcu_backend --simulate --no-mqtt
```

The backend prints vehicle state once per second. Stop it with `Ctrl+C`.
Use `--smoke-test` to exit automatically after 1.5 seconds.

## Run With Hardware

```bash
export IPC_MQTT_HOST=127.0.0.1
export IPC_MQTT_PORT=1883
export IPC_MQTT_USER=ipc_qt
export IPC_MQTT_PASSWORD='<mqtt-password>'

./build-pi/ipc_vcu_backend \
  /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
```

Use `--no-mqtt` when testing only the Uno and UART path.

## Safety Behavior

- Missing valid telemetry keeps PWM at zero.
- Reverse authorization is capped at PWM 43, representing 20 km/h.
- Any active or latched critical DTC forces PWM to zero.
- ECU communication timeout latches DTC `0xE1`.
- The Pi sends motor authorization every 100 ms.
- The Uno stops the motor after 350 ms without a valid command and reports `0xE2`.
- DTC clear is accepted only in Park with pedal and speed at zero after the raw fault disappears.
- MQTT key `D` requests the same safe DTC-clear path; all other keypad events are logged only.

ODO, Trip and DTE are handled by the cluster's UI/metrics module, not this
standalone terminal backend. ODO and Trip persistence are implemented in the
cluster; display-settings persistence is a future extension.
