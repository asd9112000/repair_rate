
將你想跑的所有case run 過一次
./build/bin/HierarchicalRECAM 2 2   --canonical-four --write-area-manifest   --topology edge --shared-rows 1   --output-dir reports/area

根據需求將將所有case 放到
1. /cacti/CATTI_Input_Data
2. /OpenTCAM/OpenTCAM_Input_Data

執行script
1./cacti/scripts/run_catti_estimation.py