# IPC Cluster Integration

This folder contains the current Qt/QML cluster and its connection to the IPC
C++ backend. It is the active UI, not a separate draft.

## Data flow

- Uno sends vehicle CAN-like frames over UART.
- `SerialManager` receives the frames.
- `VCUStateMachine` validates data and calculates the safe motor command.
- `BackendBridge` exposes simple values to QML.
- The original QML gauge displays speed, SOC, gear, estimated range and trip.
- The left vehicle image is decorative. No ADAS or lane signal is claimed.
- ESP32 keypad events arrive over MQTT and control the cluster menu.

## Display calculations

- ODO and Trip A independently integrate PWM-derived speed. Both start at zero
  on a new installation; resetting Trip A never changes ODO.
- A speed sample expires after 500 ms. Lost communication, duplicate frames or
  invalid drive frames cannot keep distance accumulating indefinitely.
- Hardware mode saves distance and moving time every five seconds, on Trip
  reset and on normal shutdown. Data lives in `distance.ini` under Qt's
  AppDataLocation (`~/.local/share/IPCProject/IPCCluster` on a typical Pi).
  Sudden power loss can lose the unsaved interval; this is not a certified odometer.
- Simulation uses temporary in-memory distance only and does not read/write
  hardware distance. DRIVE minutes count fresh positive-speed intervals.
- `EST. RANGE` uses SOC, drive mode and motor command. It is an estimate because
  this prototype does not measure battery voltage, current or real energy use.
- `POWER` is the authorized PWM command shown as a percentage. It is not kW.
- `REGEN 0-3` is a selectable prototype setting. Actual regenerative braking
  requires brake input and suitable motor/power electronics.

## Warning levels

- Yellow: non-critical warning. The vehicle may continue with limited output.
- Red: critical DTC. The VCU stops the authorized motor command.
- `P0A80` is treated as a non-critical low-voltage warning in this project.
- Every other known or unknown DTC remains critical by default.

The cluster and terminal backend compile the same DTC/state-machine sources
from `vcu/cpp_backend`. Low battery limits PWM to 80; Reverse remains capped
at 43. Critical faults cannot be replaced by a later low-battery warning.

## Keypad controls

| Key | Action |
| --- | --- |
| `A` | Open/close menu details |
| `B` | Close menu details |
| `C` | Return to Vehicle page |
| `2`, `8` | While menu is open: adjust regen on Vehicle, brightness on Display |
| `4`, `6` | While menu is open: previous/next page |
| `5` | While menu is open: request safe Trip reset on Trip; close menu |
| `1` | Cycle position/low-beam light display states |
| `3` | Toggle high-beam display state |
| `7` | Toggle fog-light display state |
| `9` | Cycle ECO/NORMAL/SPORT motor authorization mode |
| `*`, `#` | Toggle left/right turn-signal display states |
| `0` | Toggle hazard display state |
| `D` | Acknowledge and request a safe DTC clear |

The UI and the terminal backend must not open the same Uno serial port at the
same time.

Pages are Vehicle, Energy, Trip, Warnings and Display. Startup shows the compact
Trip summary with menu details closed. A opens the last selected page; B closes
it, and C selects Vehicle then closes it. Closing the menu does not disable
lights, signals, drive-mode selection or DTC acknowledgement. Navigation and
setting adjustments are ignored while closed. Clicking a menu icon opens its
page; clicking the warning telltale opens Warnings. There is no hidden Service
key sequence. Light controls affect the display, not physical lamps.
Trip reset requires fresh telemetry, Park, zero pedal and zero speed. DTC clear
is handled through the backend safety gate.

## Build on macOS

```bash
cmake -S . -B build-mac -G Ninja \
  -DQt5_DIR="$(brew --prefix qt@5)/lib/cmake/Qt5"
cmake --build build-mac
```

Run with simulated vehicle data and no MQTT:

```bash
./build-mac/ipc_cluster --simulate --no-mqtt
```

Run simulation with the ESP32 connected to the broker on the Pi:

```bash
export IPC_MQTT_HOST=192.168.1.75
export IPC_MQTT_PORT=1883
export IPC_MQTT_USER=ipc_qt
export IPC_MQTT_PASSWORD='<mqtt-password>'
./build-mac/ipc_cluster --simulate
```

## Build and run on Raspberry Pi

```bash
cmake -S . -B build-pi -G Ninja
cmake --build build-pi
```

With a desktop display:

```bash
export IPC_MQTT_HOST=127.0.0.1
export IPC_MQTT_PORT=1883
export IPC_MQTT_USER=ipc_qt
export IPC_MQTT_PASSWORD='<mqtt-password>'
./build-pi/ipc_cluster \
  /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
```

For a local simulation, add `--simulate`. For testing without the ESP32, add
`--no-mqtt`.
