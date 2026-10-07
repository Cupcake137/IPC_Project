#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd "${script_dir}/.." && pwd)"
if [[ -x "${script_dir}/ipc_vcu_backend" ]]; then
    default_binary="${script_dir}/ipc_vcu_backend"
else
    default_binary="${project_dir}/build-pi/ipc_vcu_backend"
fi
binary="${IPC_BACKEND_BINARY:-${default_binary}}"
serial_port="${IPC_SERIAL_PORT:-/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0}"
if [[ ! -x "${binary}" ]]; then
    printf 'Backend is not executable: %s\n' "${binary}" >&2
    printf 'Build it with: cmake -S %s -B %s/build-pi -G Ninja -DBUILD_TESTING=ON\n' \
        "${project_dir}" "${project_dir}" >&2
    exit 2
fi
if [[ ! -e "${serial_port}" ]]; then
    printf 'CH340 serial device is unavailable: %s\n' "${serial_port}" >&2
    exit 2
fi
if [[ ! -r "${serial_port}" || ! -w "${serial_port}" ]]; then
    printf 'Current user cannot read and write %s. Check membership in dialout.\n' \
        "${serial_port}" >&2
    exit 2
fi

: "${IPC_MQTT_HOST:=127.0.0.1}"
: "${IPC_MQTT_PORT:=1883}"
: "${IPC_MQTT_USER:=ipc_qt}"
if [[ -z "${IPC_MQTT_PASSWORD:-}" ]]; then
    printf 'IPC_MQTT_PASSWORD is required for the ESP32 steering controller.\n' >&2
    exit 2
fi
export IPC_MQTT_HOST IPC_MQTT_PORT IPC_MQTT_USER IPC_MQTT_PASSWORD

if command -v mosquitto_pub >/dev/null 2>&1; then
    if ! mosquitto_pub -h "${IPC_MQTT_HOST}" -p "${IPC_MQTT_PORT}" \
        -u "${IPC_MQTT_USER}" -P "${IPC_MQTT_PASSWORD}" \
        -t ipc/system/preflight -m "pi-ready" -q 1; then
        printf 'MQTT broker preflight failed at %s:%s.\n' \
            "${IPC_MQTT_HOST}" "${IPC_MQTT_PORT}" >&2
        exit 2
    fi
fi

printf 'Starting IPC hardware mode on %s with MQTT %s:%s.\n' \
    "${serial_port}" "${IPC_MQTT_HOST}" "${IPC_MQTT_PORT}"
exec "${binary}" "${serial_port}"
