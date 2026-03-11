#!/bin/bash

rm -f SharedLine.log
rm -f ./SharedLineReport/repairRates.csv

# Loop through fault numbers 8 to 14
for fault_num in {9..15}; do
    # Generate faults with the current fault number and stack height 250
    make gen_fault f=$fault_num s=250

    # Loop through s_line values 3 to 5
    for s_line in {3..5}; do
        # Run rdr_r with the current s_line and fault_num, and set rptName accordingly
        # ./SharedLine.o $s_line $s_line --rptName "RepairReport_r${s_line}_c${s_line}_f${fault_num}.rpt" > SharedLine.log 2>&1
        ./SharedLine.o $s_line $s_line --rptName "SL_RepairReport_r${s_line}_c${s_line}_f${fault_num}.rpt" --faultNum $fault_num >> SharedLine.log 2>&1
    done
done

# 模擬結束後從 SharedLine.log 中提取 RepairRate 和 RepairRate_RECAM 的數值，並將它們保存到 repair_rates.csv 文件中。
echo "FaultNum,SpareLine,RepairRate,RepairRate_RECAM" >> ./SharedLineReport/repairRates.csv
grep "RepairRate:" SharedLine.log | while read -r line; do
    fault_num=$(echo "$line" | grep -oP 'faultNum: \K\d+')
    spare_line=$(echo "$line" | grep -oP 'SpareLine: \K\d+')
    repair_rate=$(echo "$line" | grep -oP 'RepairRate: \K[0-9.]+')
    repair_rate_recam=$(echo "$line" | grep -oP 'RepairRate_RECAM: \K[0-9.]+')
    echo "$fault_num,$spare_line,$repair_rate,$repair_rate_recam" >> ./SharedLineReport/repairRates.csv
done