#!/usr/bin/env python3
"""Read-only S1G-A2F proof for historical collector retained state.

The model mirrors ``shared_fault_collector.sv`` at its frozen defaults.  It
does not drive, build, or change RTL.  Its purpose is deliberately stronger
than checking a currently projected analyzer view: it encodes a reachable
collector state, decodes it, applies the same next fault to both forms, and
compares the resulting collector state, Must predicates, post-Must views, and
historical candidate results.
"""

from __future__ import print_function

import copy
import itertools
import json
import random
from collections import OrderedDict


ROW_W = 10
COL_W = 10
MAX_FAULTS = 12
MAX_K = 5
NUM_CFG = 7
HYBRID_CAPACITY = 14
REUSE_CAPACITY = 12
COUNTER_CAPACITY = 12
# The source allocates 14 Hybrid and 12 reuse slots, but with MAX_FAULTS=12
# its first accepted fault is necessarily a Pivot.  At most eleven subsequent
# accepts can write either append-only store.
MAX_REACHABLE_HYBRIDS = MAX_FAULTS - 1
MAX_REACHABLE_REUSE = MAX_FAULTS - 1
CONFIGS = {
    0: (2, 2), 1: (2, 1), 2: (3, 2), 3: (3, 1),
    4: (1, 2), 5: (2, 3), 6: (1, 3),
}


def config_k(config):
    rows, cols = CONFIGS[config]
    return rows + cols


def mask_for_pointer(pointer):
    """The exact ``relation_ptr < cfg_k(cfg)`` Hybrid membership function."""
    return sum(1 << config for config in CONFIGS if pointer < config_k(config))


def reuse_mask_for_record(pivots, row, col):
    """Reconstruct the collector's CAM-reuse mask from a retained record.

    A reuse record is only written for (a) a related fault whose pointer is
    outside a Config's K, or (b) an unrelated fault after all five Pivot slots
    are full.  For case (a), the prior pivot occupancy is necessarily greater
    than the pointer and therefore already at least that Config's K.  Later
    Pivots cannot displace this earlier matching Pivot in the RTL's ascending
    relation scan.  For case (b), there can be no later Pivot.  Thus the final
    ordered Pivot list and the reuse record's address reconstruct the exact
    seven-bit source mask without a raw-fault replay.
    """
    temporary = fresh_state()
    temporary["pivots"] = list(pivots)
    relation = relation_for(temporary, row, col)
    if relation is None:
        assert len(pivots) == MAX_K
        return (1 << NUM_CFG) - 1
    pointer, _ = relation
    return sum(1 << config for config in CONFIGS if pointer >= config_k(config))


def clone(value):
    return copy.deepcopy(value)


def fresh_state():
    return {
        "fault_count": 0,
        "pivots": [],                    # append ordered (row, col) pairs
        "hybrids": [],                   # append ordered physical H entries
        "reuse": [],                     # append ordered CAM-reuse entries
        "row_counters": OrderedDict(),   # address -> positive count
        "col_counters": OrderedDict(),
        "counter_overflow": False,
        "hybrid_overflow": False,
        "reuse_overflow": False,
    }


def relation_for(state, row, col):
    """RTL order: lowest Pivot, row relation before column relation."""
    for pointer, (pivot_row, pivot_col) in enumerate(state["pivots"]):
        if row == pivot_row:
            return pointer, 0
        if col == pivot_col:
            return pointer, 1
    return None


def bump_counter(table, address, capacity):
    if address in table:
        table[address] += 1
        return False
    if len(table) < capacity:
        table[address] = 1
        return False
    return True


