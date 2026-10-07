# Release Procedure

## Source Boundary

Publish `IPC_Project` only. Compiler outputs, PlatformIO caches, runtime data,
private credentials, archived designs and backups are not part of the source
release. Build binaries for the destination platform rather than copying Mac
build directories to the Pi.

## Credentials

- Commit `secrets.example.h`, never the local `secrets.h`.
- Configure Wi-Fi/MQTT locally; examples use `<mqtt-password>`.
- Rotate credentials exposed in logs or screenshots.
- Keep `.env`, broker password files and private keys outside Git.
- Check staged content: ignore rules do not remove previously tracked secrets.

The release file review covered 177 indexed files and found no high-confidence
private-key/token matches. No secret, cache, build or backup paths were staged.
Pattern scans do not prove that every possible secret is absent.

## Preflight

```bash
git status --short
git diff --check
git check-ignore firmware/esp32_keypad_controller/include/secrets.h
git ls-files '*secrets*' '*.pem' '*.key' '.env*'
```

Run the software tests and follow RELEASE_ACCEPTANCE.md for hardware checks.
Review `git diff --cached --stat` and `git diff --cached` before committing.

## Release Contents

- Uno and ESP32 firmware, CAN database, C reference, C++ backend and Qt/QML HMI.
- English setup, wiring, data-scope and validation documentation.
- Demo link, reference/artwork attribution and dependency notices.
- Source archive and SHA-256 checksum; video hosted as a separate release asset.

## Version 1.0.0

`main` contains the final source; `v1.0.0` identifies the release snapshot.
The release hosts the demo described in DEMO.md. The video predates the last
watchdog correction; subsequent functional Pi acceptance is recorded separately.
CI validates software on Linux, not board flashing or physical motor behavior.

## Rights And Scope

LICENSE.md records the no-blanket-license policy for mixed-provenance material.
THIRD_PARTY_NOTICES.md records the HMI reference, icon licenses and artwork
provenance. These notices do not grant rights that the project does not hold.

Do not present modeled speed/SOC, display-only lights/regen or UART/MQTT frames
as a measured BMS, physical BCM, regenerative power stage or physical CAN bus.
