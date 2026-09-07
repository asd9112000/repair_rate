# RTL testbench boundary

RTL unit tests, architecture regressions, fixtures, and HDL simulation-only helpers belong
here. C++ and Python simulator regression tests remain in `tests/`.

```text
tb/
├── primitives/
├── dss_2x2/
├── recam/
└── common/
```

RTL comparisons may consume versioned traces exported by the C++ simulator, but testbench
outputs must be written to `results/`, not `reports/`.
