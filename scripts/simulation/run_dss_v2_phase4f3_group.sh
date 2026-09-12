#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd); cd "$root"
scripts/simulation/run_verilator_test.sh recam_dss_v2_group_core rtl/dss_v2/top/recam_dss_v2_group_core.sv tb/dss_v2/recam_dss_v2_group_core_test.cpp rtl/dss_v2/common/dss_v2_params_pkg.sv rtl/dss_v2/common/dss_v2_types_pkg.sv rtl/dss_v2/group/dss_v2_group_candidate_store.sv rtl/dss_v2/group/dss_v2_group_slot_decode.sv rtl/dss_v2/group/dss_v2_group_priority_reader.sv rtl/dss_v2/topology/dss_topology_2x2_directional.sv rtl/dss_v2/resource/dss_v2_resource_feasibility.sv rtl/dss_v2/resource/dss_v2_resource_ledger.sv rtl/dss_v2/adapter/dss_v2_legacy_ledger_diagnostic_adapter.sv
