#!/usr/bin/env bash

# Reproducible fault-count × spare-line analysis for DynamicSpareSharing.
# Each fault-count map set is generated once inside the C++ runner and reused
# across every symmetric spare count and comparison policy.

set -Eeuo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_ROOT=$(cd -- "${SCRIPT_DIR}/../../.." && pwd)
cd "${PROJECT_ROOT}"
SIMULATOR_RELATIVE="build/bin/DynamicSpareSharing"
SIMULATOR="${PROJECT_ROOT}/${SIMULATOR_RELATIVE}"

FAULT_MIN=$((4 * 4))
FAULT_MAX=$((4 * 16))
FAULT_STEP=4
SPARE_MIN=2
SPARE_MAX=6
RUNS=250
SEED=20260820
FAULT_MODEL=moderate
SPATIAL_MODEL=mixed
STORAGE_MODE=cam
GROUP_LAYOUT=2x2
MAX_BORROWS=3
HYBRID_CAM_ENTRY_WIDTH_BITS=64
BUFFER_ENTRIES=2
BUFFER_MODE_EXPLICIT=false
PAPER_CAM_REUSE=true
PAPER_CAM_REUSE_EXPLICIT=false
LOCAL_FIRST=true
KEEP_RUN_DETAILS=false
BUILD_SIMULATOR=true
OUTPUT_DIR=""
RUN_ID=""

usage() {
    cat <<'EOF'
Usage: scripts/group/dynamic_spare_sharing/sweep.sh [options]

Sweep options:
  --fault-min N          First total A/B/C/D fault count (default: 16)
  --fault-max N          Last total A/B/C/D fault count (default: 64)
  --fault-step N         Total-fault interval (default: 4)
  --spare-min N          First symmetric Rs=Cs count (default: 2)
  --spare-max N          Last symmetric Rs=Cs count (default: 6)
  --runs N               Generated groups per configuration (default: 100)
  --seed N               Deterministic seed (default: 20260820)

Experiment definition:
  --layout LAYOUT        2x2 or row-only 1x4 (default: 2x2)
  --fault-model MODEL    uniform|moderate|strong|hotspot (default: moderate)
  --spatial MODEL        uniform|mixed|clustered (default: mixed)
  --storage MODE         cam|sram (default: cam)
  --max-borrows N        Group borrow budget (default: 3)
  --no-local-first       Analyze sharing even after local repair succeeds
  --hybrid-cam-entry-width-bits N  Independent entry width (default: 64)
  --paper-cam-reuse      Use CAM reuse capacity Rs+Cs (default)
  --buffer N             Override with a fixed extra-pivot buffer capacity

Output/control:
  --output-dir DIR       Explicit run root; must not already exist
  --run-id NAME          Name under reports/group/dynamic_spare_sharing/
  --keep-run-details     Also retain attempts.csv and runs.csv
  --no-build             Use the existing build/bin/DynamicSpareSharing
  -h, --help             Show this help

The 2x2 representative set is No Sharing, Directional m=1/m=2,
Pairwise Edge m=1/m=2, Global Pool m=1.  The 1x4 set replaces those sharing
policies with Pair m=1/m=2 and Neighbor m=1/m=2.

Examples:
  make sim_DynamicSpareSharing ARGS="--fault-model strong --runs 1000"
  make sim_DynamicSpareSharing ARGS="--storage sram --run-id sram_strong"
EOF
}

die() {
    echo "Error: $*" >&2
    exit 1
}

require_value() {
    (( $# >= 2 )) || die "$1 requires a value."
}

is_nonnegative_integer() {
    [[ $1 =~ ^[0-9]+$ ]]
}

while (( $# > 0 )); do
    case "$1" in
        --fault-min)
            require_value "$@"
            FAULT_MIN=$2
            shift 2
            ;;
        --fault-max)
            require_value "$@"
            FAULT_MAX=$2
            shift 2
            ;;
        --fault-step)
            require_value "$@"
            FAULT_STEP=$2
            shift 2
            ;;
        --spare-min)
            require_value "$@"
            SPARE_MIN=$2
            shift 2
            ;;
        --spare-max)
            require_value "$@"
            SPARE_MAX=$2
            shift 2
            ;;
        --runs)
            require_value "$@"
            RUNS=$2
            shift 2
            ;;
        --seed)
            require_value "$@"
            SEED=$2
            shift 2
            ;;
        --fault-model)
            require_value "$@"
            FAULT_MODEL=$2
            shift 2
            ;;
        --spatial)
            require_value "$@"
            SPATIAL_MODEL=$2
            shift 2
            ;;
        --storage)
            require_value "$@"
            STORAGE_MODE=$2
            shift 2
            ;;
        --layout)
            require_value "$@"
            GROUP_LAYOUT=$2
            shift 2
            ;;
        --max-borrows)
            require_value "$@"
            MAX_BORROWS=$2
            shift 2
            ;;
        --hybrid-cam-entry-width-bits)
            require_value "$@"
            HYBRID_CAM_ENTRY_WIDTH_BITS=$2
            shift 2
            ;;
        --buffer)
            require_value "$@"
            [[ ${PAPER_CAM_REUSE_EXPLICIT} == false ]] || \
                die "--buffer and --paper-cam-reuse are mutually exclusive."
            BUFFER_ENTRIES=$2
            BUFFER_MODE_EXPLICIT=true
            PAPER_CAM_REUSE=false
            shift 2
            ;;
        --paper-cam-reuse)
            [[ ${BUFFER_MODE_EXPLICIT} == false ]] || \
                die "--buffer and --paper-cam-reuse are mutually exclusive."
            PAPER_CAM_REUSE=true
            PAPER_CAM_REUSE_EXPLICIT=true
            shift
            ;;
        --no-local-first)
            LOCAL_FIRST=false
            shift
            ;;
        --output-dir)
            require_value "$@"
            OUTPUT_DIR=$2
            shift 2
            ;;
        --run-id)
            require_value "$@"
            RUN_ID=$2
            shift 2
            ;;
        --keep-run-details)
            KEEP_RUN_DETAILS=true
            shift
            ;;
        --no-build)
            BUILD_SIMULATOR=false
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "Unknown option: $1 (use --help for usage)."
            ;;
    esac
