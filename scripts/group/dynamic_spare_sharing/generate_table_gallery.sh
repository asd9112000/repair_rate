#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_ROOT=$(cd -- "${SCRIPT_DIR}/../../.." && pwd)
cd "${PROJECT_ROOT}"

SIMULATOR="${PROJECT_ROOT}/build/bin/DynamicSpareSharing"
INPUT_ROOT=${OUTPUT_ROOT:-"${PROJECT_ROOT}/reports/group/dynamic_spare_sharing/table_gallery"}
GALLERY_DIR=${GALLERY_OUTPUT_DIR:-"${INPUT_ROOT}/gallery"}
RUNS=${RUNS:-250}
SEED=${SEED:-20260820}
SPATIAL_MODEL=${SPATIAL_MODEL:-mixed}
read -r -a RS_LIST <<< "${RS_VALUES:-2 3 4}"
read -r -a FAULT_LIST <<< "${FAULT_COUNTS:-16 20 24 28 32}"

make dynamic_sharing_b

for rs in "${RS_LIST[@]}"; do
    cs=$rs
#   for cs in 2 3 4; do
    for faults in "${FAULT_LIST[@]}"; do
      out="${INPUT_ROOT}/rs${rs}_cs${cs}_f${faults}/raw"
      "${SIMULATOR}" "$rs" "$cs" \
        --sweep \
        --fault-count "$faults" \
        --spatial "${SPATIAL_MODEL}" \
        --runs "${RUNS}" \
        --seed "${SEED}" \
        --paper-cam-reuse \
        --local-first \
        --summary-only \
        --output-dir "$out"
    done
#   done
done

python3 scripts/group/dynamic_spare_sharing/plot_fault_model_vs_policy.py \
    "${INPUT_ROOT}" \
    --output-dir "${GALLERY_DIR}"
