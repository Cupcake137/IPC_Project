# IPC CAN Database

`ipc_virtual_can.dbc` is the single source of truth for CAN IDs and signal byte
positions. The project uses the same CAN data even while UART and MQTT are the
physical transports.

## Messages

| ID | Message | Direction |
|---:|---|---|
| `0x080` | `VCU_MOTOR_COMMAND` | Raspberry Pi to Arduino Uno |
| `0x100` | `ECU_DRIVE_STATUS` | Arduino Uno to Raspberry Pi |
| `0x101` | `ECU_ENERGY_STATUS` | Arduino Uno to Raspberry Pi |
| `0x102` | `ECU_DIAG_STATUS` | Arduino Uno to Raspberry Pi |
| `0x300` | `SWC_KEY_EVENT` | ESP32 to Raspberry Pi |
| `0x301` | `SWC_NETWORK_STATUS` | ESP32 to Raspberry Pi |

## How To Read A Trace

```text
100#2A3F036B01DA
```

- `100`: CAN ID `ECU_DRIVE_STATUS`.
- `2A`: pedal is 42 percent.
- `3F`: speed is 63 km/h.
- `03`: gear is Drive.
- `6B`: applied PWM is 107.
- `01`: alive counter is 1.
- `DA`: CAN payload CRC8.

The alive counter changes from 0 to 15 and then wraps to 0. A repeated counter
is rejected as a duplicate. The CRC8 covers CAN ID, DLC, and all data bytes
except the final CRC byte.

## Transports

- UART: binary envelope `AA 55`, version, CAN ID, DLC, CAN data, CRC16.
- MQTT: candump text such as `300#...` on `ipc/can/swc`.
- Future MCP2515: send the same CAN ID, DLC, and data without changing the DBC.

Transport CRC16 protects the UART packet. CAN CRC8 protects the message payload.
Keeping them separate makes the same message usable on UART, MQTT, and physical
CAN.
