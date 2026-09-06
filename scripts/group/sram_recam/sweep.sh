#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_ROOT=$(cd -- "${SCRIPT_DIR}/../../.." && pwd)

# Run the default 4-SA SRAM-RECAM policy comparison.
"${PROJECT_ROOT}/build/bin/DynamicSpareSharing_SRAM_RECAM" 2 2 \
  --runs 1000 \
  --fault-count 20 \
  --seed 20260820 \
  --topology directional \
  --shared-lines 1 \
  --local-first \
  --policies serial,chunk2,chunk4,wide \
  --output-dir "${PROJECT_ROOT}/reports/group/sram_recam" \
  --overwrite
