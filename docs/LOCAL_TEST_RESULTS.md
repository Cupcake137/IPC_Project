# Validation Results

Release: 1.0.0. Validation dates: 2026-10-07 and 2026-10-08.

## Software Validation

| Check | Result | Scope |
| --- | --- | --- |
| C reference build and simulation | PASS | Shared CAN contract, low-SOC limited torque, Reverse cap and critical stop |
| C++ backend build and tests | PASS | 2 CTest targets, including actual serial parser/state-machine integration |
| UART regression scenarios | PASS | Fresh Drive, duplicate Drive, bad payload CRC, Energy-only traffic, unknown ID and silence |
| HMI build and tests | PASS | 3 CTest targets: menu, distance and cluster state |
| Terminal simulation startup | PASS | Bounded simulation run exits with status 0 |
| QML startup | PASS | Offscreen simulation starts/exits without QML errors |
| Shell script syntax | PASS | bash -n on build/test, sync, hardware and package scripts |
| GitHub Linux CI | PASS | Ubuntu 24.04: C reference, backend, HMI/startup and shell checks |

macOS builds use Qt 5.15 and CMake/Ninja. The UART tests use a pseudo-terminal
and verify outgoing zero-PWM frames after the freshness timeout, not only a
model property. Continued fresh Drive telemetry retains normal motor authorization.

Offscreen startup is not a visual screenshot test. The hardware demo separately
shows the visible cluster and assembled prototype.

## Firmware Build Record

| Target | Result | Scope |
| --- | --- | --- |
| ESP32 | PASS | esp32dev, espressif32 7.0.1, MQTT 2.5.3; example credentials |
| Uno on the recorded Mac toolchain | HOST TOOLCHAIN LIMITATION | Installed x86_64 AVR compiler could not run on the host |

The recorded ESP32 build used 45016/327680 bytes RAM and 753433/1310720 bytes
flash. This does not validate Wi-Fi credentials or live MQTT. The Uno Mac result
is not a successful firmware compilation; build it on a compatible toolchain.
The final watchdog correction changes the Pi backend, not either board's firmware.

## Hardware Validation

The maintainer confirmed stable functional operation of the updated Pi setup
on 2026-10-08. This confirmation and the supplied hardware demo are distinct
from automated software tests. Exact load conditions, durations and flashed
firmware hashes were not recorded for every extended acceptance item.

Use RELEASE_ACCEPTANCE.md to repeat and record the full checklist. Real motor
noise, sustained reconnect behavior and long-duration reliability require
documented hardware runs. An automated PASS is not an automotive safety claim.
