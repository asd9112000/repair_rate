#!/bin/bash

set -Eeuo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)
cd "${PROJECT_ROOT}"

## ===================================================
## This script has been moved to Makefile,
## Commend: make analyze_redundantrate
## ===================================================


################################################################################
# 腳本名稱: analyze_RedundantRate.sh
# 相關文件: RedundantRate.cpp
# 目的功能:
#   自動化測試 RedundantRate 架構。遍歷 fault 4-16 與 spare line 2-6，
#   生成 fault、執行測試並整理平均額外線數。
#
# 功能流程:
#   1. 迴圈遍歷故障數量 (fault_num: 4-16)
#   2. 對每個故障數量，調用 make gen_fault 生成故障 (stack_height: 250)
#   3. 再迴圈遍歷備用線數量 (s_line: 2-6)
#   4. 運行 make rdr_r 執行修復測試並生成報告
#   5. 所有報告存儲到指定位置供後續分析
################################################################################
make rdr_b fault_generator_b

# Loop through fault numbers 4 to 16
for fault_num in {4..16}; do
    # Generate faults with the current fault number and stack height 250
    make gen_fault f="${fault_num}" s=250

    # Loop through s_line values 2 to 6
    for s_line in {2..6}; do
        # Run rdr_r with the current s_line and fault_num, and set rptName accordingly
        make rdr_r s="${s_line}" rptName="${s_line}_${s_line}_${fault_num}"
    done
done

# Collect RepairReportR_C_F.rpt files into one CSV summary.
report_dir="reports/RedundantRate"
csv_file="${report_dir}/RedundantRate.csv"
csv_rows=$(mktemp)
trap 'rm -f "${csv_rows}"' EXIT
mkdir -p "${report_dir}"

for row_count in {2..6}; do
    col_count=${row_count}
    for fault_count in {4..16}; do
        report_file="${report_dir}/RepairReport${row_count}_${col_count}_${fault_count}.rpt"
        if [[ ! -f ${report_file} ]]; then
            echo "Warning: expected report is missing: ${report_file}" >&2
            continue
        fi

        average=$(awk '
            /Average:[[:space:]]*/ {
                sub(/^.*Average:[[:space:]]*/, "")
                sub(/[[:space:]].*$/, "")
                print
                exit
            }
        ' "${report_file}")

        if [[ -z ${average} ]]; then
            echo "Warning: Average not found in ${report_file}" >&2
            continue
        fi

        printf '%s,%s,%s,%s\n' \
            "${row_count}" "${col_count}" "${fault_count}" "${average}" >> "${csv_rows}"
    done
done

{
    echo "Row,Column,Fault,AverageExtraLines"
    sort -t, -k1,1n -k2,2n -k3,3n "${csv_rows}"
} > "${csv_file}"

echo "CSV summary written to ${csv_file}"

# Generate the publication-ready visualization for the new CSV summary.
python3 scripts/plot_redundant_rate_for_redudant_rate_cpp.py
