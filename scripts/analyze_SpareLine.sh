## ===================================================
## This script has been moved to Makefile,
## Commend: make analyze_spareline
## ===================================================


#!/bin/bash

################################################################################
# 腳本名稱: analyze_SpareLine.sh
# 相關文件: SharedLine.cpp
# 目的功能:
#   自動化測試 SharedLine 架構的修復率性能。通過遍歷不同的故障數量
#   和備用線數量，生成故障並運行模擬，最後提取並統計修復率數據
#   (RepairRate 和 RepairRate_RECAM) 到 CSV 文件。
#
# 功能流程:
#   1. 清空舊的日誌和報告文件
#   2. 迴圈遍歷故障數量 (fault_num: 8-16)
#   3. 對每個故障數量，調用 make gen_fault 生成故障
#   4. 再迴圈遍歷備用線數量 (s_line: 2-6)
#   5. 運行 SharedLine.o 模擬和修復操作
#   6. 從日誌中提取修復率數據，存儲到 CSV 文件
################################################################################

rm -f SharedLine.log
rm -f ./Report_SharedLine/

# Loop through fault numbers 8 to 14
for fault_num in {8..16}; do
    # Generate faults with the current fault number and stack height 250
    make gen_fault f=$fault_num s=100

    # Loop through s_line values 3 to 5
    for s_line in {2..6}; do
        # Run rdr_r with the current s_line and fault_num, and set rptName accordingly
        # ./SharedLine.o $s_line $s_line --rptName "RepairReport_r${s_line}_c${s_line}_f${fault_num}.rpt" > SharedLine.log 2>&1
        ./SharedLine.o $s_line $s_line --rptName "SL_RepairReport_r${s_line}_c${s_line}_f${fault_num}.rpt" --faultNum $fault_num >> SharedLine.log 2>&1
    done
done

# 模擬結束後從 SharedLine.log 中提取 RepairRate 和 RepairRate_RECAM 的數值，並將它們保存到 repair_rates.csv 文件中。
echo "FaultNum,SpareLine,RepairRate,RepairRate_RECAM" >> ./Report_SharedLine/repairRates.csv
grep "RepairRate:" SharedLine.log | while read -r line; do
    fault_num=$(echo "$line" | grep -oP 'faultNum: \K\d+')
    spare_line=$(echo "$line" | grep -oP 'SpareLine: \K\d+')
    repair_rate=$(echo "$line" | grep -oP 'RepairRate: \K[0-9.]+')
    repair_rate_recam=$(echo "$line" | grep -oP 'RepairRate_RECAM: \K[0-9.]+')
    echo "$fault_num,$spare_line,$repair_rate,$repair_rate_recam" >> ./Report_SharedLine/repairRates.csv
done