# IPC Automotive Project Validation

This checklist validates the Arduino ECU, VCU core, Qt application, UART integration, and hardware safety behavior. Run the relevant checks after changes to firmware, protocol, backend logic, or wiring.

## 1. Communication Baseline

Confirm the implementation matches the project baseline:

- Arduino SoftwareSerial uses D2 RX and D3 TX at 9600 baud.
- Arduino USB debug uses 115200 baud.
- The Pi uses the stable CH340 path under `/dev/serial/by-id/`.
- Both boards use the same frame IDs, payload lengths, counter, and checksum rules.

## 2. Arduino ECU Build

```bash
cd IPC_ECU_uno
pio run
```

The build must finish without project-source errors and remain within Arduino Uno flash and RAM limits.

## 3. VCU Core Build

```bash
cd Pi3B+_VCU
make clean all
```

The build must generate `bin/vcu_powertrain` without compiler errors.

## 4. Ubuntu VM Qt Build And Tests

```bash
cd Pi3B+_VCU/phase3_qt_backend
cmake -S . -B build-vm -G Ninja -DBUILD_TESTING=ON
cmake --build build-vm
ctest --test-dir build-vm --output-on-failure
```

Expected result:

```text
100% tests passed, 0 tests failed
```

## 5. Qt Simulation

```bash
./build-vm/ipc_phase3_backend --simulate
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

## 6. Raspberry Pi Qt Build And Tests

```bash
cd Pi3B+_VCU/phase3_qt_backend
cmake -S . -B build-pi -G Ninja -DBUILD_TESTING=ON
cmake --build build-pi
ctest --test-dir build-pi --output-on-failure
```

## 7. Integrated Hardware Test

Run with the stable CH340 path:

```bash
./build-pi/ipc_phase3_backend \
  /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
```

Verify:

- Uno gear changes are reflected by the Qt model/UI.
- Park and Neutral always command zero PWM.
- Drive uses the forward L298 direction.
- Reverse uses the opposite L298 direction and limited PWM.
- ECO, NORMAL, and SPORT produce different authorized PWM values.
- An ECU DTC enters FAULT and commands zero PWM.

## 8. Communication Safety Test

Perform this test with the motor unloaded or L298 ENA disconnected:

1. Start in READY and confirm valid telemetry.
2. Disconnect the CH340 USB-UART.
3. Confirm `NO ECU DATA`, DTC `U0100`, FAULT, and PWM zero within one second.
4. Reconnect the CH340 and confirm that the process stays alive and reopens the port.
5. Confirm the DTC remains latched.
6. Select Park, release the pedal, wait for speed zero, and clear the DTC.
7. Confirm the system returns to READY.

## 9. Soak Test

Run hardware mode for at least 30 minutes. Acceptance requires:

- No process crash or frozen UI.
- No unintended motor command in Park, Neutral, Fault, Charging, or communication loss.
- Checksum and dropped-frame counters remain understandable and do not rise continuously under normal wiring conditions.
- USB-UART reconnect works without restarting the application.

## 10. Current Project Status

The current UART-based ECU/VCU and Qt backend are validated when all applicable checks above pass. ODO/Trip persistence, DTE blending, the VM-to-Pi cross-compilation sysroot, final UI design, and MCP2515 CAN migration remain separate work items.
