#!/usr/bin/env bash
# DATE2026 group-scope adapter for the frozen R3 backend.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
rs=""; cs=""; share_m=""; groups=""; seed=""; f_groups=""
output_root=""; mode="custom"; resume=0; dry_run=0; policies="canonical"; topologies="canonical"

usage() {
    cat <<'EOF'
Usage: scripts/run_date2026_repair_sweep.sh --scope group [options]
  --rs N --cs N [--share-m canonical_m1_m2] --groups N --f-group-list LIST
  --seed N --policies canonical|sixcase_static --topologies canonical|static --output-root PATH
  --preflight | --formal --resume --dry-run

Use `sixcase_static` with `static` for the N=2 final-archive six-case preflight.
EOF
}

while (($#)); do
    case "$1" in
        --rs) rs="$2"; shift 2 ;; --cs) cs="$2"; shift 2 ;;
        --share-m) share_m="$2"; shift 2 ;; --groups) groups="$2"; shift 2 ;;
        --seed) seed="$2"; shift 2 ;; --f-group-list) f_groups="$2"; shift 2 ;;
        --policies) policies="$2"; shift 2 ;; --topologies) topologies="$2"; shift 2 ;;
        --output-root) output_root="$2"; shift 2 ;; --resume) resume=1; shift ;;
        --dry-run) dry_run=1; shift ;; --preflight) mode="preflight"; shift ;;
        --formal) mode="formal"; shift ;; --help|-h) usage; exit 0 ;;
        --devices|--groups-per-device|--banks-per-device|--cam-model|--cam-capacity|--device-policy)
            echo "$1 is device-scope only; use --scope device" >&2; exit 2 ;;
        *) echo "unknown group option: $1" >&2; exit 2 ;;
    esac
done

[[ ("$policies" == "canonical" && "$topologies" == "canonical") ||
   ("$policies" == "sixcase_static" && "$topologies" == "static") ]] || {
    echo "GROUP_NOT_READY: use canonical/canonical or sixcase_static/static" >&2; exit 2; }
if [[ "$policies" == "sixcase_static" ]]; then
    [[ -z "$share_m" || "$share_m" == "static_m1" ]] || {
        echo "GROUP_NOT_READY: sixcase_static uses fixed m=1 contracts; omit --share-m or use static_m1" >&2; exit 2; }
else
    [[ -z "$share_m" || "$share_m" == "canonical_m1_m2" ]] || {
        echo "GROUP_NOT_READY: R3 Matrix V2 uses canonical m=1 and m=2 policies; omit --share-m or use canonical_m1_m2" >&2; exit 2; }
fi
[[ -z "$rs" || -z "$cs" || "$rs" == "$cs" ]] || { echo "--rs and --cs must match" >&2; exit 2; }
[[ "$policies" != "sixcase_static" || ("$rs" == "2" && "$cs" == "2") ]] || {
    echo "GROUP_NOT_READY: sixcase_static is N=2-only final-archive evidence" >&2; exit 2; }

if [[ -z "$output_root" ]]; then
    output_root="$root/results/date2026/repair_rate/group/$mode"
fi
command=(python3 "$root/scripts/group/r3_formal_group_repair_rate.py" --output-root "$output_root")
[[ "$policies" == "sixcase_static" ]] && command+=(--policy-preset sixcase_static)
[[ -n "$groups" ]] && command+=(--samples "$groups")
[[ -n "$seed" ]] && command+=(--master-seed "$seed")
[[ -n "$rs" ]] && command+=(--rs "$rs" --cs "$cs")
[[ -n "$f_groups" ]] && command+=(--f-group-list "$f_groups")
command+=(--mode "$mode")
if ((resume)); then
    command+=(--resume)
fi
if ((dry_run)); then
    cat <<EOF
scope=group
backend=$root/scripts/group/r3_formal_group_repair_rate.py
mode=$mode
rs=${rs:-default}
cs=${cs:-default}
share_m=${share_m:-canonical_m1_m2}
f_group_list=${f_groups:-default}
groups=${groups:-100000}
seed=${seed:-20260922}
policies=$policies
topologies=$topologies
output_root=$output_root
resume=$resume
command=$(printf '%q ' "${command[@]}")
EOF
    exit 0
fi
exec "${command[@]}"
