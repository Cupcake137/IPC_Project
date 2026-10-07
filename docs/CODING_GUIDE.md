# Source Code Reading Guide

This guide is intended for a fresher or junior developer. Its goal is to make
the control flow understandable before studying Qt, MQTT, or CAN in depth.

## 1. Recommended Reading Order

### C reference core

1. `vcu/c_core/include/vcu_core.h`: vehicle state data.
2. `vcu/c_core/src/vcu_core.c`: frame handling and PWM calculation.
3. `vcu/c_core/include/ipc_can_db.h`: CAN IDs and message structures.
4. `vcu/c_core/src/ipc_can_db.c`: `data[8]` packing, unpacking, and CRC8.
5. `vcu/c_core/src/can_manager.c`: UART transport for CAN frames.
6. `vcu/c_core/src/main.c`: UART loop and motor-command transmission.

The `run_simulation()` function in `main.c` is a test scenario, not the normal
hardware path. A new reader can skip it initially and return to it later.

### C++ backend

1. `vcu/cpp_backend/include/CanDatabase.h`: message and signal names.
2. `vcu/cpp_backend/src/CanDatabase.cpp`: CAN packing, unpacking, and CRC8.
3. `vcu/cpp_backend/include/Protocol.h`: UART envelope definition.
4. `vcu/cpp_backend/src/Protocol.cpp`: UART CRC16, encoder, and stream parser.
5. `vcu/cpp_backend/src/VCUStateMachine.cpp`: safety rules and PWM calculation.
6. `vcu/cpp_backend/src/DTCManager.cpp`: raw and latched DTC behavior.
7. `vcu/cpp_backend/src/SerialManager.cpp`: UART, reconnect, and heartbeat.
8. `vcu/cpp_backend/src/main.cpp`: module wiring and program startup.
9. `vcu/cpp_backend/src/VehicleModel.cpp`: validated runtime values.
10. `vcu/cpp_backend/src/SteeringInputManager.cpp`: MQTT adapter.

## 2. Hardware Data Flow

```text
Potentiometer and gear buttons
    -> Arduino Uno creates CAN telemetry
    -> UART at 9600 baud
    -> SerialManager extracts the CAN frame
    -> VCUStateMachine checks gear, pedal, and DTC
    -> VCUStateMachine calculates allowed PWM
    -> SerialManager sends a motor command to the Uno
    -> Uno controls the L298 and motor
```

The ESP32 is a separate input path:

```text
Keypad -> ESP32 creates CAN frame 0x300 -> MQTT -> SteeringInputManager -> backend command
```

## 3. Project Coding Rules

- A function should have one main responsibility.
- Use descriptive names such as `serialPort`, `motorPwm`, and `receivedFrame`.
- Validate input and return early to avoid deeply nested conditions.
- Header files declare interfaces; implementation belongs in `.c` or `.cpp` files.
- Do not add templates, extra layers, or smart pointers without a real need.
- Do not remove the watchdog, CRC, alive counter, DTC latch, or Reverse limit to
  shorten the code. They are safety requirements, not unnecessary complexity.
- Update protocol tests and message documentation whenever a frame changes.
- Split files by responsibility, not by function. A small group of related
  functions is easier to follow than many one-function files.

## 4. Qt Concepts Used By The Backend

- `QObject`: base class for modules that expose signals and slots.
- `signals`: notifications emitted when an event occurs.
- `QObject::connect`: connects an event from one module to another function.
- `QTimer`: executes periodic work without creating a custom thread.
- Lambda `[&] { ... }`: a short local function used only for module wiring and
  logging in this project, not for safety-critical logic.

`VehicleModel`, `DTCManager`, and `VCUStateMachine` are normal C++ classes. They
do not use `QObject` or signals and slots. The Mosquitto callback runs outside
the Qt event loop, so only the MQTT adapter needs `QMetaObject::invokeMethod` to
deliver data safely to Qt.

## 5. Checklist Before Changing Logic

1. Identify the input CAN ID and data byte.
2. Define valid values and error values.
3. Decide whether the error must force PWM to zero.
4. Change the smallest relevant function.
5. Add or update a test for that behavior.
6. Run `./tools/test-project-local.sh` before synchronizing the source to the Pi.

## 6. Why Some Files Are Still Technical

CRC calculation, a byte-stream parser, an MQTT callback, and a vehicle state
machine have unavoidable technical detail. They are isolated so they can be
tested and learned one module at a time. Making them shorter by removing error
handling would make the project less reliable, not more junior-friendly.
