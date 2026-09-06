#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_ROOT=$(cd -- "${SCRIPT_DIR}/../../.." && pwd)
cd "${PROJECT_ROOT}"

SIMULATOR="${PROJECT_ROOT}/build/bin/SharedLine"
REPORT_DIR="${PROJECT_ROOT}/reports/legacy/sharedline"
LOG_FILE="${REPORT_DIR}/SharedLine.log"
CSV_FILE="${REPORT_DIR}/repairRates.csv"

make sl_b fault_generator_b
mkdir -p "${REPORT_DIR}"

################################################################################
# 腳本名稱: analyze_SpareLine.sh
# 相關文件: SharedLine.cpp
# 目的功能:
#   自動化測試 SharedLine 架構的修復率性能。通過遍歷不同的故障數量
#   和備用線數量，生成故障並運行模擬，最後提取並統計修復率數據
#   (RepairRate 和 RepairRate_RECAM) 到 CSV 文件。
#
# 功能流程:
#   1. 清空這個分析流程產生的舊日誌和 CSV
#   2. 迴圈遍歷故障數量 (fault_num: 8-16)
#   3. 對每個故障數量，調用 make gen_fault 生成故障
#   4. 再迴圈遍歷備用線數量 (s_line: 2-6)
#   5. 運行 build/bin/SharedLine 模擬和修復操作
#   6. 從日誌中提取修復率數據，存儲到 CSV 文件
################################################################################

rm -f "${LOG_FILE}" "${CSV_FILE}"

# Loop through fault numbers 8 to 16
for fault_num in {8..16}; do
    # Generate faults with the current fault number and stack height 250
    make gen_fault f="${fault_num}" s=100

    # Loop through s_line values 2 to 6
    for s_line in {2..6}; do
        "${SIMULATOR}" "${s_line}" "${s_line}" \
            --rptName "SL_RepairReport_r${s_line}_c${s_line}_f${fault_num}.rpt" \
            --faultNum "${fault_num}" >> "${LOG_FILE}" 2>&1
    done
done

# 模擬結束後從 log 提取 RepairRate 和 RepairRate_RECAM，寫入 CSV。
echo "FaultNum,SpareLine,RepairRate,RepairRate_RECAM" > "${CSV_FILE}"
grep "RepairRate:" "${LOG_FILE}" | while read -r line; do
    fault_num=$(echo "$line" | grep -oP 'faultNum: \K\d+')
    spare_line=$(echo "$line" | grep -oP 'SpareLine: \K\d+')
    repair_rate=$(echo "$line" | grep -oP 'RepairRate: \K[0-9.]+')
    repair_rate_recam=$(echo "$line" | grep -oP 'RepairRate_RECAM: \K[0-9.]+')
    echo "$fault_num,$spare_line,$repair_rate,$repair_rate_recam" >> "${CSV_FILE}"
done

# Generate publication-ready figures and print comparison statistics.
python3 scripts/legacy/sharedline/plot_repair_rates.py "${CSV_FILE}" \
    --output-dir "${REPORT_DIR}"
