# Continuous-analysis cycle semantics

The measured origin is the rising edge that accepts a captured analyzer-state
generation. EARLY scans the frozen canonical priority and becomes ready on the
first analyzer-valid and prefix-compatible candidate edge. GROUP captures four
final-SA candidate records, then captures the static-selector result one edge
later. BIST-done waiting is excluded from both metrics.

The raw fault-to-state-update edge is not implemented by these CA top wrappers.
Thus EARLY `2..5` and GROUP `6` cycles are architectural black-box estimates,
not direct RTL measurements.