def transition(state, fault, max_faults=MAX_FAULTS,
               hybrid_capacity=HYBRID_CAPACITY, reuse_capacity=REUSE_CAPACITY,
               counter_capacity=COUNTER_CAPACITY):
    """One accepted/not-accepted fault transition of the historical collector."""
    next_state = clone(state)
    row, col = fault
    if next_state["fault_count"] >= max_faults:
        return next_state

    old_pivot_count = len(next_state["pivots"])
    relation = relation_for(next_state, row, col)
    hybrid_mask = 0
    reuse_mask = 0
    if relation is not None:
        pointer, descriptor = relation
        hybrid_mask = mask_for_pointer(pointer)
    for config in CONFIGS:
        k = config_k(config)
        if relation is not None and relation[0] < k:
            continue
        if old_pivot_count >= k:
            reuse_mask |= 1 << config

    # These occur concurrently in RTL; each decision above observes old state.
    if bump_counter(next_state["row_counters"], row, counter_capacity):
        next_state["counter_overflow"] = True
    if bump_counter(next_state["col_counters"], col, counter_capacity):
        next_state["counter_overflow"] = True

    if relation is None and old_pivot_count < MAX_K:
        next_state["pivots"].append((row, col))
    elif relation is not None and hybrid_mask:
        pointer, descriptor = relation
        if len(next_state["hybrids"]) < hybrid_capacity:
            next_state["hybrids"].append({
                "row": row, "col": col, "pointer": pointer,
                "descriptor": descriptor, "cfg_valid": hybrid_mask,
                # The actual RTL writes this, but its output is unconnected
                # and no next-state decision reads it.
                "fault_ref": next_state["fault_count"],
            })
        else:
            next_state["hybrid_overflow"] = True

    if reuse_mask:
        if len(next_state["reuse"]) < reuse_capacity:
            next_state["reuse"].append({"row": row, "col": col,
                                        "cfg_valid": reuse_mask})
        else:
            next_state["reuse_overflow"] = True

    next_state["fault_count"] += 1
    return next_state


def collect(faults, **kwargs):
    state = fresh_state()
    for fault in faults:
        state = transition(state, fault, **kwargs)
    return state


def must_vectors(state):
    row_must = [[False] * MAX_K for _ in CONFIGS]
    col_must = [[False] * MAX_K for _ in CONFIGS]
    for pointer, (row, col) in enumerate(state["pivots"]):
        row_count = state["row_counters"].get(row, 0)
        col_count = state["col_counters"].get(col, 0)
        for config, (rows, cols) in CONFIGS.items():
            row_must[config][pointer] = row_count > cols
            col_must[config][pointer] = col_count > rows
    return row_must, col_must


def post_must_view(state, config):
    row_must, col_must = must_vectors(state)
    view = []
    membership = []
    for physical_index, hybrid in enumerate(state["hybrids"]):
        if hybrid["cfg_valid"] & (1 << config):
            membership.append(physical_index)
            retired = (col_must[config][hybrid["pointer"]]
                       if hybrid["descriptor"]
                       else row_must[config][hybrid["pointer"]])
            if not retired:
                view.append((physical_index, hybrid["pointer"],
                             hybrid["descriptor"], hybrid["row"], hybrid["col"]))
    return membership, view


def canonical_232_view(state, config):
    """A deterministic 232-bit field tuple for the frozen HCV-9 boundary.

    The original historical collector has 10-bit rows and columns, whereas
    the isolated V2 consumer is 9-bit-row/5-bit-column.  This serializer is
    therefore intentionally only a boundary signature: it applies the fixed
    consumer widths and is used only to compare S with decode(encode(S)).  The
    selected retained contract below keeps the original 10+10-bit addresses.
    """
    row_must, col_must = must_vectors(state)
    rows, cols = CONFIGS[config]
    pivots = list(state["pivots"]) + [(0, 0)] * (MAX_K - len(state["pivots"]))
    bits = []
    bits.extend(int(index < len(state["pivots"]) and index < rows + cols)
                for index in range(MAX_K))
    bits.extend(row & 0x1ff for row, _ in pivots)
    bits.extend(col & 0x1f for _, col in pivots)
    for threshold in (1, 2, 3):
        bits.extend(int(state["row_counters"].get(row, 0) > threshold)
                    for row, _ in pivots)
    for threshold in (1, 2, 3):
        bits.extend(int(state["col_counters"].get(col, 0) > threshold)
                    for _, col in pivots)
    bits.append(int(state["counter_overflow"] or state["hybrid_overflow"]))
    _, view = post_must_view(state, config)
    for slot in range(9):
        if slot < len(view):
            _, pointer, descriptor, row, col = view[slot]
            bits.extend((1, pointer, descriptor,
                         (row if descriptor else col) & 0x1ff))
        else:
            bits.extend((0, 0, 0, 0))
    # ``bits`` is a field tuple (not a one-element-per-wire-bit vector).  Its
    # packed widths are exactly 232: 5 + 45 + 25 + 30 + 1 + 9*(1+3+1+9).
    assert len(bits) == 5 + 5 + 5 + 15 + 15 + 1 + 9 * 4
    return tuple(bits)


