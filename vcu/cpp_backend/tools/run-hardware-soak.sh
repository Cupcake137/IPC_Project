#!/usr/bin/env bash
set -euo pipefail

binary="${1:-${HOME}/ipc-vcu/bin/ipc_vcu_backend}"
serial_port="${2:-/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0}"
duration_seconds="${3:-1800}"
log_file="${4:-ipc-vcu-hardware-soak.log}"

if [[ ! -x "${binary}" ]]; then
    printf 'Backend is not executable: %s\n' "${binary}" >&2
    exit 2
fi
if [[ ! -e "${serial_port}" ]]; then
    printf 'Serial device is unavailable: %s\n' "${serial_port}" >&2
    exit 2
fi
if [[ ! "${duration_seconds}" =~ ^[0-9]+$ ]] || (( duration_seconds < 60 )); then
    printf 'Duration must be at least 60 seconds.\n' >&2
    exit 2
fi

printf 'Starting %ss hardware soak test.\n' "${duration_seconds}"
printf 'During the run, exercise P/R/N/D and repeat high-pedal operation with the motor unloaded.\n'

set +e
timeout --signal=INT "${duration_seconds}" \
    "${binary}" "${serial_port}" 2>&1 | tee "${log_file}"
backend_status=${PIPESTATUS[0]}
set -e

if [[ ${backend_status} -ne 124 && ${backend_status} -ne 130 ]]; then
    printf 'FAIL: backend exited unexpectedly with status %s.\n' "${backend_status}" >&2
    exit 1
fi

if grep -Eq 'U0100|state=4|\[Serial\]' "${log_file}"; then
    printf 'FAIL: communication, FAULT, or serial errors were found in %s.\n' "${log_file}" >&2
    exit 1
fi

printf 'PASS: no communication loss, FAULT state, or serial error was logged.\n'
