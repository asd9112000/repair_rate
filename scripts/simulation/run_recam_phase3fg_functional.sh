#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd); cd "$root"
scripts/simulation/run_verilator_test.sh recam_physical_resource_ledger_checker rtl/recam/recam_physical_resource_ledger_checker.sv tb/recam/recam_ledger_checker_test.cpp
scripts/simulation/run_verilator_test.sh recam_config_resource_action_decoder rtl/recam/recam_config_resource_action_decoder.sv tb/recam/recam_resource_action_decoder_test.cpp
scripts/simulation/run_verilator_test.sh recam_resource_aware_fallback_selector rtl/recam/recam_resource_aware_fallback_selector.sv tb/recam/recam_resource_fallback_test.cpp rtl/recam/recam_physical_resource_ledger_checker.sv rtl/recam/recam_config_resource_action_decoder.sv