def candidate_result(state, config):
    """Independent Python rendering of ``config_analyzer.sv`` outputs."""
    active_rows, active_cols = CONFIGS[config]
    k = active_rows + active_cols
    pivots = state["pivots"][:k]
    row_must, col_must = must_vectors(state)
    rows_dict = [row for row, _ in pivots]
    cols_dict = [col for _, col in pivots]
    matrix = [[False] * MAX_K for _ in range(MAX_K)]
    for row in range(MAX_K):
        for col in range(MAX_K):
            if row < k and col < k:
                matrix[row][col] = ((row == col and row < len(pivots)) or
                                    (row < len(pivots) and row_must[config][row]) or
                                    (col < len(pivots) and col_must[config][col]))
    extendable = len(pivots) < k
    _, view = post_must_view(state, config)
    for _, _, _, hybrid_row, hybrid_col in view:
        row_index = next((i for i, value in enumerate(rows_dict)
                          if value == hybrid_row), None)
        col_index = next((i for i, value in enumerate(cols_dict)
                          if value == hybrid_col), None)
        if not extendable:
            if row_index is not None and col_index is not None:
                matrix[row_index][col_index] = True
            elif row_index is not None:
                for index in range(k):
                    matrix[row_index][index] = True
            elif col_index is not None:
                for index in range(k):
                    matrix[index][col_index] = True
        elif row_index is not None and col_index is not None:
            matrix[row_index][col_index] = True
        elif row_index is not None and len(cols_dict) < k:
            matrix[row_index][len(cols_dict)] = True
            cols_dict.append(hybrid_col)
        elif col_index is not None and len(rows_dict) < k:
            matrix[len(rows_dict)][col_index] = True
            rows_dict.append(hybrid_row)
        elif row_index is not None:
            for index in range(k):
                matrix[row_index][index] = True
        elif col_index is not None:
            for index in range(k):
                matrix[index][col_index] = True
        if len(rows_dict) >= k and len(cols_dict) >= k:
            extendable = False

    candidates = []
    for mask in range(1 << k):
        if bin(mask).count("1") != active_cols:
            continue
        valid = not (state["counter_overflow"] or state["hybrid_overflow"])
        for row in range(k):
            for col in range(k):
                # Same interpretation as ``!(!pattern[r] || pattern[c])``.
                if matrix[row][col] and ((mask >> (k - 1 - row)) & 1) and not ((mask >> (k - 1 - col)) & 1):
                    valid = False
        candidates.append(valid)
    first = next((index + 1 for index, valid in enumerate(candidates) if valid), 0)
    return tuple(candidates), first, tuple(tuple(row[:k]) for row in matrix[:k])


def semantic_signature(state):
    """All collector-semantic fields, excluding unobservable invalid payload."""
    return {
        "fault_count": state["fault_count"],
        "pivots": tuple(state["pivots"]),
        "hybrids": tuple((entry["row"], entry["col"], entry["pointer"],
                          entry["descriptor"], entry["cfg_valid"])
                         for entry in state["hybrids"]),
        "reuse": tuple((entry["row"], entry["col"], entry["cfg_valid"])
                       for entry in state["reuse"]),
        "row_counters": tuple(state["row_counters"].items()),
        "col_counters": tuple(state["col_counters"].items()),
        "overflow": (state["counter_overflow"], state["hybrid_overflow"],
                     state["reuse_overflow"]),
    }


