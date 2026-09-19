#!/usr/bin/env python3
"""Focused compatibility checks for the R3 formal sweep policy descriptor."""

from __future__ import annotations

import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "r3_formal_group_repair_rate", ROOT / "scripts/group/r3_formal_group_repair_rate.py")
assert SPEC and SPEC.loader
R3 = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(R3)


def row_for(policy: dict[str, object], implementation: str) -> dict[str, str]:
    return {
        "corpus_id": "corpus", "corpus_hash": "hash", "canonical_policy_id": str(policy["id"]),
        "implementation_policy_id": implementation, "policy_id": implementation,
        "solution_policy": implementation, "solution_class": str(policy["solution_class"]),
        "candidate_contract": str(policy["candidate_contract"]),
        "priority_class": str(policy["priority_class"]), "search_scope": str(policy["search_scope"]),
        "backtracking": str(policy["backtracking"]).lower(),
        "paper_canonical": str(policy["paper_canonical"]).lower(), "legacy_alias_of": "-",
        "layout": str(policy["layout"]), "topology": str(policy["topology"]),
        "share_row": str(policy["share_row"]), "share_col": str(policy["share_col"]),
        "N": "2", "RS": "2", "CS": "2", "F_GROUP": "8", "seed": "1",
    }


def expect_failure(rows: list[dict[str, str]], policy: dict[str, object]) -> None:
    try:
        R3.validate_policy_sidecar(rows, policy, {"RS": 2, "CS": 2, "F_GROUP": 8, "seed": 1}, "corpus")
    except RuntimeError:
        return
    raise AssertionError("semantic mismatch was accepted")


def main() -> int:
    if R3.CANONICAL_F_GROUP_LIST != (8, 12, 16, 20, 24, 28, 32):
        raise AssertionError("historical F_GROUP list changed")
    if R3.parse_f_group_list("4,8,12,16,20,24,28,32,36,40")[-1] != 40:
        raise AssertionError("extended F_GROUP override was rejected")
    for invalid in ("", "8,,12", "8,8", "0,8", "-1,8", "8,x"):
        try:
            R3.parse_f_group_list(invalid)
        except ValueError:
            continue
        raise AssertionError(f"invalid F_GROUP list accepted: {invalid!r}")

    policy = next(item for item in R3.canonical_policies(2)
                  if item["id"] == "directional_m1_local_first")
    # Old sidecars retained the invocation alias; new producer output uses the
    # normalized ID.  Both are the one audited DIRECTIONAL_V2 LOCAL_FIRST policy.
    R3.validate_policy_sidecar([row_for(policy, "directional_v2_early")], policy,
                               {"RS": 2, "CS": 2, "F_GROUP": 8, "seed": 1}, "corpus")
    R3.validate_policy_sidecar([row_for(policy, "normalized_local_first")], policy,
                               {"RS": 2, "CS": 2, "F_GROUP": 8, "seed": 1}, "corpus")
    expect_failure([row_for(policy, "normalized_streaming_early")], policy)
    mismatched = row_for(policy, "normalized_local_first")
    mismatched["topology"] = "edge"
    expect_failure([mismatched], policy)
    print("r3 formal sweep policy contract PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
