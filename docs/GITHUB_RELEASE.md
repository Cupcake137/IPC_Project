# GitHub Release Guide

## Publication Boundary

Publish the `IPC_Project` repository only, not its parent `IPC_prj` folder.
Private checkpoints, Git bundles, archived designs, reference projects and audit
exports remain outside the release. Do not upload an entire Documents folder.

## Credentials

- Keep `firmware/esp32_keypad_controller/include/secrets.h` private. Commit the
  example file only; configure Wi-Fi/MQTT locally.
- Documentation uses `<mqtt-password>`, not a working password.
- Rotate any password previously shared in logs, screenshots or conversations.
- Do not include `.env`, broker password files, private keys or credential-bearing
  screenshots in a release. Check staged content, not just ignore rules.
- `.gitignore` does not remove files already tracked or secrets in Git history.

The 2026-10-07 pattern scan covered 69 current text files and 43 reachable Git
text blobs before these packaging documents were added. Two literal credentials
were found in the HMI README and replaced. No matches were found in the scanned
historical text blobs. A shell environment-variable export was a false positive.
The local `secrets.h` file was confirmed ignored and its contents were not printed.
This limited scan is not proof that every possible secret is absent; binary files,
unreachable objects, external repositories and remote branches were not certified.

## Source Review Before Staging

Run from the repository root:

```bash
git status --short
git diff --check
git check-ignore firmware/esp32_keypad_controller/include/secrets.h
git ls-files '*secrets*' '*.pem' '*.key' '.env*'
```

After selecting files to stage, review both `git diff --cached --stat` and
`git diff --cached` locally. The tree contains earlier renames and staged changes;
do not assume every staged change belongs to the most recent packaging work.
Do not use a broad parent-directory upload or blindly stage every file.

## Release Evidence

The focused UART watchdog, C-reference warning and build/documentation
corrections are recorded in [Final Source Review](FINAL_REVIEW.md), with
regression results in [Local Test Results](LOCAL_TEST_RESULTS.md).

Use [Local Test Results](LOCAL_TEST_RESULTS.md) and
[Release Acceptance](RELEASE_ACCEPTANCE.md). Record the actual Pi/firmware versions
tested and a short hardware demonstration. The latest local changes have not been
synced to the Pi by the assistant; the owner performs that step.

The Uno build on the current Mac was blocked by the installed AVR compiler's
architecture, not verified as a passing firmware build. Compile/flash it with a
working toolchain and complete the updated hardware checklist before claiming
end-to-end release validation. Do not claim physical CAN, measured SOC/speed,
real regenerative braking or road-vehicle safety certification.

## Assets And License

The portfolio handover records a no-blanket-license policy in
[Rights And Licensing](../LICENSE.md). It does not relicense retained HMI code
or unknown-origin artwork. See [Third-Party Notices](../THIRD_PARTY_NOTICES.md)
for provenance notes before further redistribution. Do not classify an
unknown-origin image as freely redistributable.

## Final Package

- Current firmware, CAN database, C core, C++ backend and approved QML HMI.
- English setup, hardware contract, data scope, tests and acceptance records.
- Retained asset attribution, dependency notices and explicit rights policy.
- A concise demo showing gear, throttle response, keypad menu and fail-safe behavior.

## Review Handover

Source is submitted on `codex/release-handover` through a pull request for owner
review. The `v1.0.0-review` prerelease hosts the video described in DEMO.md.
No force push, history rewrite or automatic Pi synchronization is required.
Merge only after reviewing the source, recorded validation and provenance notes.
