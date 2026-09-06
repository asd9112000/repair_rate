#!/usr/bin/env bash

# Sweep fault counts and symmetric spare-line counts for SharedLine_SRAM.
# Each fault count is generated once, so every spare-line case at that fault
# count is evaluated against exactly the same set of fault patterns.

set -Eeuo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_ROOT=$(cd -- "${SCRIPT_DIR}/../../.." && pwd)
cd "${PROJECT_ROOT}"
SIMULATOR_RELATIVE="build/bin/SharedLine_SRAM"
SIMULATOR="${PROJECT_ROOT}/${SIMULATOR_RELATIVE}"
FAULT_GENERATOR_RELATIVE="build/bin/fault_generator"
FAULT_GENERATOR="${PROJECT_ROOT}/${FAULT_GENERATOR_RELATIVE}"

FAULT_MIN=4
FAULT_MAX=16
SPARE_MIN=2
SPARE_MAX=6
STACK_HEIGHT=100
BUFFER_ENTRIES=2
BUFFER_MODE_EXPLICIT=false
PAPER_CAM_REUSE=false
BUILD_SIMULATOR=true
OUTPUT_DIR=""

usage() {
    cat <<'EOF'
Usage: scripts/legacy/sharedline_sram/sweep.sh [options]

Sweep options:
  --fault-min N          First fault count (default: 4)
  --fault-max N          Last fault count (default: 16)
  --spare-min N          First Rs=Cs spare-line count (default: 2)
  --spare-max N          Last Rs=Cs spare-line count (default: 6)
  --stack-height N       Number of four-PE patterns per case (default: 100)

Buffer CAM mode (mutually exclusive):
  --buffer N             Fixed additional-pivot buffer capacity (default: 2)
  --paper-cam-reuse      Per-configuration paper capacity Rs+Cs

Output/control:
  --output-dir DIR       CSV/figure directory (default: mode-specific directory)
  --no-build             Use existing simulator/generator binaries in build/bin
  -h, --help             Show this help

Examples:
  scripts/legacy/sharedline_sram/sweep.sh --buffer 0
  scripts/legacy/sharedline_sram/sweep.sh --buffer 2 --fault-min 8 --fault-max 12
  scripts/legacy/sharedline_sram/sweep.sh --paper-cam-reuse
EOF
}

die() {
    echo "Error: $*" >&2
    exit 1
}

require_value() {
    if (( $# < 2 )); then
        die "$1 requires a value."
    fi
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
        --stack-height)
            require_value "$@"
            STACK_HEIGHT=$2
            shift 2
            ;;
        --buffer)
            require_value "$@"
            if [[ ${PAPER_CAM_REUSE} == true ]]; then
                die "--buffer and --paper-cam-reuse are mutually exclusive."
            fi
            BUFFER_ENTRIES=$2
            BUFFER_MODE_EXPLICIT=true
            shift 2
            ;;
        --paper-cam-reuse)
            if [[ ${BUFFER_MODE_EXPLICIT:-false} == true ]]; then
                die "--buffer and --paper-cam-reuse are mutually exclusive."
            fi
            PAPER_CAM_REUSE=true
            shift
            ;;
        --output-dir)
            require_value "$@"
            OUTPUT_DIR=$2
            shift 2
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

for value_name in FAULT_MIN FAULT_MAX SPARE_MIN SPARE_MAX STACK_HEIGHT; do
    value=${!value_name}
    is_nonnegative_integer "${value}" || die "${value_name} must be an integer."
done
is_nonnegative_integer "${BUFFER_ENTRIES}" || die "--buffer must be a nonnegative integer."

(( FAULT_MIN > 0 )) || die "--fault-min must be greater than zero."
(( FAULT_MAX >= FAULT_MIN )) || die "--fault-max must be >= --fault-min."
(( SPARE_MIN > 0 )) || die "--spare-min must be greater than zero."
(( SPARE_MAX >= SPARE_MIN )) || die "--spare-max must be >= --spare-min."
(( STACK_HEIGHT > 0 )) || die "--stack-height must be greater than zero."

if [[ ${PAPER_CAM_REUSE} == true ]]; then
    MODE_SLUG=paper_cam_reuse
    MODE_DESCRIPTION="paper additional-pivot CAM reuse (capacity = local Rs+Cs)"
    SIMULATOR_MODE_ARGS=(--paper-cam-reuse)
else
    MODE_SLUG="buffer_${BUFFER_ENTRIES}"
    MODE_DESCRIPTION="fixed additional-pivot buffer (capacity = ${BUFFER_ENTRIES})"
    SIMULATOR_MODE_ARGS=(--buffer "${BUFFER_ENTRIES}")
fi

if [[ -z ${OUTPUT_DIR} ]]; then
    OUTPUT_DIR="${PROJECT_ROOT}/reports/legacy/sharedline_sram/analysis_${MODE_SLUG}"
