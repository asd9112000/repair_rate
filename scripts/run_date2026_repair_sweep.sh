#!/usr/bin/env bash
# Unified DATE2026 repair-rate entry point.  Scope dispatch only: group and
# device backends intentionally retain separate simulator/resource semantics.
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: scripts/run_date2026_repair_sweep.sh --scope group|device [options]

The group backend is the validated DynamicSpareSharing/R3 pipeline.  The
device backend is a guarded interface stub; it never substitutes group data.
Run with --scope group --help or --scope device --help for scope details.
EOF
}

scope=""
forward=()
while (($#)); do
    case "$1" in
        --scope)
            (($# >= 2)) || { echo "--scope requires group or device" >&2; exit 2; }
            scope="$2"; shift 2 ;;
        --help|-h) forward+=("$1"); shift ;;
        *) forward+=("$1"); shift ;;
    esac
done

case "$scope" in
    group) exec "$(dirname "$0")/run_date2026_group_sweep.sh" "${forward[@]}" ;;
    device) exec "$(dirname "$0")/run_date2026_device_sweep.sh" "${forward[@]}" ;;
    *) usage >&2; exit 2 ;;
esac
