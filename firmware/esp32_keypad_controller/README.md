# ESP32 Steering-Wheel Controller

This firmware scans the tested 4x4 keypad matrix and publishes debounced key
events to the Raspberry Pi Mosquitto broker over Wi-Fi.

## Board And Wiring

- PlatformIO board: `esp32dev`
- Drive pins: GPIO 16, 4, 2, 15
- Sense pins: GPIO 19, 18, 5, 17
- Do not hold a keypad key while the ESP32 is powering up because GPIO 2 and 15
  are boot-strapping pins on common ESP32 development boards.

The matrix mapping is intentionally arranged for the tested keypad ribbon
orientation. Do not replace it with the usual row-major example mapping.

## Local Configuration

`include/secrets.h` contains the local Wi-Fi and MQTT settings and is ignored by
Git. For a new checkout:

```bash
cp include/secrets.example.h include/secrets.h
```

Then edit only `include/secrets.h`.

## Build And Upload

```bash
pio run
pio run --target upload
pio device monitor
```

Expected startup output includes `MQTT connected`. Key and network CAN frames
are published as candump text to `ipc/can/swc` at QoS 1, for example
`300#0001000000A5`. Controller presence is also retained as `online`/`offline`
on `ipc/swc/status`. Events produced while MQTT is offline are deliberately
dropped so an old steering-wheel command cannot execute after reconnection.
