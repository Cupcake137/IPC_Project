# IPC Automotive Project

Automotive IPC/VCU prototype using an Arduino Uno as the ECU and a Raspberry Pi 3B+ as the VCU and Qt display controller.

## Repository layout

- `IPC_ECU_uno/`: PlatformIO firmware for the Arduino Uno.
- `Pi3B+_VCU/`: C implementation of the VCU powertrain core.
- `Pi3B+_VCU/phase3_qt_backend/`: Qt 5/C++17 backend, simulation mode, QML UI, and tests.
- [`docs/PROJECT_HARDWARE_BASELINE.md`](docs/PROJECT_HARDWARE_BASELINE.md): verified hardware and wiring baseline.
- [`docs/PROJECT_VALIDATION.md`](docs/PROJECT_VALIDATION.md): build, simulation, hardware, and safety validation checklist.

## Communication

- Transport: CH340 USB-UART through a 3.3 V/5 V level shifter.
- Protocol baud rate: 9600.
- Arduino debug baud rate: 115200.
- Arduino SoftwareSerial pins: D2 RX and D3 TX.

## Build Arduino Uno

```bash
cd IPC_ECU_uno
pio run
```

## Build Phase 2 VCU core

```bash
cd Pi3B+_VCU
make
```

## Build Phase 3 on Raspberry Pi

```bash
cd Pi3B+_VCU/phase3_qt_backend
cmake -S . -B build-pi -G Ninja -DBUILD_TESTING=ON
cmake --build build-pi
ctest --test-dir build-pi --output-on-failure
```

Run with hardware:

```bash
./build-pi/ipc_phase3_backend /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
```

Run the simulation without a display:

```bash
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
  ./build-pi/ipc_phase3_backend --simulate
```

## Current scope

Phase 3 includes serial framing and validation, reconnect and communication timeout handling, the vehicle model, the VCU state machine, DTC management, simulation mode, QML integration, and a reverse authorization limit representing 20 km/h. ODO/Trip persistence, DTE blending, a true cross-compilation sysroot, and MCP2515 CAN migration remain in progress.
