# IPC Phase 3 Qt Cluster

This folder contains the Qt/C++ backend and the first Qt Quick/QML instrument-cluster UI. It mirrors the Phase 2 UART contract and keeps UI-facing data in `VehicleModel`.

## Data Flow

```text
Arduino ECU -> SerialManager -> VCUStateMachine -> VehicleModel -> QML
                                DTCManager
VCUStateMachine -> SerialManager -> Arduino ECU motor PWM command
```

`SimulationManager` can replace `SerialManager` as the data source and motor-command sink. The state machine and model are identical in both modes.

## UART Contract

```text
AA 55 | ID_H | ID_L | DLC | counter | payload[0..DLC-1] | checksum
```

Checksum is the 8-bit sum of `ID_H + ID_L + DLC + counter + payload`.

Baudrate is fixed at `9600` for hardware bring-up.

## First Build On Raspberry Pi

```bash
cd phase3_qt_backend
cmake -S . -B build-pi -G Ninja
cmake --build build-pi
```

The first build is performed directly on the Pi to verify Qt 5.15, the UART protocol, and the hardware path before setting up the VM sysroot/toolchain.

## Run On Pi

```bash
./build-pi/ipc_phase3_backend \
  /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
```

The CH340 USB-UART adapter is the VCU channel. The Uno USB ACM/debug port is not used for this protocol.

## Run Without Hardware

On the Ubuntu VM or Pi:

```bash
./build-vm/ipc_phase3_backend --simulate
```

The command opens the Qt Quick dashboard and drives it with simulated vehicle data.

When built in `build-pi`, use:

```bash
./build-pi/ipc_phase3_backend --simulate
```

The 20-second scenario repeats through Park, Reverse, Neutral, Drive, and an injected motor-overload DTC. Stop it with `Ctrl+C`.

The simulator also clears the latched DTC under safe conditions and enters Charging before repeating.

## Automated Tests

```bash
cmake --build build-vm
ctest --test-dir build-vm --output-on-failure
```

The tests cover protocol encode/decode, checksum rejection, all VCU states, PWM authorization, latched DTC behavior, communication loss, safe DTC clearing, and Charging.

## Phase 3 Safety Behavior

- No valid telemetry: `ACC`, PWM is zero.
- Healthy telemetry: `READY`, PWM follows VCU authorization.
- Active or latched DTC: `FAULT`, PWM is zero.
- ECU data timeout over 500 ms: communication DTC `0xE1` is latched and PWM is zero.
- USB-UART removal: the backend remains running and retries the port every second.
- DTC clear is accepted only in Park with pedal and speed at zero, after the raw fault has disappeared.

## Build On Ubuntu VM

The VM has Qt 5.15.3 and can build the backend locally for development:

```bash
cmake -S . -B build-vm -G Ninja
cmake --build build-vm
```

Do not deploy the VM-linked executable as the final Pi binary. A Pi sysroot and CMake toolchain will be added after the direct Pi build passes.
