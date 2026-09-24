# RECAM_N2_2R2C verification

`recam_n2_2r2c_physical_column_test.cpp` instantiates the historical Phase-3A
analyzer and the corrected analyzer through a semantic adapter.  It proves that
physical columns 1 and 257 do not alias, covers 0, 31, 32, 255, 256, 257, 4095,
and 8191, and compares 1000 deterministic vectors limited to columns 0--31.

The historical Phase-3A interface has no separate ConfigID output.  The test
therefore compares its complete six-candidate validity vector in addition to
repairable and PatternID, which is stronger than comparing only the selected
first legal candidate.