done

for value_name in \
    FAULT_MIN FAULT_MAX FAULT_STEP SPARE_MIN SPARE_MAX RUNS SEED MAX_BORROWS \
    HYBRID_CAM_ENTRY_WIDTH_BITS BUFFER_ENTRIES; do
    value=${!value_name}
    is_nonnegative_integer "${value}" || die "${value_name} must be an integer."
done
(( FAULT_MIN > 0 )) || die "--fault-min must be greater than zero."
(( FAULT_MAX >= FAULT_MIN )) || die "--fault-max must be >= --fault-min."
(( FAULT_STEP > 0 )) || die "--fault-step must be greater than zero."
(( SPARE_MIN > 0 )) || die "--spare-min must be greater than zero."
(( SPARE_MAX >= SPARE_MIN )) || die "--spare-max must be >= --spare-min."
(( RUNS > 0 )) || die "--runs must be greater than zero."
(( HYBRID_CAM_ENTRY_WIDTH_BITS > 0 )) || \
    die "--hybrid-cam-entry-width-bits must be greater than zero."

case "${FAULT_MODEL}" in
    uniform)
        CSV_FAULT_MODEL=uniform
        ;;
    moderate)
        CSV_FAULT_MODEL=moderate_imbalance
        ;;
    strong)
        CSV_FAULT_MODEL=strong_imbalance
        ;;
    hotspot)
        CSV_FAULT_MODEL=hotspot
        ;;
    *)
        die "--fault-model must be uniform, moderate, strong, or hotspot."
        ;;
esac
case "${SPATIAL_MODEL}" in
    uniform|mixed|clustered) ;;
    *) die "--spatial must be uniform, mixed, or clustered." ;;
esac
case "${STORAGE_MODE}" in
    cam|sram) ;;
    *) die "--storage must be cam or sram." ;;
esac
case "${GROUP_LAYOUT}" in
    2x2|1x4) ;;
    *) die "--layout must be 2x2 or 1x4." ;;
esac
if [[ -n ${RUN_ID} && ! ${RUN_ID} =~ ^[A-Za-z0-9._-]+$ ]]; then
    die "--run-id may contain only letters, digits, '.', '_' and '-'."
fi

if [[ -z ${OUTPUT_DIR} ]]; then
    if [[ -z ${RUN_ID} ]]; then
        RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)_layout_${GROUP_LAYOUT}_${FAULT_MODEL}_${STORAGE_MODE}_f${FAULT_MIN}-${FAULT_MAX}_step${FAULT_STEP}_s${SPARE_MIN}-${SPARE_MAX}_seed${SEED}"
    fi
    RUN_ROOT="${PROJECT_ROOT}/reports/group/dynamic_spare_sharing/${RUN_ID}"
