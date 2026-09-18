#!/usr/bin/env python3
"""Audit model for S1G-A2D-R historical post-Must analyzer views.

The model intentionally mirrors only the relevant historical RTL contracts:
greedy row-before-column Pivot relation, ConfigID membership by pointer,
physical fault counters, final Config-specific Must predicates, and the
multi-config analyzer's per-entry retirement predicate.  It neither imports
nor modifies production RTL.
"""

import json


MAX_FAULTS = 12
MAX_K = 5
CONFIGS = {
    0: (2, 2, "2R2C"),
    1: (2, 1, "2R1C"),
    2: (3, 2, "3R2C"),
    3: (3, 1, "3R1C"),
    4: (1, 2, "1R2C"),
    5: (2, 3, "2R3C"),
    6: (1, 3, "1R3C"),
}


def collector_trace(faults):
    """Model the historical collector's relation and append-order Hybrid store."""
    assert len(faults) <= MAX_FAULTS
    pivots = []
    records = []
    row_counts = {}
    col_counts = {}
    for fault_index, (row, col) in enumerate(faults):
        relation = None
        for pointer, (pivot_row, pivot_col) in enumerate(pivots):
            if row == pivot_row:
                relation = (pointer, 0)
                break
            if col == pivot_col:
                relation = (pointer, 1)
                break
        row_counts[row] = row_counts.get(row, 0) + 1
        col_counts[col] = col_counts.get(col, 0) + 1
        if relation is None:
            assert len(pivots) < MAX_K
            pivots.append((row, col))
        else:
            pointer, descriptor = relation
            members = [config for config, (rows, cols, _) in CONFIGS.items()
                       if pointer < rows + cols]
            if members:
                records.append({
                    "physical_index": len(records),
                    "fault_index": fault_index,
                    "row": row,
                    "col": col,
                    "pointer": pointer,
                    "descriptor": "column" if descriptor else "row",
                    "cfg_valid": members,
                })
    return {
        "faults": [list(fault) for fault in faults],
        "pivots": [list(pivot) for pivot in pivots],
        "records": records,
        "row_counts": row_counts,
        "col_counts": col_counts,
    }


def view_for_config(state, config_id):
    """Return the exact membership and final analyzer views for one ConfigID."""
    rows, cols, _ = CONFIGS[config_id]
    row_must = []
    col_must = []
    for pivot_row, pivot_col in state["pivots"]:
        row_must.append(state["row_counts"][pivot_row] > cols)
        col_must.append(state["col_counts"][pivot_col] > rows)
    membership = []
    retired = []
    post_must = []
    for record in state["records"]:
        if config_id not in record["cfg_valid"]:
            continue
        membership.append(record["physical_index"])
        must = (col_must[record["pointer"]]
                if record["descriptor"] == "column"
                else row_must[record["pointer"]])
        if must:
            retired.append(record["physical_index"])
        else:
            post_must.append(record["physical_index"])
    return {
        "row_must": row_must,
        "col_must": col_must,
        "membership_indices": membership,
        "retired_indices": retired,
        "post_must_indices": post_must,
        "membership_count": len(membership),
        "post_must_count": len(post_must),
    }


def star_faults(pivot_count, row_extras, col_extras):
    """Build independent Pivot stars without accidental cross-Pivot relations."""
    faults = [(100 + index, 200 + index) for index in range(pivot_count)]
    next_value = 1000
    for index in range(pivot_count):
        for _ in range(row_extras[index]):
            faults.append((100 + index, next_value))
            next_value += 1
        for _ in range(col_extras[index]):
            faults.append((next_value, 200 + index))
            next_value += 1
    assert len(faults) <= MAX_FAULTS
    return faults


def bounded_compositions(total_limit, dimensions):
    """Yield nonnegative vectors with sum no greater than total_limit."""
    current = [0] * dimensions

    def visit(index, remaining):
        if index == dimensions:
            yield tuple(current)
            return
        for value in range(remaining + 1):
            current[index] = value
            yield from visit(index + 1, remaining - value)

    yield from visit(0, total_limit)


