本專案的 Synopsys Design Compiler 已確認可用。

工作目錄：
/home/asd9112000/repair_rate

請勿安裝或修改 EDA 軟體，也不要自行設定 license server。

在非互動 shell 中，使用現有的 site setup：

tcsh -f -c 'source /cad/synopsys/CIC/synthesis.cshrc; which dc_shell; dc_shell -version'

預期結果：
dc_shell 路徑：
/usr/cad/synopsys/synthesis/2024.09-sp2/amd64/syn/bin/dc_shell

版本：
W-2024.09-SP2

注意：dc_shell -version 只驗證 binary 和環境，不代表 synthesis feature 已成功 checkout。必須實際執行小型或正式 synthesis 才能確認授權。

執行 repository 既有 synthesis runner 時，runner 本身會載入：
/cad/synopsys/CIC/synthesis.cshrc

若希望工作不受 Codex 回合或流量限制中斷，請使用 detached tmux，並將 stdout/stderr 寫入 runner.log。Codex 沙箱可能禁止建立 tmux socket；遇到 Operation not permitted 時，應申請執行 tmux 的外部權限，不要修改 EDA 或 license 設定。

可參考的已成功 runner：
bash scripts/synthesis/run_recam_canonical_rs2_early_dc.sh
bash scripts/synthesis/run_recam_canonical_rs2_global_noscratch_dc.sh

兩者一起執行：
bash scripts/synthesis/run_recam_canonical_rs2_all_dc.sh

成功判定不能只看 dc_shell -version；必須確認結果目錄的 metadata.txt 含：
STATUS=PASS

並確認至少存在：
area.rpt
timing.rpt
qos.rpt
gate_count.rpt
hierarchy_area.rpt
runner.log
dc_shell.log

已確認成功的環境：
Synopsys Design Compiler W-2024.09-SP2
TSMC018 Arm CBDK slow.db
slow corner
20.0 ns clock
zero I/O delay
compile -map_effort low
NAND2X1 area = 9.979200