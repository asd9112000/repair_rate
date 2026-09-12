#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 ]]; then
    echo "usage: $0 TOP RTL_FILE CPP_TEST [VERILATOR_ARGS ...] -- [TEST_ARGS ...]" >&2
    exit 2
fi

top=$1
rtl_file=$2
cpp_test=$3
shift 3

rtl_file=$(readlink -f "$rtl_file")
cpp_test=$(readlink -f "$cpp_test")

verilator_args=()
test_args=()
after_separator=0
for argument in "$@"; do
    if [[ $argument == -- ]]; then
        after_separator=1
    elif [[ $after_separator -eq 0 ]]; then
        verilator_args+=("$argument")
    else
        test_args+=("$argument")
    fi
done

build_dir="build/rtl/verilator/${top}"
rm -rf "$build_dir"
mkdir -p "$build_dir"

sv_package_files=()
sv_other_files=("$rtl_file")
verilator_options=()
for argument in "${verilator_args[@]}"; do
    if [[ $argument == *.sv && $(basename "$argument") == *_pkg.sv ]]; then
        sv_package_files+=("$argument")
    elif [[ $argument == *.sv ]]; then
        sv_other_files+=("$argument")
    else
        verilator_options+=("$argument")
    fi
done

verilator -Wall -Wno-fatal --cc --exe \
    --top-module "$top" \
    --Mdir "$build_dir" \
    "${verilator_options[@]}" \
    "${sv_package_files[@]}" "${sv_other_files[@]}" "$cpp_test"
make -s -C "$build_dir" -f "V${top}.mk"
"$build_dir/V${top}" "${test_args[@]}"
