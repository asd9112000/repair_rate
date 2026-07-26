## ===================================================
## This script has been moved to Makefile,
## Commend: make analyze_redundantrate
## ===================================================


################################################################################
# 腳本名稱: analyze_RedundantRate.sh
# 相關文件: RedundantRate.cpp
# 目的功能:
#   自動化測試 RedundantRate 架構的修復率性能。通過遍歷不同的故障數量 (8-14)
#   和備用線數量 (3-5)，生成故障並運行修復測試，評估冗餘設計的修復效率。
#
# 功能流程:
#   1. 迴圈遍歷故障數量 (fault_num: 8-14)
#   2. 對每個故障數量，調用 make gen_fault 生成故障 (stack_height: 250)
#   3. 再迴圈遍歷備用線數量 (s_line: 3-5)
#   4. 運行 make rdr_r 執行修復測試並生成報告
#   5. 所有報告存儲到指定位置供後續分析
################################################################################
#!/bin/bash

# Loop through fault numbers 8 to 14
for fault_num in {8..14}; do
    # Generate faults with the current fault number and stack height 250
    make gen_fault f=$fault_num s=250

    # Loop through s_line values 3 to 5
    for s_line in {3..5}; do
        # Run rdr_r with the current s_line and fault_num, and set rptName accordingly
        make rdr_r s=$s_line rptName="${s_line}_${s_line}_${fault_num}"
    done
done