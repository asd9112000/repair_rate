python3 scripts/group/r3_formal_group_repair_rate.py   --f-group-list 8,12,16,20,24,28,32,36,44,48   --samples 300   --mode custom   --output-root 01_my_note/repair_rate_300_0919_v2



# run
set -euo pipefail

cd /home/asd9112000/repair_rate_date2026_canonical
root="tmp/date2026/6case_10k"
faults="8,12,16,20,24,28,32,36,40,44,48"

for n in 2 3 4; do
  extra=()
  [[ -d "$root/n$n/manifest" ]] && extra+=(--resume)

  scripts/run_date2026_group_sweep.sh \
    --rs "$n" --cs "$n" \
    --share-m 1 \
    --groups 10000 \
    --f-group-list "$faults" \
    --seed 20260922 \
    --policies sixcase_static \
    --topologies static \
    --preflight \
    --output-root "$root/n$n" \
    "${extra[@]}"
done

# analysis
./scripts/analysis_date2026_group_sweep.sh \
  --input-root tmp/date2026/6case_1k \
  --output-root date2026/6case_1k
  --policy-set sixcase_static \
  --n-list 2,3,4

# plot
./scripts/plot_date2026_group_sweep.sh \
  --analysis-root results/date2026/6case_1k \
  --policy-set sixcase_static \
  --n-list 2,3,4 \
  --include-local