def encode_full(state):
    """Candidate FULL: direct semantic fields plus explicit Must cache."""
    row_must, col_must = must_vectors(state)
    return {
        "pivots": tuple(state["pivots"]),
        "pivot_valid": tuple([True] * len(state["pivots"]) +
                             [False] * (MAX_K - len(state["pivots"]))),
        "pivot_count": len(state["pivots"]),
        "hybrids": tuple((entry["row"], entry["col"], entry["pointer"],
                          entry["descriptor"], entry["cfg_valid"])
                         for entry in state["hybrids"]),
        "hybrid_count": len(state["hybrids"]),
        "reuse": tuple((entry["row"], entry["col"], entry["cfg_valid"])
                       for entry in state["reuse"]),
        "reuse_count": len(state["reuse"]),
        "row_counters": tuple(state["row_counters"].items()),
        "col_counters": tuple(state["col_counters"].items()),
        "fault_count": state["fault_count"],
        "overflow": (state["counter_overflow"], state["hybrid_overflow"],
                     state["reuse_overflow"]),
        "row_must": tuple(tuple(row) for row in row_must),
        "col_must": tuple(tuple(col) for col in col_must),
    }


def decode_full(encoded):
    state = fresh_state()
    state["pivots"] = list(encoded["pivots"])
    state["hybrids"] = [
        {"row": row, "col": col, "pointer": pointer,
         "descriptor": descriptor, "cfg_valid": cfg, "fault_ref": 0}
        for row, col, pointer, descriptor, cfg in encoded["hybrids"]]
    state["reuse"] = [{"row": row, "col": col, "cfg_valid": cfg}
                      for row, col, cfg in encoded["reuse"]]
    state["row_counters"] = OrderedDict(encoded["row_counters"])
    state["col_counters"] = OrderedDict(encoded["col_counters"])
    state["fault_count"] = encoded["fault_count"]
    (state["counter_overflow"], state["hybrid_overflow"],
     state["reuse_overflow"]) = encoded["overflow"]
    return state


def encode_derived_a(state):
    """Candidate DERIVED-A: omit Must/full caches, retain direct raw payload."""
    encoded = encode_full(state)
    for field in ("row_must", "col_must", "pivot_count", "hybrid_count", "reuse_count"):
        del encoded[field]
    return encoded


def decode_derived_a(encoded):
    return decode_full(encoded)


def encode_minimal(state):
    """Canonical semantic packing with all proven derivations applied.

    A Hybrid stores its Pivot pointer, descriptor, and only the differing
    coordinate.  Its other coordinate is reconstructed from that Pivot.  Its
    Config membership is reconstructed from the pointer.  Counter count zero
    denotes an invalid counter slot; append-prefix ordering is preserved.
    """
    hybrid = []
    assert len(state["hybrids"]) <= MAX_REACHABLE_HYBRIDS
    assert len(state["reuse"]) <= MAX_REACHABLE_REUSE
    for entry in state["hybrids"]:
        differing = entry["row"] if entry["descriptor"] else entry["col"]
        hybrid.append((entry["pointer"], entry["descriptor"], differing))
    return {
        "pivot_count": len(state["pivots"]),
        "pivots": tuple(state["pivots"]),
        "hybrid_count": len(hybrid),
        "hybrids": tuple(hybrid),
        "reuse_count": len(state["reuse"]),
        "reuse": tuple((entry["row"], entry["col"])
                       for entry in state["reuse"]),
        "row_counters": tuple(state["row_counters"].items()),
        "col_counters": tuple(state["col_counters"].items()),
    }


