#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd);cd "$root"
scripts/simulation/run_verilator_test.sh recam_dss_group_allocator rtl/recam/recam_dss_group_allocator.sv tb/recam/recam_dss_group_allocator_test.cpp rtl/recam/recam_resource_aware_fallback_selector.sv rtl/recam/recam_physical_resource_ledger_checker.sv rtl/recam/recam_config_resource_action_decoder.sv
scripts/simulation/run_verilator_test.sh recam_dss_group_allocator rtl/recam/recam_dss_group_allocator.sv tb/recam/recam_dss_group_allocator_random_test.cpp rtl/recam/recam_resource_aware_fallback_selector.sv rtl/recam/recam_physical_resource_ledger_checker.sv rtl/recam/recam_config_resource_action_decoder.sv
