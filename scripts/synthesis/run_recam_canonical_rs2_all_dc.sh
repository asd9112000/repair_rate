#!/usr/bin/env bash
set -uo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
overall_status=0

if ! bash "$repo_root/scripts/synthesis/run_recam_canonical_rs2_early_dc.sh"; then
    overall_status=1
fi
if ! bash "$repo_root/scripts/synthesis/run_recam_canonical_rs2_global_noscratch_dc.sh"; then
    overall_status=1
fi

exit "$overall_status"