elif [[ ${OUTPUT_DIR} == /* ]]; then
    RUN_ROOT=${OUTPUT_DIR}
else
    RUN_ROOT="${PROJECT_ROOT}/${OUTPUT_DIR}"
fi

[[ ! -e ${RUN_ROOT} ]] || \
    die "Run directory already exists: ${RUN_ROOT}. Choose a new --run-id or --output-dir."

RAW_DIR="${RUN_ROOT}/raw"
PLOT_DATA_DIR="${RUN_ROOT}/plot_data"
PLOTS_DIR="${RUN_ROOT}/plots"
LOG_DIR="${RUN_ROOT}/logs"
MANIFEST="${RUN_ROOT}/run_config.txt"
SIMULATION_LOG="${LOG_DIR}/simulation.log"
mkdir -p "${RAW_DIR}" "${PLOT_DATA_DIR}" "${PLOTS_DIR}" "${LOG_DIR}"

if [[ ${BUILD_SIMULATOR} == true ]]; then
    make dynamic_sharing_b
fi
[[ -x "${SIMULATOR}" ]] || \
    die "${SIMULATOR_RELATIVE} is missing; run without --no-build."

SIMULATOR_ARGS=(
    "${SPARE_MIN}"
    "${SPARE_MIN}"
    --layout "${GROUP_LAYOUT}"
    --repair-rate-sweep
    --fault-min "${FAULT_MIN}"
    --fault-max "${FAULT_MAX}"
    --fault-step "${FAULT_STEP}"
    --spare-min "${SPARE_MIN}"
    --spare-max "${SPARE_MAX}"
    --fault-model "${FAULT_MODEL}"
    --spatial "${SPATIAL_MODEL}"
    --storage "${STORAGE_MODE}"
    --runs "${RUNS}"
    --seed "${SEED}"
    --max-borrows "${MAX_BORROWS}"
    --hybrid-cam-entry-width-bits "${HYBRID_CAM_ENTRY_WIDTH_BITS}"
    --output-dir "${RAW_DIR}"
)
if [[ ${LOCAL_FIRST} == true ]]; then
    SIMULATOR_ARGS+=(--local-first)
fi
if [[ ${PAPER_CAM_REUSE} == true ]]; then
    SIMULATOR_ARGS+=(--paper-cam-reuse)
else
    SIMULATOR_ARGS+=(--buffer "${BUFFER_ENTRIES}")
fi
if [[ ${KEEP_RUN_DETAILS} == false ]]; then
    SIMULATOR_ARGS+=(--summary-only)
fi

{
    printf 'Simulator=%s\n' "${SIMULATOR_RELATIVE}"
    printf 'Schema=fault_count_x_symmetric_spare_x_representative_policy\n'
    printf 'FaultModel=%s\n' "${FAULT_MODEL}"
    printf 'FaultSpatialModel=%s\n' "${SPATIAL_MODEL}"
    printf 'StorageMode=%s\n' "${STORAGE_MODE}"
    printf 'GroupLayout=%s\n' "${GROUP_LAYOUT}"
    printf 'FaultRange=%s..%s\n' "${FAULT_MIN}" "${FAULT_MAX}"
    printf 'FaultStep=%s\n' "${FAULT_STEP}"
    printf 'SpareRange=%s..%s (Rs=Cs)\n' "${SPARE_MIN}" "${SPARE_MAX}"
    printf 'RunsPerConfiguration=%s\n' "${RUNS}"
    printf 'Seed=%s\n' "${SEED}"
    printf 'LocalFirst=%s\n' "${LOCAL_FIRST}"
    printf 'MaximumGroupBorrows=%s\n' "${MAX_BORROWS}"
    printf 'HybridCAMEntryWidthBits=%s\n' "${HYBRID_CAM_ENTRY_WIDTH_BITS}"
    if [[ ${PAPER_CAM_REUSE} == true ]]; then
        printf 'BufferMode=cam_reuse (capacity=Rs+Cs)\n'
    else
        printf 'BufferMode=fixed (capacity=%s)\n' "${BUFFER_ENTRIES}"
    fi
    printf 'KeepRunDetails=%s\n' "${KEEP_RUN_DETAILS}"
    if [[ ${GROUP_LAYOUT} == 1x4 ]]; then
        printf 'Policies=NoSharing,Pair1,Pair2,Neighbor1,Neighbor2,G1\n'
    else
        printf 'Policies=NoSharing,D1,D2,PairwiseEdge1,PairwiseEdge2,G1\n'
    fi
    printf 'Command='
    printf '%q ' "./${SIMULATOR_RELATIVE}" "${SIMULATOR_ARGS[@]}"
    printf '\n'
} > "${MANIFEST}"

echo "DynamicSpareSharing repair-rate sweep"
echo "  Fault model: ${FAULT_MODEL} (${SPATIAL_MODEL})"
echo "  Storage:     ${STORAGE_MODE}"
echo "  Layout:      ${GROUP_LAYOUT}"
if [[ ${PAPER_CAM_REUSE} == true ]]; then
    echo "  Buffer:      CAM reuse (capacity=Rs+Cs)"
else
    echo "  Buffer:      fixed capacity ${BUFFER_ENTRIES}"
fi
echo "  Faults:      ${FAULT_MIN}..${FAULT_MAX} (step ${FAULT_STEP})"
echo "  Spares:      ${SPARE_MIN}..${SPARE_MAX} (Rs=Cs)"
echo "  Runs:        ${RUNS} per configuration"
echo "  Output:      ${RUN_ROOT}"

"${SIMULATOR}" "${SIMULATOR_ARGS[@]}" 2>&1 | tee "${SIMULATION_LOG}"

python3 scripts/group/dynamic_spare_sharing/plot_sweep.py \
    "${RAW_DIR}/summary.csv" \
    --output-dir "${PLOTS_DIR}" \
    --plot-data-dir "${PLOT_DATA_DIR}" \
    --fault-model "${CSV_FAULT_MODEL}" \
    --storage-mode "${STORAGE_MODE}"

echo
echo "Analysis complete."
echo "  Run root:       ${RUN_ROOT}"
echo "  Raw CSV:        ${RAW_DIR}/summary.csv"
echo "  Processed data: ${PLOT_DATA_DIR}"
echo "  Figures:        ${PLOTS_DIR}"
echo "  Manifest:       ${MANIFEST}"
echo "  Log:            ${SIMULATION_LOG}"
