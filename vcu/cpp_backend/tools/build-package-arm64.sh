#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${1:-${project_dir}/build-vm-release}"
output_dir="${2:-${project_dir}/dist}"
stage_dir="${build_dir}/package-root"
archive="${output_dir}/ipc-vcu-aarch64.tar.gz"

cmake -S "${project_dir}" -B "${build_dir}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON
cmake --build "${build_dir}"
ctest --test-dir "${build_dir}" --output-on-failure

cmake -E remove_directory "${stage_dir}"
cmake -E make_directory "${stage_dir}" "${output_dir}"
cmake --install "${build_dir}" --prefix "${stage_dir}"
tar -C "${stage_dir}" -czf "${archive}" bin

printf 'Created %s\n' "${archive}"
