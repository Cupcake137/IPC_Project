#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cpp_backend="${repo_root}/vcu/cpp_backend"
backend_build_dir="${cpp_backend}/build-local"
hmi_dir="${repo_root}/hmi/cluster_ui"
hmi_build_dir="${hmi_dir}/build-local"

if command -v pio >/dev/null 2>&1; then
    platformio="$(command -v pio)"
elif [[ -x "${HOME}/.platformio/penv/bin/pio" ]]; then
    platformio="${HOME}/.platformio/penv/bin/pio"
else
    printf 'PlatformIO CLI was not found.\n' >&2
    exit 2
fi

cmake_options=(
    -G Ninja
    -DBUILD_TESTING=ON
    -DCMAKE_BUILD_TYPE=Debug
)

if [[ "$(uname -s)" == "Darwin" ]]; then
    if ! command -v brew >/dev/null 2>&1; then
        printf 'Homebrew is required to locate Qt 5 on macOS.\n' >&2
        exit 2
    fi
    qt_prefix="$(brew --prefix qt@5)"
    cmake_options+=("-DCMAKE_PREFIX_PATH=${qt_prefix}")
fi

if [[ ! -f "${repo_root}/firmware/esp32_keypad_controller/include/secrets.h" ]]; then
    printf 'ESP32 MQTT configuration is missing. Copy secrets.example.h to secrets.h and fill it in.\n' >&2
    exit 2
fi

printf '\n[1/6] Building Arduino Uno firmware\n'
"${platformio}" run --project-dir "${repo_root}/firmware/arduino_motor_ecu"

printf '\n[2/6] Building ESP32 steering-controller firmware\n'
"${platformio}" run --project-dir "${repo_root}/firmware/esp32_keypad_controller"

printf '\n[3/6] Building the C VCU reference core\n'
make -C "${repo_root}/vcu/c_core" clean test

printf '\n[4/6] Building and testing the C++ VCU backend\n'
cmake -S "${cpp_backend}" -B "${backend_build_dir}" "${cmake_options[@]}"
cmake --build "${backend_build_dir}"
ctest --test-dir "${backend_build_dir}" --output-on-failure

printf '\n[5/6] Running the terminal backend smoke test\n'
"${backend_build_dir}/ipc_vcu_backend" \
    --simulate \
    --no-mqtt \
    --smoke-test

printf '\n[6/6] Building and testing the Qt/QML cluster\n'
cmake -S "${hmi_dir}" -B "${hmi_build_dir}" "${cmake_options[@]}"
cmake --build "${hmi_build_dir}"
ctest --test-dir "${hmi_build_dir}" --output-on-failure

printf '\nPASS: local project preflight completed.\n'
printf 'Pi UART, reconnect, motor-noise, and soak tests are still hardware-only.\n'
