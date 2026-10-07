#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
pi_host="${1:-}"
mode="${2:---dry-run}"
remote_dir="${3:-/home/cupcake/Documents/My_project/IPC_Project/}"

if [[ -z "${pi_host}" ]]; then
    printf 'Usage: %s <user@Pi-IP> [--dry-run|--apply] [remote-directory]\n' "$0" >&2
    exit 2
fi
if [[ "${mode}" != "--dry-run" && "${mode}" != "--apply" ]]; then
    printf 'Mode must be --dry-run or --apply.\n' >&2
    exit 2
fi
if [[ "${remote_dir}" != /* ]]; then
    printf 'Remote directory must be an absolute path.\n' >&2
    exit 2
fi

rsync_args=(
    -avz
    --progress
    --exclude=.git/
    --exclude=.pio/
    --exclude='build*/'
    --exclude=dist/
    --exclude='*.log'
    --exclude=firmware/esp32_keypad_controller/include/secrets.h
)
if [[ "${mode}" == "--dry-run" ]]; then
    rsync_args+=(--dry-run --itemize-changes)
fi

printf 'Source:      %s/\n' "${repo_root}"
printf 'Destination: %s:%s\n' "${pi_host}" "${remote_dir}"
printf 'Mode:        %s (files are never deleted remotely)\n' "${mode}"
rsync "${rsync_args[@]}" "${repo_root}/" "${pi_host}:${remote_dir}"

if [[ "${mode}" == "--dry-run" ]]; then
    printf '\nDry run only. Repeat with --apply after checking the destination above.\n'
else
    printf '\nProject source synchronized successfully.\n'
fi
