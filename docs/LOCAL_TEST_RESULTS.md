# Local Validation Results

Date: 2026-10-07. Source: IPC_Project after DTC, distance and menu updates.
Validation was performed on macOS. No Pi connection, synchronization, firmware
upload or motor operation was performed during this pass.

## Results

| Check | Result | Evidence / scope |
| --- | --- | --- |
| C reference build and virtual-CAN simulation | PASS | make clean test in isolated source copy |
| C++ backend build | PASS | Qt 5.15 CMake/Ninja build |
| Backend tests | PASS | 9 test functions; 1 CTest target |
| Terminal simulation smoke test | PASS | Process exited with status 0; this is a short startup check |
| HMI build | PASS | Qt/QML cluster links with shared DTC/state-machine sources |
| Menu tests | PASS | Actual MenuController.qml with a fake vehicle object |
| Distance tests | PASS | Fresh interval, disconnect/reconnect, Trip reset, save/reload and moving time |
| HMI warning/state tests | PASS | Low-SOC limited output and critical motor stop |
| HMI CTest suite | PASS | All 3 targets passed |
| QML startup | PASS | Offscreen simulation starts/exits without QML errors |
| Visual screenshot verification | NOT VERIFIED | Offscreen grab did not produce an image |
| ESP32 firmware build | PASS | esp32dev, espressif32 7.0.1, MQTT 2.5.3; example credentials only |
| Uno firmware build | TOOLCHAIN BLOCKED | Installed AVR compiler is x86_64; host reports Bad CPU type in executable |
| Shell script syntax | PASS | bash -n on local-test, sync, hardware, soak, package and deploy scripts |
| Updated Pi hardware acceptance | USER VALIDATION | Use RELEASE_ACCEPTANCE.md after local synchronization |

ESP32 build used 45016/327680 bytes RAM and 753433/1310720 bytes application
flash in this environment. This does not validate Wi-Fi credentials or live MQTT.
Uno did not reach compilation: no conclusion about its compilation correctness
can be drawn from this failure. No Rosetta installation or toolchain replacement
was attempted. Build Uno on a compatible configured machine before release.

Temporary build paths are not part of the project or required on another machine:

- /private/tmp/ipc-release-local-20261007: firmware and C reference source copy.
- /private/tmp/ipc-dtc-backend-verified: backend build and tests.
- /private/tmp/ipc-dtc-hmi-verified: HMI build and tests.

## Coverage Boundaries

### 2026-10-08 Watchdog Correction

Fresh builds after the focused correction passed on macOS:

- C reference: `make clean test`, including low-SOC limited torque and Reverse cap.
- Backend: both CTest targets passed (`ipc_vcu_tests`, `ipc_serial_tests`).
- Serial regression: six pseudo-terminal scenarios passed: fresh Drive,
  duplicate Drive, bad payload CRC, Energy-only traffic, unknown ID and silence.
  The five stale/invalid-input scenarios verified outgoing zero-PWM commands
  after timeout, not only the model property.
- HMI: build and all three CTest targets passed (menu, distance, cluster state).
- Terminal simulation startup passed. QML offscreen startup exited normally;
  the screenshot option is used to bound runtime, not as visual validation.

No QML/artwork or board firmware changes were made. This pass did not connect
to the Pi, flash boards, validate physical motor behavior or resolve asset rights.
See [Final Source Review](FINAL_REVIEW.md) for the correction scope.

Menu tests cover controller actions, not all rendered components or physical
keypad input. Distance tests cover the calculation/storage helper; UART-to-HMI
timing, reset interlocks and real filesystem shutdown must also be checked on
the Pi. Backend tests include CRC rejection, duplicate-counter rejection,
warning/critical latching, safe clear, Reverse limit and keypad payload parsing.
They do not demonstrate a sustained UART/MQTT reconnect or motor-noise run.

Automated PASS is not a hardware reliability or automotive safety claim.
