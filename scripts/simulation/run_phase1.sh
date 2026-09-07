#!/usr/bin/env bash
set -euo pipefail

runner=scripts/simulation/run_verilator_test.sh

"$runner" generic_cam \
    rtl/primitives/cam/generic_cam.sv \
    tb/primitives/generic_cam_test.cpp \
    -GENTRY_NUM=4 -GKEY_WIDTH=8 -GDATA_WIDTH=8

"$runner" generic_cam \
    rtl/primitives/cam/generic_cam.sv \
    tb/primitives/generic_cam_test.cpp \
    -GENTRY_NUM=7 -GKEY_WIDTH=8 -GDATA_WIDTH=8

"$runner" sram_like \
    rtl/primitives/sram/sram_like.sv \
    tb/primitives/sram_like_test.cpp \
    -GENTRY_NUM=5 -GDATA_WIDTH=16

"$runner" sram_like \
    rtl/primitives/sram/sram_like.sv \
    tb/primitives/sram_like_test.cpp \
    -GENTRY_NUM=7 -GDATA_WIDTH=16

"$runner" sync_fifo \
    rtl/primitives/fifo/sync_fifo.sv \
    tb/primitives/sync_fifo_test.cpp \
    -GDEPTH=3 -GDATA_WIDTH=8

"$runner" pway_search \
    rtl/primitives/search/pway_search.sv \
    tb/primitives/pway_search_test.cpp \
    -GENTRY_NUM=5 -GENTRY_WIDTH=8 -GP=1 -- 5

"$runner" pway_search \
    rtl/primitives/search/pway_search.sv \
    tb/primitives/pway_search_test.cpp \
    -GENTRY_NUM=5 -GENTRY_WIDTH=8 -GP=2 -- 3

"$runner" pway_search \
    rtl/primitives/search/pway_search.sv \
    tb/primitives/pway_search_test.cpp \
    -GENTRY_NUM=5 -GENTRY_WIDTH=8 -GP=4 -- 2

echo "Phase 1 Verilator regression PASS"
