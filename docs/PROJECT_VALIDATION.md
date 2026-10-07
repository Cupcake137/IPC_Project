# IPC Automotive Project Validation

This checklist validates the Arduino ECU, VCU core, Qt application, UART integration, and hardware safety behavior. Run the relevant checks after changes to firmware, protocol, backend logic, or wiring.

## Automated Local Preflight

From the repository root on macOS or Ubuntu:

```bash
./tools/test-project-local.sh
```

This command builds both firmware targets, the C reference, C++ backend and HMI,
then runs their tests and a terminal smoke test. It does not replace hardware tests.

## 1. Communication Baseline

Confirm the implementation matches the project baseline:

- Arduino SoftwareSerial uses D2 RX and D3 TX at 9600 baud.
- Arduino USB debug uses 115200 baud.
- The Pi uses the stable CH340 path under `/dev/serial/by-id/`.
- All modules use the IDs and payloads in `can_database/ipc_virtual_can.dbc`.
- The ESP32 and backend use the same broker credentials and `ipc/can/swc` topic.
- UART frames use protocol version 1 and CRC16; each CAN payload also validates CRC8.

## 2. Arduino ECU Build

```bash
cd firmware/arduino_motor_ecu
pio run
```

The build must finish without project-source errors and remain within Arduino Uno flash and RAM limits.

## 3. ESP32 Steering Controller Build

```bash
cp firmware/esp32_keypad_controller/include/secrets.example.h \
  firmware/esp32_keypad_controller/include/secrets.h
# Fill in the local Wi-Fi and MQTT values, then:
pio run --project-dir firmware/esp32_keypad_controller
```

Confirm all 16 keypad contacts publish one `0x300` press frame and one `0x300`
release frame. A held key also publishes long press. Do not press a key while
the ESP32 is booting because GPIO2 and GPIO15 are boot-strapping pins.

## 4. VCU Reference Core Build

```bash
cd vcu/c_core
make clean all
```

The build must generate `bin/vcu_powertrain` without compiler errors. This executable is a reference harness and must not run alongside the Qt VCU on the same serial device.

## 5. Ubuntu VM Qt Build And Tests

```bash
cd vcu/cpp_backend
cmake -S . -B build-vm -G Ninja -DBUILD_TESTING=ON
cmake --build build-vm
ctest --test-dir build-vm --output-on-failure
```

Expected result:

```text
100% tests passed, 0 tests failed
```

## 6. Qt Simulation

```bash
./build-vm/ipc_vcu_backend --simulate
```

Observe at least one complete 20-second cycle:

- OFF exists before the transport is initialized.
- ACC exists while the transport is open but no telemetry has arrived.
- READY authorizes PWM only in Reverse or Drive.
- Reverse PWM never exceeds 43, corresponding to 20 km/h in the current simulated speed model.
- DTC 0x22 enters FAULT and immediately sets authorized PWM to zero.
- DTC remains latched after the raw DTC disappears.
- The simulator clears the DTC only in Park at zero pedal and speed.
- CHARGING keeps authorized PWM at zero.

## 7. Raspberry Pi Qt Build And Tests

```bash
cd vcu/cpp_backend
cmake -S . -B build-pi -G Ninja -DBUILD_TESTING=ON
cmake --build build-pi
ctest --test-dir build-pi --output-on-failure
```

Alternatively, package and deploy the ARM64 VM build:

```bash
cd vcu/cpp_backend
./tools/build-package-arm64.sh
./tools/deploy-to-pi.sh cupcake@RasberryPi3B
```

The deploy script must pass both the backend unit tests and the terminal smoke test on the Pi.

## 8. UI Metrics Scope

The current HMI displays ODO, Trip and estimated range. These are prototype
calculations, not calibrated measurements. Distance is persisted in hardware
mode with independent ODO/Trip; settings persistence remains future work.
See HMI_INTEGRATION.md for the persistence interval and fresh-speed rules.

## 9. Integrated Hardware Test

Start Mosquitto, power the ESP32, and run with the stable CH340 path:

```bash
export IPC_MQTT_PASSWORD='<mqtt-password>'
vcu/cpp_backend/tools/run-hardware.sh
```

Verify:

- Uno gear changes are reflected by the backend vehicle model and telemetry log.
- Park and Neutral always command zero PWM.
- Drive uses the forward L298 direction.
- Reverse uses the opposite L298 direction and limited PWM.
- ECO, NORMAL, and SPORT produce different authorized PWM values.
- An ECU DTC enters FAULT and commands zero PWM.
- ESP32 keypad events arrive once and preserve sequence order.
- Key `D` requests DTC clear only through the normal Park/pedal/speed safety gate.

## 10. Communication Safety Test

Perform this test with the motor unloaded or L298 ENA disconnected:

1. Start in READY and confirm valid telemetry.
2. While the motor is turning slowly, stop the Qt process with `Ctrl+C`. Confirm the Uno stops the motor within 350 ms.
3. Restart the Qt process, then disconnect the CH340 USB-UART.
4. Confirm the Uno stops the motor within 350 ms and reports DTC `0xE2` on its USB debug log.
5. Confirm the Pi reports `NO ECU DATA`, DTC `U0100`, FAULT, and PWM zero within one second.
6. Reconnect the CH340 and confirm that the process stays alive and reopens the port.
7. Confirm the DTC remains latched.
8. Select Park, release the pedal, wait for speed zero, and press D in the HMI to request a safe DTC clear. There is no hidden Service menu in the current UI.
9. Confirm the system returns to READY.

## 11. Motor Noise And Soak Test

Before the test, use a separate motor supply where possible, retain the common signal ground, add local bulk decoupling near the L298 supply, and fit a small ceramic suppression capacitor directly across the motor terminals. Keep motor wiring short and twisted away from UART wiring.

Run hardware mode for at least 30 minutes, including repeated high-PWM acceleration. Acceptance requires:

```bash
./vcu/cpp_backend/tools/run-hardware-soak.sh \
  ./vcu/cpp_backend/build-pi/ipc_vcu_backend \
  /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0 1800
```

- No backend crash or stalled telemetry processing.
- No Arduino reset, serial disconnect, or corrupted gear command at high PWM.
- No unintended motor command in Park, Neutral, Fault, Charging, or communication loss.
- The backend does not continuously report `invalid UART frame rejected by CRC16` under normal wiring conditions.
- USB-UART reconnect works without restarting the application.

## 12. Project Hardware Acceptance

The VCU backend is complete only when:

- Arduino, ESP32, VCU reference core, C++ VCU backend, and automated tests build successfully.
- The terminal backend smoke test passes on both the VM and Pi.
- UART hardware and communication safety tests pass.
- MQTT keypad navigation and safety gating pass.
- The 30-minute motor-noise soak test passes without an Uno reset.

Physical CAN migration is future work, separate from this UART/MQTT prototype.
The soak script checks logged faults; a PASS does not independently detect
every MCU reset or certify electrical reliability. Observe Uno debug logs too.
