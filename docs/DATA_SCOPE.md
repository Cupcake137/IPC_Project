# Prototype Data And Restart Behavior

This document describes the implemented prototype, not production vehicle
sensing or automotive safety certification.

## SOC Ownership

Keep the existing Uno-owned SOC model for this release. The Pi receives and
displays SOC; it does not calculate a second SOC or restore a saved value into
the ECU. Moving SOC to the Pi would change the data contract and is separate
future work.

- Uno initializes simulated SOC to 100% on boot and stores it only in RAM.
- Every scheduled 100 ms update, traction with positive PWM consumes
  `0.03 + (PWM / 255) * 0.12` percentage points. Other conditions consume 0.002.
  These accelerated demonstration rates are not battery-energy measurements.
- Energy telemetry is sent every 1000 ms, rounded to a whole percent.
- Resetting or power-cycling Uno restores SOC to 100% and gear to Park.
- Restarting only the Pi application does not intentionally reset Uno SOC.
  Opening the Uno USB debug port can reset some boards; this is separate from
  opening the CH340 link to Uno SoftwareSerial.
- SOC below 15% produces raw DTC 0x11. The HMI also shows a low-battery warning
  at received SOC <= 20%, even before raw DTC 0x11 appears.
- Zero received SOC forces Fault and zero authorized PWM. SOC is not a BMS.

## Data Classification

| Value or action | Implemented source / meaning |
| --- | --- |
| Pedal | Uno A0 potentiometer; raw ADC below 40 maps to zero |
| Gear | Uno D9/D10 buttons; shift blocked above 2% pedal |
| Motor output | Real L298 PWM/direction command; no speed/current sensor |
| Speed | Uno derives km/h from applied PWM, maximum model speed 120 |
| SOC | Accelerated Uno battery simulation, not actual battery capacity |
| DTE | Pi heuristic from SOC, mode and authorized PWM; not calibrated range |
| ODO / Trip | Integrated fresh estimated speed; independent persisted counters |
| DRIVE minutes | Accumulated fresh positive-speed time, not total app uptime |
| Temperature / weather | Fixed display demonstration, not a sensor or service |
| Average energy | Fixed 15.8 kWh/100km display example |
| Regen | Selectable display setting 0-3; no energy-recovery hardware |
| Lights / turn signals | Interactive display states, not physical lamp outputs |
| Seatbelt / parking brake | Derived from gear Park, not dedicated sensors |
| ABS / ESC | Visual placeholders, not implemented controllers |
| Brightness | UI overlay, not physical display-backlight control |

Disconnecting a motor lead does not prevent simulated speed, SOC consumption
or overload logic. The firmware does not measure whether the motor is turning.
Do not interpret displayed motion as proof of physical motor operation.

## Fault Demonstrations

| Internal code | Trigger | Pi behavior |
| --- | --- | --- |
| 0x11 | Uno simulated SOC < 15% | Warning; maximum PWM 80; Reverse still <= 43 |
| 0x22 | Uno applied PWM >= 250 for at least 3000 ms, unless a higher-priority raw condition is present | Critical; zero PWM |
| 0xE1 | Pi communication watchdog detects lost ECU communication after telemetry has been received | Critical; zero PWM |
| 0xE2 | Uno receives no valid motor command for > 350 ms after its first command | Critical; Uno stops output locally |
| Other nonzero | Unknown diagnostic code received | Critical by default |

The UI labels P0A80/P1A10/U0100/U0101 are project diagnostic descriptions, not
evidence that the implementation follows standardized vehicle DTC definitions.
Overload is a PWM/time demonstration, not measured overcurrent protection.

Pi holds a critical fault until raw conditions have cleared and a clear request
is safe: Park, pedal zero, speed zero. A later low-SOC warning cannot downgrade
the held critical fault. The latch is process-local and is not persisted.
An active low-SOC raw condition cannot be acknowledged away by pressing D.

Exercise overload in simulation first. Do not force a real motor to full PWM
just to demonstrate a DTC. Communication-loss tests require a secured, unloaded
motor and the precautions in HARDWARE_BRINGUP.md.

## Persistence And Remaining Boundaries

Hardware HMI stores distance in Qt AppDataLocation under IPCProject/IPCCluster,
in distance.ini. New installations start at zero, without the old 1803 km
example. Saves occur every five seconds, on safe Trip reset and normal exit.
Abrupt termination may lose the unsaved interval. Simulation neither loads nor
saves the hardware distance file. Terminal backend does not maintain HMI distance.

SOC, diagnostic latches, regen, lighting and display settings are not persisted.
No actual battery voltage/current, wheel speed or motor reset-cause telemetry
is available. Electrical reliability with the connected motor still requires
hardware validation. Future work can add real sensors, a BMS, physical CAN,
settings persistence and more precise signal-health reporting.
