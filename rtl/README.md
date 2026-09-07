# RTL source boundary

This directory is reserved for synthesizable SystemVerilog. It is independent of the C++
system-level simulator in the repository root, `inc/`, and `src/`.

Planned source layout:

```text
rtl/
├── common/
│   ├── types_pkg.sv
│   └── interface_pkg.sv
├── primitives/
│   ├── cam/generic_cam.sv
│   ├── sram/sram_like.sv
│   ├── fifo/sync_fifo.sv
│   └── search/pway_search.sv
├── dss_2x2/
│   ├── common/
│   ├── cam/dss_cam_top.sv
│   └── sram_fifo/dss_sram_top.sv
├── recam/recam_top.sv
└── dss_1x4/future/
```

Directory placeholders do not represent implemented or verified RTL. Module creation and
phase advancement remain governed by `docs_verilog/02_PHASE_CONTROL.md` and
`docs_verilog/PHASE_STATUS.md`.
