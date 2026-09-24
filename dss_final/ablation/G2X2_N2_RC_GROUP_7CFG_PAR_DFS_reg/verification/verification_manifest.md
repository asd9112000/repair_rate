# Functional verification manifest

- Strict Verilator lint: full normalized-ablation top and lockstep wrapper, with `-Wall -Werror-WIDTH`.
- Lockstep: seed 1, ten smoke vectors, and 1000 full vectors; exact repairability, ConfigID, PatternID, action, donor, release/borrow, line validity/type, and final physical addresses.
- Address gate: existing `physical_column_test.cpp`, using the analyzer instantiated by all seven lanes; verifies 1 != 257 and 0/31/32/255/256/257/4095/8191.
- Dense-map producer regression: seven lanes, 16 writes, map index boundaries, and release/borrow replication.

DC synthesis is intentionally excluded from this functional freeze.