def capacity_envelope(config_id):
    """Exhaust all independent-star allocations and check the symbolic upper bound."""
    rows, cols, name = CONFIGS[config_id]
    k = rows + cols
    state_count = 0
    membership_gt9 = 0
    post_must_gt9 = 0
    maximum_membership = -1
    maximum_post_must = -1
    membership_witness = None
    post_must_witness = None
    per_pivot_upper_bounds = []
    for pivot_count in range(1, k + 1):
        budget = MAX_FAULTS - pivot_count
        upper_bound = min(budget, pivot_count * ((cols - 1) + (rows - 1)))
        per_pivot_upper_bounds.append({"pivots": pivot_count, "upper_bound": upper_bound})
        for allocation in bounded_compositions(budget, 2 * pivot_count):
            row_extras = allocation[:pivot_count]
            col_extras = allocation[pivot_count:]
            state = collector_trace(star_faults(pivot_count, row_extras, col_extras))
            view = view_for_config(state, config_id)
            state_count += 1
            if view["membership_count"] > 9:
                membership_gt9 += 1
            if view["post_must_count"] > 9:
                post_must_gt9 += 1
            if view["membership_count"] > maximum_membership:
                maximum_membership = view["membership_count"]
                membership_witness = {
                    "pivots": pivot_count,
                    "row_extras": list(row_extras),
                    "col_extras": list(col_extras),
                }
            if view["post_must_count"] > maximum_post_must:
                maximum_post_must = view["post_must_count"]
                post_must_witness = {
                    "pivots": pivot_count,
                    "row_extras": list(row_extras),
                    "col_extras": list(col_extras),
                }
    symbolic_maximum = max(item["upper_bound"] for item in per_pivot_upper_bounds)
    assert maximum_post_must == symbolic_maximum
    assert post_must_gt9 == 0
    return {
        "config_id": config_id,
        "geometry": name,
        "abstract_allocation_states_explored": state_count,
        "per_pivot_upper_bounds": per_pivot_upper_bounds,
        "max_membership_visible": maximum_membership,
        "membership_witness": membership_witness,
        "max_post_must_analyzer_visible": maximum_post_must,
        "post_must_witness": post_must_witness,
        "membership_visible_gt9_states": membership_gt9,
        "post_must_visible_gt9_states": post_must_gt9,
    }


def directed_cases():
    """Exercise the required predicate distinctions with legal collector traces."""
    cases = {}
    row_counterexample = collector_trace([(0, 0)] + [(0, col) for col in range(1, 12)])
    column_counterexample = collector_trace([(0, 0)] + [(row, 0) for row in range(1, 12)])
    mixed = collector_trace([(0, 0), (0, 1), (1, 0)])
    membership_holes = collector_trace([
        (10, 20), (11, 21), (12, 22), (13, 23),
        (13, 100), (13, 101),
    ])
    config2_max = collector_trace(star_faults(3, (1, 1, 1), (2, 2, 2)))
    config5_max = collector_trace(star_faults(3, (2, 2, 2), (1, 1, 1)))
    no_retirement = collector_trace([(0, 0), (0, 1), (1, 0)])
    for label, state in {
        "DR1_row_membership_counterexample": row_counterexample,
        "DR2_column_membership_counterexample": column_counterexample,
        "DR3_mixed_row_and_column_must": mixed,
        "DR4_membership_holes_and_must": membership_holes,
        "DR5_config2_post_must_nine": config2_max,
        "DR6_config5_post_must_nine": config5_max,
        "DR7_membership_gt9_post_must_lte9": row_counterexample,
        "DR8_no_must_retirement": no_retirement,
    }.items():
        cases[label] = {
            "physical_hybrid_entries": state["records"],
            "by_config": {str(config): view_for_config(state, config) for config in CONFIGS},
        }
    for config in CONFIGS:
        assert cases["DR1_row_membership_counterexample"]["by_config"][str(config)]["membership_count"] == 11
        assert cases["DR1_row_membership_counterexample"]["by_config"][str(config)]["post_must_count"] == 0
        assert cases["DR2_column_membership_counterexample"]["by_config"][str(config)]["membership_count"] == 11
        assert cases["DR2_column_membership_counterexample"]["by_config"][str(config)]["post_must_count"] == 0
    assert cases["DR5_config2_post_must_nine"]["by_config"]["2"]["post_must_count"] == 9
    assert cases["DR6_config5_post_must_nine"]["by_config"]["5"]["post_must_count"] == 9
    assert cases["DR8_no_must_retirement"]["by_config"]["0"]["membership_count"] == 2
    assert cases["DR8_no_must_retirement"]["by_config"]["0"]["post_must_count"] == 2
    return cases


def main():
    envelopes = [capacity_envelope(config) for config in sorted(CONFIGS)]
    expected = [8, 3, 9, 8, 3, 9, 8]
    assert [entry["max_post_must_analyzer_visible"] for entry in envelopes] == expected
    assert [entry["max_membership_visible"] for entry in envelopes] == [11] * len(CONFIGS)
    print(json.dumps({
        "model": "historical collector membership plus final Must retirement",
        "max_faults": MAX_FAULTS,
        "configs": envelopes,
        "total_abstract_allocation_states_explored": sum(
            entry["abstract_allocation_states_explored"] for entry in envelopes),
        "directed_cases": directed_cases(),
    }, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