elif [[ ${OUTPUT_DIR} != /* ]]; then
    OUTPUT_DIR="${PROJECT_ROOT}/${OUTPUT_DIR}"
fi

REPORT_RELATIVE_DIR="analysis_${MODE_SLUG}/reports"
REPORT_DIR="${PROJECT_ROOT}/reports/legacy/sharedline_sram/${REPORT_RELATIVE_DIR}"
CSV_FILE="${OUTPUT_DIR}/repairRates.csv"
LOG_FILE="${OUTPUT_DIR}/SharedLine_SRAM.log"
CONFIG_FILE="${OUTPUT_DIR}/run_config.txt"

mkdir -p "${OUTPUT_DIR}" "${REPORT_DIR}"

if [[ ${BUILD_SIMULATOR} == true ]]; then
    make sl_sram_b fault_generator_b
fi

[[ -x "${SIMULATOR}" ]] || \
    die "${SIMULATOR_RELATIVE} is missing; run without --no-build."
[[ -x "${FAULT_GENERATOR}" ]] || \
    die "${FAULT_GENERATOR_RELATIVE} is missing; run without --no-build."

printf 'FaultNum,SpareLine,RepairRate,RepairRate_RECAM\n' > "${CSV_FILE}"
: > "${LOG_FILE}"

{
    printf 'Simulator=%s\n' "${SIMULATOR_RELATIVE}"
    printf 'BufferMode=%s\n' "${MODE_DESCRIPTION}"
    printf 'FaultRange=%s..%s\n' "${FAULT_MIN}" "${FAULT_MAX}"
    printf 'SpareRange=%s..%s (Rs=Cs)\n' "${SPARE_MIN}" "${SPARE_MAX}"
    printf 'StackHeight=%s\n' "${STACK_HEIGHT}"
    printf 'CaseCount=%s\n' "$(( (FAULT_MAX - FAULT_MIN + 1) * (SPARE_MAX - SPARE_MIN + 1) ))"
} > "${CONFIG_FILE}"

CASE_LOG=$(mktemp)
trap 'rm -f "${CASE_LOG}"' EXIT

echo "SharedLine_SRAM repair-rate sweep"
echo "  Mode:   ${MODE_DESCRIPTION}"
echo "  Faults: ${FAULT_MIN}..${FAULT_MAX}"
echo "  Spares: ${SPARE_MIN}..${SPARE_MAX} (Rs=Cs)"
echo "  Output: ${OUTPUT_DIR}"

for (( fault_num = FAULT_MIN; fault_num <= FAULT_MAX; ++fault_num )); do
    echo "Generating ${STACK_HEIGHT} patterns for FaultNum=${fault_num} ..."
    if ! make gen_fault f="${fault_num}" s="${STACK_HEIGHT}" >> "${LOG_FILE}" 2>&1; then
        die "Fault generation failed for FaultNum=${fault_num}; see ${LOG_FILE}."
    fi

    for (( spare_line = SPARE_MIN; spare_line <= SPARE_MAX; ++spare_line )); do
        report_name="${REPORT_RELATIVE_DIR}/SL_SRAM_RepairReport_r${spare_line}_c${spare_line}_f${fault_num}_${MODE_SLUG}.rpt"
        printf '\n=== FaultNum=%s SpareLine=%s Mode=%s ===\n' \
            "${fault_num}" "${spare_line}" "${MODE_SLUG}" >> "${LOG_FILE}"

        if ! "${SIMULATOR}" \
            "${spare_line}" "${spare_line}" \
            "${SIMULATOR_MODE_ARGS[@]}" \
            --rptName "${report_name}" > "${CASE_LOG}" 2>&1; then
            tee -a "${LOG_FILE}" < "${CASE_LOG}" > /dev/null
            die "Simulation failed for FaultNum=${fault_num}, SpareLine=${spare_line}; see ${LOG_FILE}."
        fi
        tee -a "${LOG_FILE}" < "${CASE_LOG}" > /dev/null

        summary_count=$(awk '/RepairRate:/ { ++count } END { print count + 0 }' "${CASE_LOG}")
        if [[ ${summary_count} -ne 1 ]]; then
            die "Expected one repair-rate summary for FaultNum=${fault_num}, SpareLine=${spare_line}; found ${summary_count}. See ${LOG_FILE}."
        fi

        repair_rate=$(awk '
            /RepairRate:/ {
                for (i = 1; i <= NF; ++i) {
                    if ($i == "RepairRate:") print $(i + 1)
                }
            }
        ' "${CASE_LOG}")
        repair_rate_recam=$(awk '
            /RepairRate:/ {
                for (i = 1; i <= NF; ++i) {
                    if ($i == "RepairRate_RECAM:") print $(i + 1)
                }
            }
        ' "${CASE_LOG}")

        [[ ${repair_rate} =~ ^([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?$ ]] || \
            die "Invalid RepairRate '${repair_rate}' for FaultNum=${fault_num}, SpareLine=${spare_line}."
        [[ ${repair_rate_recam} =~ ^([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?$ ]] || \
            die "Invalid RepairRate_RECAM '${repair_rate_recam}' for FaultNum=${fault_num}, SpareLine=${spare_line}."

        printf '%s,%s,%s,%s\n' \
            "${fault_num}" "${spare_line}" "${repair_rate}" "${repair_rate_recam}" >> "${CSV_FILE}"
        printf '  FaultNum=%s SpareLine=%s: SharedLine_SRAM=%s, RECAM=%s\n' \
            "${fault_num}" "${spare_line}" "${repair_rate}" "${repair_rate_recam}"
    done
done

PLOT_CACHE_DIR=${MPLCONFIGDIR:-"${TMPDIR:-/tmp}/repair_rate_matplotlib_${UID}"}
mkdir -p "${PLOT_CACHE_DIR}"
MPLCONFIGDIR="${PLOT_CACHE_DIR}" python3 scripts/legacy/sharedline/plot_repair_rates.py \
    "${CSV_FILE}" \
    --output-dir "${OUTPUT_DIR}" \
    --architecture-label "SharedLine_SRAM"

echo
echo "Analysis complete."
echo "  CSV:     ${CSV_FILE}"
echo "  Figures: ${OUTPUT_DIR}"
echo "  Log:     ${LOG_FILE}"
