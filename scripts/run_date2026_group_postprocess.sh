#!/usr/bin/env bash
# DATE2026 group-level derived-data and figure entry point.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
input_root="$root/tmp/date2026/6case_1k"
output_root="$root/results/date2026/repair_rate/group/postprocess"
policy_set="sixcase_static"
n_list="2,3,4"
mode="all"
include_local=0

usage() {
    cat <<'USAGE'
Usage: scripts/run_date2026_group_postprocess.sh [options]
  --input-root PATH       Root containing n2/, n3/, and n4/ raw sweep results
  --output-root PATH      Separate derived-data root (never writes raw sidecars)
  --policy-set NAME       legacy or sixcase_static (default: sixcase_static)
  --n-list LIST           Comma-separated N values (default: 2,3,4)
  --include-local          Include LOCAL on the six-case repair-rate figure
  --analysis-only | --plot-only | --all
USAGE
}

while (($#)); do
    case "$1" in
        --input-root) input_root="$2"; shift 2 ;;
        --output-root) output_root="$2"; shift 2 ;;
        --policy-set) policy_set="$2"; shift 2 ;;
        --n-list) n_list="$2"; shift 2 ;;
        --include-local) include_local=1; shift ;;
        --analysis-only) mode="analysis"; shift ;;
        --plot-only) mode="plot"; shift ;;
        --all) mode="all"; shift ;;
        --help|-h) usage; exit 0 ;;
        *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done
case "$policy_set" in legacy|sixcase_static) ;; *) echo "unknown policy set: $policy_set" >&2; exit 2;; esac

IFS=',' read -r -a ns <<<"$n_list"
for n in "${ns[@]}"; do
    [[ "$n" =~ ^[0-9]+$ ]] || { echo "invalid N: $n" >&2; exit 2; }
    input="$input_root/n$n"
    output="$output_root/$policy_set/n$n"
    [[ -d "$input" ]] || { echo "missing input root: $input" >&2; exit 2; }
    if [[ "$mode" == "all" || "$mode" == "analysis" ]]; then
        for analyzer in analyze_group_repair_rate analyze_policy_pairs analyze_fault_imbalance analyze_search_complexity; do
            python3 "$root/scripts/analysis/r3_group/$analyzer.py" --input-root "$input" --output-root "$output" --policy-set "$policy_set"
        done
    fi
    if [[ "$mode" == "all" || "$mode" == "plot" ]]; then
        [[ -f "$output/data/repair_rate_summary.csv" ]] || { echo "missing analysis output for N=$n; run --analysis-only first" >&2; exit 2; }
        if ((include_local)); then
            python3 "$root/scripts/plot/r3_group/plot_group_repair_rate.py" --analysis-root "$output" --policy-set "$policy_set" --n "$n" --include-local
        else
            python3 "$root/scripts/plot/r3_group/plot_group_repair_rate.py" --analysis-root "$output" --policy-set "$policy_set" --n "$n"
        fi
        python3 "$root/scripts/plot/r3_group/plot_policy_pairs.py" --analysis-root "$output" --policy-set "$policy_set"
        python3 "$root/scripts/plot/r3_group/plot_repair_rate_gain.py" --analysis-root "$output" --policy-set "$policy_set"
        python3 "$root/scripts/plot/r3_group/plot_fault_imbalance.py" --analysis-root "$output" --policy-set "$policy_set"
        python3 "$root/scripts/plot/r3_group/plot_search_complexity.py" --analysis-root "$output" --policy-set "$policy_set"
    fi
    echo "N=$n output: $output"
done
