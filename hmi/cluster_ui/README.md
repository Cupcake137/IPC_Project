# IPC Qt HMI

This folder contains the Qt/QML instrument-cluster UI for the IPC project.
It reuses the existing C++ backend, simulated CAN frames, Uno UART link, and
ESP32 MQTT keypad link.

## Simple architecture

```text
Uno -> UART CAN-like frames -> C++ backend -> BackendBridge -> QML
ESP32 keypad -> MQTT CAN-like frames ------^                 |
                                                             v
                                                        IPC display
```

The QML files display backend properties and forward mouse actions to the
backend. Motor authorization, DTC handling, UART parsing, keypad decoding,
control state, and the CAN database stay in C++.

## Hardware interaction scope

- Uno telemetry supplies gear, modeled speed, pedal input, SOC and raw faults.
- The Pi derives DTE and maintains independent ODO/Trip metrics from fresh speed frames.
- The backend sends the authorized motor PWM and direction back to the Uno.
- ESP32 keypad CAN-like frames control the menu, drive mode, regen, brightness,
  lights, high beam, fog light, indicators, hazard lights, and DTC acknowledge.
- Mouse clicks on telltales call the same backend functions as keypad commands.
- This prototype has no physical lamp outputs. Light and indicator commands
  therefore update the IPC state only. Adding real lamps later requires output
  hardware plus command frames for that hardware controller.

## Keypad map

| Key | Action |
| --- | --- |
| `1` | Cycle OFF, position light, low beam |
| `2` | Menu up or increase current setting |
| `3` | Toggle high beam |
| `4` | Previous menu page |
| `5` | Select; reset Trip A on the Trip page |
| `6` | Next menu page |
| `7` | Toggle front fog light |
| `8` | Menu down or decrease current setting |
| `9` | Cycle ECO, NORMAL, SPORT |
| `*` | Toggle left indicator |
| `0` | Toggle hazard lights |
| `#` | Toggle right indicator |
| `A` | Open or close the menu |
| `B` | Close the menu |
| `C` | Home: close the menu and select the Vehicle page |
| `D` | Acknowledge and request safe DTC clear |

Navigation and selection keys work only while the menu is open. Brightness is
changed with `2` and `8` on the Display page; regen level is changed on the
Vehicle page. Trip reset requires fresh connected telemetry, Park, zero speed
and zero pedal. The default driving screen starts with the menu closed.

## Build on macOS

Use `CMakeLists.txt` when opening this project in Qt Creator. The original
`Car_1.pro` demo configuration was removed because it did not include the
hardware backend. CMake is the single supported build configuration.

```bash
cmake -S . -B build-mac -G Ninja \
  -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5
cmake --build build-mac
ctest --test-dir build-mac --output-on-failure
./build-mac/ipc_cluster --simulate --no-mqtt
```

## Run with the ESP32 keypad

```bash
export IPC_MQTT_HOST=192.168.1.75
export IPC_MQTT_PORT=1883
export IPC_MQTT_USER=ipc_qt
export IPC_MQTT_PASSWORD='<mqtt-password>'
./build-mac/ipc_cluster --simulate
```

## Run on Raspberry Pi with Uno hardware

```bash
cmake -S . -B build-pi -G Ninja
cmake --build build-pi --parallel 1

export IPC_MQTT_HOST=127.0.0.1
export IPC_MQTT_PORT=1883
export IPC_MQTT_USER=ipc_qt
export IPC_MQTT_PASSWORD='<mqtt-password>'

./build-pi/ipc_cluster \
  /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
```

Use `QT_QPA_PLATFORM=offscreen` only for headless logic tests. A real display,
Wayland/X11 session, or VNC is required to see and operate the UI.

## QML files

- `main.qml`: screen state and keypad actions.
- `qml/MenuController.qml`: open/close state and menu key handling.
- `qml/HeaderBar.qml`: weather, clock, and date.
- `qml/TelltaleBar.qml`: lights, indicators, and warnings.
- `qml/GearPanel.qml`: PRND and the static top-view vehicle image.
- `qml/SpeedGauge.qml`: synchronized speed number and arc animation.
- `qml/InfoPanel.qml`: Vehicle, Energy, Trip, Warnings, and Display pages.
- `qml/FooterBar.qml`: odometer, drive mode, regen, and SOC.

## Icon assets

The existing icon set is documented as derived from Google Material Icons and
MaterialDesign. Exact per-file provenance still needs verification before
public redistribution; see [Third-Party Notices](../../THIRD_PARTY_NOTICES.md).
The current vehicle image is static and preserves its aspect ratio.

- Google Material Icons: https://github.com/google/material-design-icons
- MaterialDesign Icons: https://github.com/Templarian/MaterialDesign
