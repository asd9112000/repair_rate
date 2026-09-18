#!/usr/bin/env python3
"""Bounded proof witness for S1G-A2D historical Hybrid-view capacities.

This isolated analysis tool models only the frozen default collector facts used
by the proof: 12 accepted faults, greedy pivot classification, one Hybrid
record per related fault, ConfigID pointer membership, and final Must
retirement.  It does not import, compile, or modify production RTL.
"""

import itertools
import json


MAX_FAULTS = 12
CONFIGS = {
    0: (2, 2, "2R2C"),
    1: (2, 1, "2R1C"),
    2: (3, 2, "3R2C"),
    3: (3, 1, "3R1C"),
    4: (1, 2, "1R2C"),
    5: (2, 3, "2R3C"),
    6: (1, 3, "1R3C"),
}


def enumerate_allocations(rows, cols, pivot_count):
    """Enumerate all no-Must Hybrid allocations for a relevant pivot prefix."""
    best = -1
    best_allocation = None
    for row_extra in itertools.product(range(cols), repeat=pivot_count):
        for col_extra in itertools.product(range(rows), repeat=pivot_count):
            hybrid_count = sum(row_extra) + sum(col_extra)
            if pivot_count + hybrid_count > MAX_FAULTS:
                continue
            if hybrid_count > best:
                best = hybrid_count
                best_allocation = (row_extra, col_extra)
    return best, best_allocation


def construct_and_evaluate(rows, cols, pivot_count, allocation):
    """Build a concrete accepted trace and evaluate frozen collector rules."""
    row_extra, col_extra = allocation
    pivots = []
    faults = []
    for index in range(pivot_count):
        pivot = (100 + index, 200 + index)
        pivots.append(pivot)
        faults.append(pivot)

    unique = 1000
    for index, (pivot_row, pivot_col) in enumerate(pivots):
        for _ in range(row_extra[index]):
            faults.append((pivot_row, unique))
            unique += 1
        for _ in range(col_extra[index]):
            faults.append((unique, pivot_col))
            unique += 1

    assert len(faults) <= MAX_FAULTS
    row_counts = {}
    col_counts = {}
    hybrid_records = []
    stored_pivots = []
    for fault_row, fault_col in faults:
        relation = None
        for pointer, (pivot_row, pivot_col) in enumerate(stored_pivots):
            if fault_row == pivot_row:
                relation = (pointer, 0)
                break
            if fault_col == pivot_col:
                relation = (pointer, 1)
                break
        row_counts[fault_row] = row_counts.get(fault_row, 0) + 1
        col_counts[fault_col] = col_counts.get(fault_col, 0) + 1
        if relation is None:
            stored_pivots.append((fault_row, fault_col))
        else:
            hybrid_records.append(relation)

    assert stored_pivots == pivots
    surviving = 0
    for pointer, descriptor in hybrid_records:
        pivot_row, pivot_col = pivots[pointer]
        must = row_counts[pivot_row] > cols if descriptor == 0 else col_counts[pivot_col] > rows
        if not must:
            surviving += 1
    return {
        "accepted_faults": len(faults),
        "hybrid_records": len(hybrid_records),
        "surviving_hybrids": surviving,
        "row_extra": list(row_extra),
        "col_extra": list(col_extra),
        "witness": [[row, col] for row, col in faults],
    }


def analyze_config(config_id, rows, cols, name):
    k = rows + cols
    symbolic_choices = []
    enumerated_best = -1
    enumerated_witness = None
    for pivot_count in range(1, k + 1):
        upper = min(MAX_FAULTS - pivot_count, pivot_count * (k - 2))
        symbolic_choices.append({"pivots": pivot_count, "upper": upper})
        allocation_best, allocation = enumerate_allocations(rows, cols, pivot_count)
        assert allocation_best == upper
        witness = construct_and_evaluate(rows, cols, pivot_count, allocation)
        assert witness["surviving_hybrids"] == allocation_best
        if allocation_best > enumerated_best:
            enumerated_best = allocation_best
            enumerated_witness = {"pivots": pivot_count, **witness}

    symbolic_maximum = max(choice["upper"] for choice in symbolic_choices)
    assert symbolic_maximum == enumerated_best
    return {
        "config_id": config_id,
        "geometry": name,
        "k": k,
        "symbolic_choices": symbolic_choices,
        "proven_maximum": symbolic_maximum,
        "witness": enumerated_witness,
    }


def main():
    results = [analyze_config(config_id, *CONFIGS[config_id]) for config_id in sorted(CONFIGS)]
    print(json.dumps({"max_faults": MAX_FAULTS, "configs": results}, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
