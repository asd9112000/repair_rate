#!/usr/bin/env bash
# Reserved DATE2026 device-scope interface.  No group backend is ever called.
set -euo pipefail
if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
    cat <<'EOF'
Usage: scripts/run_date2026_repair_sweep.sh --scope device [shared options]

Reserved options: --rs --cs --share-m --groups --seed --policies --topologies
--f-group-list --output-root --resume --dry-run --preflight --formal --devices
--groups-per-device --banks-per-device --cam-model --cam-capacity --device-policy.

DEVICE_NOT_READY: no DATE2026 device simulation contract is frozen by this
runner. This command performs no simulation and writes no result artifact.
EOF
    exit 0
fi
echo "DEVICE_NOT_READY: DATE2026 device sweep is a reserved interface only; no device backend was launched." >&2
exit 3