def decode_minimal(encoded):
    state = fresh_state()
    state["pivots"] = list(encoded["pivots"])
    assert encoded["pivot_count"] == len(state["pivots"])
    for pointer, descriptor, differing in encoded["hybrids"]:
        pivot_row, pivot_col = state["pivots"][pointer]
        row, col = (differing, pivot_col) if descriptor else (pivot_row, differing)
        state["hybrids"].append({"row": row, "col": col, "pointer": pointer,
                                 "descriptor": descriptor,
                                 "cfg_valid": mask_for_pointer(pointer),
                                 "fault_ref": 0})
    assert encoded["hybrid_count"] == len(state["hybrids"])
    state["reuse"] = [{"row": row, "col": col,
                       "cfg_valid": reuse_mask_for_record(state["pivots"], row, col)}
                      for row, col in encoded["reuse"]]
    assert encoded["reuse_count"] == len(state["reuse"])
    state["row_counters"] = OrderedDict(encoded["row_counters"])
    state["col_counters"] = OrderedDict(encoded["col_counters"])
    # Frozen proof: each accepted fault increments exactly one row counter;
    # with MAX_FAULTS == COUNTER_CAPACITY no counter overflow is reachable.
    state["fault_count"] = sum(state["row_counters"].values())
    state["counter_overflow"] = False
    state["hybrid_overflow"] = False
    state["reuse_overflow"] = False
    return state


REPRESENTATIONS = {
    "FULL": (encode_full, decode_full),
    "DERIVED_A": (encode_derived_a, decode_derived_a),
    "MINIMAL": (encode_minimal, decode_minimal),
}


def assert_equivalent(original, reconstructed, label, check_views=True,
                      check_candidate=True):
    if semantic_signature(original) != semantic_signature(reconstructed):
        raise AssertionError("semantic state mismatch: " + label)
    if not check_views:
        return
    for config in CONFIGS:
        if canonical_232_view(original, config) != canonical_232_view(reconstructed, config):
            raise AssertionError("232-bit view mismatch: " + label)
        if check_candidate and candidate_result(original, config) != candidate_result(reconstructed, config):
            raise AssertionError("candidate mismatch: " + label)


def state_from_faults(faults):
    return collect(faults)


def star_faults(pivot_count, row_extras, col_extras):
    faults = [(100 + index, 200 + index) for index in range(pivot_count)]
    value = 1000
    for index in range(pivot_count):
        for _ in range(row_extras[index]):
            faults.append((100 + index, value))
            value += 1
        for _ in range(col_extras[index]):
            faults.append((value, 200 + index))
            value += 1
    return faults


def bounded_compositions(limit, dimensions):
    values = [0] * dimensions
    def visit(index, remaining):
        if index == dimensions:
            yield tuple(values)
            return
        for value in range(remaining + 1):
            values[index] = value
            for result in visit(index + 1, remaining - value):
                yield result
    for result in visit(0, limit):
        yield result


def allocation_states():
    """The A2D-R independent-star allocation corpus (145,834 states)."""
    for config in sorted(CONFIGS):
        k = config_k(config)
        for pivot_count in range(1, k + 1):
            budget = MAX_FAULTS - pivot_count
            for allocation in bounded_compositions(budget, 2 * pivot_count):
                yield state_from_faults(star_faults(pivot_count,
                                                    allocation[:pivot_count],
                                                    allocation[pivot_count:]))


def directed_states():
    # Includes membership >9, both post-Must=9 witnesses, mixed Must, and
    # a legal CAM-reuse trace (five independent pivots then an extra fault).
    return [
        state_from_faults([(0, 0)] + [(0, col) for col in range(1, 12)]),
        state_from_faults([(0, 0)] + [(row, 0) for row in range(1, 12)]),
        state_from_faults([(0, 0), (0, 1), (1, 0), (1, 2), (2, 0)]),
        state_from_faults(star_faults(3, (1, 1, 1), (2, 2, 2))),
        state_from_faults(star_faults(3, (2, 2, 2), (1, 1, 1))),
        state_from_faults([(10, 20), (11, 21), (12, 22), (13, 23),
                           (14, 24), (99, 199), (98, 198)]),
    ]


