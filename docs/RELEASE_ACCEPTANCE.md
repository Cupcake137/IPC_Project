# Hardware Release Acceptance

## Recorded Owner Acceptance

2026-10-08: the owner confirmed that the updated project ran stably on the Pi
after the final watchdog correction. This records overall functional acceptance.
Individual items below remain a repeatable checklist, not invented per-test
evidence. Exact firmware hashes, test duration and load conditions were not supplied.

Use this checklist after manually synchronizing and rebuilding the updated
source on the Pi. Follow HARDWARE_BRINGUP.md first. Record the tested revision,
firmware versions and wiring configuration; old hardware results do not cover
the newly changed behavior automatically.

## Preconditions

- [ ] Uno firmware builds on a compatible toolchain.
- [ ] Both firmware targets are flashed from the intended source revision.
- [ ] Pi backend and HMI builds/tests pass.
- [ ] MQTT credentials and UART by-id path are correct.
- [ ] Only the HMI OR terminal backend owns the CH340 UART.
- [ ] Motor is secured; test starts with motor power off, Park and zero pedal.

## Updated UI And Metrics

- [ ] Startup: details closed, Trip summary shown, no missing assets or overlap.
- [ ] A opens/closes, B closes, C selects Vehicle and closes.
- [ ] 4/6 navigate and wrap pages only when details are open.
- [ ] 2/8 adjust Vehicle regen or Display brightness only when open.
- [ ] Clicking a menu icon or warning telltale opens the expected details.
- [ ] 1/3/7 control light indicators; */0/# control turn signals/hazard.
- [ ] 9 cycles drive mode and affects authorized motor output as documented.
- [ ] Trip reset is blocked in Drive/Reverse, with pedal/speed nonzero, or stale speed.
- [ ] In fresh Park at zero pedal/speed, 5 on Trip resets Trip but never ODO.
- [ ] Normal HMI restart preserves hardware ODO/Trip and moving time.
- [ ] Simulation does not change the hardware distance file.
- [ ] UART disconnect or missing speed frames stops distance accumulation after the freshness window.
- [ ] Reconnect does not add the disconnected time to distance.

## Control And Fault Behavior

- [ ] Park/Neutral: potentiometer sweep never drives the motor.
- [ ] Drive/Reverse: direction is correct and Reverse stays PWM <= 43.
- [ ] Gear shift is rejected above 2% pedal.
- [ ] Low-SOC DTC 0x11 limits PWM <= 80 without critical stop; zero SOC stops output.
- [ ] Critical/unknown DTC commands zero PWM and stays latched after raw recovery.
- [ ] Later low SOC does not downgrade a latched critical DTC.
- [ ] D requests clear only after raw recovery in Park at zero pedal/speed.
- [ ] Stopping the Pi application stops Uno output through its 350 ms watchdog.
- [ ] UART loss/restore produces the expected fault and reconnect behavior.
- [ ] Duplicate/invalid Drive or Energy-only traffic cannot maintain motor PWM
      after the Pi's 500 ms freshness window (100 ms watchdog polling).
- [ ] ESP32/broker loss and recovery do not replay old key actions unexpectedly.

Use simulation for the full-PWM overload demonstration first. Do not deliberately
stress the physical motor to create a fault. Keep the safe setup for watchdog
tests and power down before touching wiring.

## Reliability Evidence

- [ ] Repeat with motor connected, not only with a motor lead removed.
- [ ] Run a 30-minute secured/unloaded motor test with varied gear/PWM.
- [ ] Record Uno USB debug and Pi logs; no unexplained reset or repeated CRC failure.
- [ ] Capture the final UI and hardware demo after the updated checks pass.

Test record:

| Field | Recorded value |
| --- | --- |
| Date / operator | |
| Source revision | |
| Uno / ESP32 firmware | |
| Pi OS / Qt version | |
| Motor supply / wiring | |
| Log and video paths | |
| Failed checks / observations | |

Mark only observed checks. An unresolved motor-connected reset remains a
known reliability limitation, even when the disconnected-motor test is stable.
