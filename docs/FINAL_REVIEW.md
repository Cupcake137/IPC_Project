# Release Validation

Date: 2026-10-08.

## Focused Corrections

- UART motor authorization now requires a valid ECU_DRIVE_STATUS payload with
  a changed alive counter. Duplicate Drive frames, bad payload CRC, unrelated
  frames and silence cannot refresh the 500 ms watchdog. The watchdog is polled
  every 100 ms. Expiry clears cached PWM and reports the existing U0100 fault.
- The serial command sender also forces PWM to zero while communication is
  unhealthy. Critical faults still require the existing safe-clear procedure;
  reconnecting does not automatically restart the motor.
- The C reference permits the low-SOC warning 0x11 with PWM capped at 80,
  while preserving the Reverse cap of 43 and critical-fault stop behavior.
  The reference is not a replacement for the production backend's DTC latch.
- CMake is the supported HMI build path. The obsolete qmake demo project was
  removed instead of maintaining a second, incomplete build configuration.
- Dependency notices identify 256dpi/MQTT and libmosquitto correctly.

No QML, artwork, Uno firmware, ESP32 firmware or hardware protocol was changed.

## Regression Coverage

`ipc_serial_tests` uses a local pseudo-terminal on macOS/Linux and the actual
SerialManager, parser and state machine. It verifies continued operation with
fresh Drive telemetry and zero outgoing PWM after timeout for duplicate frames,
bad payload CRC, Energy-only traffic, unknown IDs and silence.

The C simulation checks low-SOC limited Drive output, enabled torque, the
Reverse cap and critical-fault shutdown. Existing backend and HMI tests remain
part of the validation procedure.

## Public Release Checklist

Local validation after the correction: C reference simulation PASS, backend
build and both CTest targets PASS, HMI build and all three CTest targets PASS.
Firmware was unchanged and was not rebuilt in this correction pass.

- The maintainer subsequently confirmed stable operation of the updated project on
  the Pi. Extended acceptance tests remain a repeatable checklist rather than
  independently witnessed evidence. Neither board needs reflashing for this correction.
- Verify retained upstream HMI code and artwork redistribution permission and
  retain the applicable license texts. THIRD_PARTY_NOTICES.md records unresolved
  provenance; this review does not grant permission or replace approved artwork.
- LICENSE.md records the no-blanket-license policy for this portfolio handover.
  Review staged files for secrets and use the hardware demo link in README.md.

The final source and demo are published in the `v1.0.0` release. Setup and
release procedures are described in GITHUB_RELEASE.md.