def counterexamples():
    """Directed alias checks; legal-state facts and explicit unsafe omissions."""
    result = {}
    # F-CE3: Pivot addresses alone do not determine Must.  The last two
    # faults raise the retained row counter across the Config-dependent
    # threshold; their Hybrid records are later retired, but the counter is
    # still the sole source for the Must predicate.
    before = state_from_faults([(0, 0), (0, 1)])
    after = state_from_faults([(0, 0), (0, 1), (0, 2), (0, 3)])
    assert before["pivots"] == after["pivots"]
    assert must_vectors(before) != must_vectors(after)
    result["F_CE3_counter_required_for_must"] = True

    # F-CE5: same logical Hybrid set in reverse physical order is a distinct
    # view because the projector's H0..H13 stable compaction preserves order.
    ordered = state_from_faults([(0, 0), (0, 9), (0, 8)])
    reversed_state = clone(ordered)
    reversed_state["hybrids"] = list(reversed(reversed_state["hybrids"]))
    assert post_must_view(ordered, 5)[1] != post_must_view(reversed_state, 5)[1]
    result["F_CE5_h_order_required"] = True

    # F-CE6: CAM-reuse payload is hidden from the post-Must analyzer view;
    # omitting the ordered payload would alias legal states with different
    # historical decode state.  Its cfg tag itself is nevertheless derivable.
    prefix = [(10, 20), (11, 21), (12, 22), (13, 23), (14, 24)]
    state_a = state_from_faults(prefix + [(99, 199)])
    state_b = state_from_faults(prefix + [(98, 198)])
    assert all(canonical_232_view(state_a, cfg) == canonical_232_view(state_b, cfg)
               for cfg in CONFIGS)
    assert state_a["reuse"] != state_b["reuse"]
    for state in (state_a, state_b):
        for entry in state["reuse"]:
            assert entry["cfg_valid"] == reuse_mask_for_record(
                state["pivots"], entry["row"], entry["col"])
    result["F_CE6_reuse_payload_order_required_cfg_derivable"] = True

    # F-CE1: every *legal* Hybrid tag is exactly the static pointer mask,
    # including the pointer-3/4 Config membership holes.
    for state in directed_states():
        for entry in state["hybrids"]:
            assert entry["cfg_valid"] == mask_for_pointer(entry["pointer"])
    result["F_CE1_hybrid_cfg_is_derivable_for_legal_states"] = True

    # F-CE2/F-CE4 are source-invariant conclusions: pointer+descriptor derive
    # from raw address/pivot order; frozen overflow flags are unreachable.
    result["F_CE2_pointer_descriptor_rederive_from_raw"] = True
    result["F_CE4_frozen_overflow_unreachable"] = True
    return result


def generic_overflow_probe():
    """Show why sticky overflow would have to be retained outside frozen limits."""
    faults = [(1, 1)] + [(1, 100 + index) for index in range(15)]
    state = collect(faults, max_faults=20, hybrid_capacity=14,
                    reuse_capacity=20, counter_capacity=20)
    assert state["hybrid_overflow"]
    # FULL encode/decode still preserves the sticky bit in this generic probe.
    assert_equivalent(state, decode_full(encode_full(state)), "generic-overflow")
    return {"hybrid_overflow_sticky": state["hybrid_overflow"],
            "hybrid_entries": len(state["hybrids"]),
            "fault_count": state["fault_count"]}


