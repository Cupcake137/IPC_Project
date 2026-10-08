# IPC Hardware Bring-Up

This guide starts the current Qt/QML cluster on Raspberry Pi 3B+, with the
Arduino Uno motor ECU and ESP32 keypad. The cluster embeds the C++ backend.
Run either the cluster or the terminal backend, never both on the same UART.

## 1. Electrical Preflight

1. Keep the L298 motor supply off and place the Uno in Park with the pedal at zero.
2. Confirm Uno and L298 share ground; do not power the motor from the Uno 5 V pin.
3. Confirm the level shifter uses 3.3 V on the CH340/Pi side and 5 V on the Uno side.
4. Fit 100 nF directly across the motor and 470-1000 uF near the L298 motor input.
5. Confirm ESP32 keypad pins match `firmware/esp32_keypad_controller/README.md`; do not hold a key during boot.

## 2. Flash The Controllers From The Mac

List serial ports, then upload with the matching device names:

```bash
cd IPC_Project
pio device list
pio run --project-dir firmware/arduino_motor_ecu -t upload --upload-port /dev/cu.usbmodemXXXX
pio run --project-dir firmware/esp32_keypad_controller -t upload --upload-port /dev/cu.usbserialXXXX
```

The ESP32 build uses the ignored `firmware/esp32_keypad_controller/include/secrets.h`. Never transfer
or commit this file.

## 3. Prepare The Pi Source

Copy the source repository to the Pi, excluding local secrets and build outputs.
The examples below use `~/Documents/My_project/IPC_Project` as the checkout
directory. Adjust this path for your installation and build locally on the Pi.

## 4. Build And Check The Pi

```bash
cd ~/Documents/My_project/IPC_Project
make -C vcu/c_core clean test

cmake -S vcu/cpp_backend -B vcu/cpp_backend/build-pi \
  -G Ninja -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build vcu/cpp_backend/build-pi --parallel 1
ctest --test-dir vcu/cpp_backend/build-pi --output-on-failure

cmake -S hmi/cluster_ui -B hmi/cluster_ui/build-pi \
  -G Ninja -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build hmi/cluster_ui/build-pi --parallel 1
ctest --test-dir hmi/cluster_ui/build-pi --output-on-failure

./vcu/cpp_backend/build-pi/ipc_vcu_backend --simulate --no-mqtt --smoke-test
```

All tests and the smoke test must pass before enabling the motor supply.
Single-job compilation reduces memory pressure on the Pi 3B+. Run the GUI
from a terminal in the Pi desktop/VNC session, not a plain SSH session.
To preview without hardware, run `./hmi/cluster_ui/build-pi/ipc_cluster --simulate --no-mqtt`.

## 5. Start The Complete Hardware Path

Power the Uno, Pi, and ESP32 while leaving the motor supply off. On the Pi:

```bash
sudo systemctl restart mosquitto
sudo systemctl --no-pager --full status mosquitto
ls -l /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0

export IPC_MQTT_HOST=127.0.0.1
export IPC_MQTT_PORT=1883
export IPC_MQTT_USER=ipc_qt
export IPC_MQTT_PASSWORD='<mqtt-password>'

cd ~/Documents/My_project/IPC_Project
unset QT_QPA_PLATFORM QT_QUICK_BACKEND
./hmi/cluster_ui/build-pi/ipc_cluster \
  /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
```

For terminal-only debugging through SSH, stop the cluster first and run this
instead, after exporting the same MQTT variables in that terminal:

```bash
cd ~/Documents/My_project/IPC_Project
./vcu/cpp_backend/build-pi/ipc_vcu_backend \
  /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
```

Confirm valid telemetry, Park, zero pedal and zero PWM, and test a keypad page
change before enabling the motor supply. Terminal mode reports broker/controller
status and vehicle telemetry in its log. Turn the pedal back to zero before
changing gear. Stop the application with Ctrl+C before changing wiring.

## 6. Acceptance Sequence

1. In Park and Neutral, sweep the potentiometer; motor PWM must remain zero.
2. Select Drive with zero pedal, then raise the pedal slowly; motor turns forward.
3. Release the pedal, select Reverse, and raise it slowly; direction reverses and speed stays at or below 20 km/h.
4. Press A to open the menu, use 4/6 for pages and 2/8 for settings. B closes details; C closes details and returns the selected page to Vehicle. While closed, navigation/settings keys must not change values.
5. Use `4`/`6` to reach Display, then `2`/`8` to adjust brightness. There is no hidden Service sequence in the current UI. `0` toggles the hazard display, and `9` cycles drive mode.
6. While turning slowly with the motor unloaded, stop the Qt process. Uno must stop the motor within 350 ms and report DTC `0xE2`.
7. Restart the application, return to Park at zero pedal/speed, and press `D` to request a safe latched-DTC clear. View details on Warnings.
8. Finish with the 30-minute observation test from `PROJECT_VALIDATION.md`.

Do not run `vcu/c_core/bin/vcu_powertrain` during these tests. The C++ backend is
the active VCU. Either it or the cluster must be the only CH340 UART owner.

## 7. Common Startup Problems

- `incomplete MQTT configuration`: export `IPC_MQTT_PASSWORD` in the terminal
  that launches the application; confirm the ESP32 uses the same broker.
- `not authorised`: check the broker username/password and topic permissions.
  Do not include placeholder angle brackets in the actual password.
- UART unavailable: check the by-id path, `dialout` membership and other
  processes owning the port. Do not run a second backend on that device.
- CRC16 rejection: one startup fragment can be rejected; repeated rejection
  needs firmware/protocol, baud rate, wiring and electrical-noise investigation.
- Runtime-directory permission warning is separate from UART/MQTT errors.
  Do not fix it by running the cluster as root.

See `hmi/cluster_ui/HMI_INTEGRATION.md` for the complete current key map and
the distinction between hardware telemetry and display-only controls.
