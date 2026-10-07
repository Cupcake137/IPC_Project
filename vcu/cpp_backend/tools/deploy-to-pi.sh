#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
pi_host="${1:-cupcake@RasberryPi3B}"
archive="${2:-${project_dir}/dist/ipc-vcu-aarch64.tar.gz}"
remote_dir="${3:-ipc-vcu}"

if [[ ! "${remote_dir}" =~ ^[A-Za-z0-9._/-]+$ ]]; then
    printf 'Remote directory must be relative to the Pi home directory.\n' >&2
    exit 2
fi
if [[ ! -f "${archive}" ]]; then
    printf 'Package not found: %s\n' "${archive}" >&2
    exit 2
fi

archive_name="$(basename "${archive}")"
scp "${archive}" "${pi_host}:/tmp/${archive_name}"
ssh "${pi_host}" "
    set -eu
    install -d \"\$HOME/${remote_dir}\"
    tar -xzf \"/tmp/${archive_name}\" -C \"\$HOME/${remote_dir}\"
    \"\$HOME/${remote_dir}/bin/ipc_vcu_tests\"
    \"\$HOME/${remote_dir}/bin/ipc_vcu_backend\" --simulate --smoke-test
"

printf 'VCU backend package deployed and verified on %s:%s\n' "${pi_host}" "${remote_dir}"