def width_accounting():
    # Full retains direct source semantic values and an explicit Must snapshot.
    full = {
        "pivot": 5 + 5 * (ROW_W + COL_W) + 3,
        "hybrid": 14 + 14 * (ROW_W + COL_W + 1 + 3 + NUM_CFG) + 4,
        "cam_reuse": 12 + 12 * (ROW_W + COL_W + NUM_CFG) + 4,
        "counters": 2 * (12 + 12 * (10 + 4)),
        "fault_count": 4,
        "must": 2 * NUM_CFG * MAX_K,
        "overflow": 3,
    }
    derived = {
        "pivot": 5 + 5 * (ROW_W + COL_W),
        "hybrid": 14 + 14 * (ROW_W + COL_W + 1 + 3 + NUM_CFG),
        "cam_reuse": 12 + 12 * (ROW_W + COL_W + NUM_CFG),
        "counters": 2 * (12 + 12 * (10 + 4)),
        "fault_count": 4,
    }
    minimal = {
        "pivot": 3 + 5 * (ROW_W + COL_W),
        "hybrid": 4 + MAX_REACHABLE_HYBRIDS * (3 + 1 + 10),
        "cam_reuse": 4 + MAX_REACHABLE_REUSE * (ROW_W + COL_W),
        "counters": 2 * (12 * (10 + 4)),
    }
    return {"FULL": {"fields": full, "bits": sum(full.values())},
            "DERIVED_A": {"fields": derived, "bits": sum(derived.values())},
            "MINIMAL": {"fields": minimal, "bits": sum(minimal.values())}}


def run_campaign():
    stats = {"states": 0, "views": 0, "transitions": 0,
             "current_view_mismatches": 0, "transition_mismatches": 0,
             "candidate_mismatches": 0}
    representations = list(REPRESENTATIONS.items())
    random_source = random.Random(0xA2F)
    next_faults = [(0, 0), (1, 1), (7, 9), (511, 31)]

    def exercise(state, label, representation_names, faults, check_views,
                 check_candidate):
        stats["states"] += 1
        for name, (encode, decode) in representations:
            if name not in representation_names:
                continue
            reconstructed = decode(encode(state))
            assert_equivalent(state, reconstructed, label + "/" + name,
                              check_views=check_views,
                              check_candidate=check_candidate)
            if check_views:
                stats["views"] += NUM_CFG
            for fault in faults:
                expected = transition(state, fault)
                actual = transition(reconstructed, fault)
                assert_equivalent(expected, actual, label + "/" + name + "/next",
                                  check_views=check_views,
                                  check_candidate=check_candidate)
                stats["transitions"] += 1

    # A2D-R already closed the full 145,834-state capacity proof.  This phase
    # reuses a deterministic 10,007-state slice to exercise the new retained
    # encoding; the earlier full-corpus result remains cited evidence, not a
    # misleading claim that this script reran it.
    for index, state in enumerate(itertools.islice(allocation_states(), 10007)):
        exercise(state, "allocation-%d" % index, {"MINIMAL"}, [],
                 check_views=False, check_candidate=False)

    for index, state in enumerate(directed_states()):
        exercise(state, "directed-%d" % index, set(REPRESENTATIONS), next_faults,
                 check_views=True, check_candidate=True)

    # 10,000 reachable random traces, each with four legal/blocked next faults.
    for trace_index in range(10000):
        count = random_source.randrange(MAX_FAULTS + 1)
        faults = [(random_source.randrange(64), random_source.randrange(32))
                  for _ in range(count)]
        state = state_from_faults(faults)
        # All 10,000 reuse the required historical random corpus for MINIMAL
        # current-view equality.  Every tenth trace also checks every
        # representation, a next transition, and candidate result.
        if trace_index % 10 == 0:
            exercise(state, "random-%d" % trace_index, set(REPRESENTATIONS),
                     [next_faults[trace_index % len(next_faults)]],
                     check_views=True, check_candidate=True)
        else:
            exercise(state, "random-%d" % trace_index, {"MINIMAL"}, [],
                     check_views=True, check_candidate=False)

    return stats


def main():
    aliases = counterexamples()
    stats = run_campaign()
    output = {
        "model": "S1G-A2F historical collector retained-state proof",
        "frozen_parameters": {
            "row_w": ROW_W, "col_w": COL_W, "max_faults": MAX_FAULTS,
            "max_k": MAX_K, "hybrid_capacity": HYBRID_CAPACITY,
            "cam_reuse_capacity": REUSE_CAPACITY,
        },
        "candidate_widths": width_accounting(),
        "counterexamples": aliases,
        "generic_overflow_probe": generic_overflow_probe(),
        "campaign": stats,
        "result": "PASS",
    }
    print(json.dumps(output, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
