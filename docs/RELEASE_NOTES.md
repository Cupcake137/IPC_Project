# Version 1.0.0 - 2026-10-08

## Handover

The project maintainer confirmed stable functional operation on the Raspberry Pi
hardware setup on 2026-10-08 after the final UART watchdog correction.
This is overall functional acceptance, not evidence that every extended
reliability check in RELEASE_ACCEPTANCE.md was completed. Exact load conditions,
run durations and firmware hashes were not recorded for every checklist item.

The final source is published on `main` and tagged `v1.0.0`. The release contains
the hardware demo and a source archive with a SHA-256 checksum. Use this tag for
the final snapshot; earlier local archives predate the watchdog correction.

## Included

- Uno motor ECU and ESP32 keypad firmware source.
- Shared CAN database and UART/MQTT frame definitions.
- C reference core, C++ backend and automated tests.
- Approved Qt/QML HMI, menu controls and independent ODO/Trip persistence.
- English build, hardware, data-scope and validation documentation.

## Excluded

Git history, compiler outputs, PlatformIO caches, editor settings, local logs,
private credentials, checkpoints and previous design drafts are not release files.
The source includes an example ESP32 configuration, not the owner's secrets.h.
Runtime ODO/Trip data is not included; source packaging does not reset the data
already stored on the Pi.

## Restore And Build

Extract the archive into a new directory and follow HARDWARE_BRINGUP.md. Configure
ESP32 credentials locally from include/secrets.example.h. Configure Pi MQTT
environment variables locally. Build binaries for the destination platform; this
source archive is not a ready-to-run Raspberry Pi binary distribution.

Do not launch the terminal backend and HMI simultaneously on the same UART. Keep
motor power off during flashing and initial communication checks.

## Scope And Future Development

This educational prototype uses CAN-shaped frames over UART/MQTT rather than
physical CAN. SOC and speed are modeled, not measured battery/encoder readings.
Lighting and regen controls are display demonstrations, not physical BCM outputs
or regenerative braking. See DATA_SCOPE.md for details.

Future development includes electrical-noise validation under documented motor
loads, real sensing, physical CAN transport and persistent display settings.
The historical Mac Uno toolchain failure remains recorded in LOCAL_TEST_RESULTS.md;
it is not evidence of a successful firmware compilation on that machine.

The handover records reference-code/artwork attribution and an explicit
no-blanket-license policy in ../LICENSE.md and ../THIRD_PARTY_NOTICES.md. It does
not grant rights to unverified third-party artwork. Rotate previously shared
credentials and review screenshots before publishing a portfolio demonstration.
