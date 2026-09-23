#!/usr/bin/env python3
"""DATE 2026 R3 formal group-level repair-rate sweep.

This is deliberately separate from the R2 preflight runner.  It makes one
fixed-total physical corpus per (RS, CS, F_GROUP) point, replays that corpus
through the frozen canonical policies, and refuses to write into an existing
formal-result root.  It does not invoke device-level simulation, RTL, or
synthesis.
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import hashlib
import json
import math
import shutil
import statistics
import subprocess
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any, Iterable


ROOT = Path(__file__).resolve().parents[2]
SCHEMA = "dss_r3_formal_group_repair_rate_v2"
MASTER_SEED = 20260922
FORMAL_SAMPLES = 100_000
# R3 Canonical Policy Matrix V2: the same fixed-total loads are swept for all
# supported group sizes.  Existing complete sidecars are reused only after
# validation.
# This is the frozen historical R3 point set.  Runtime --f-group-list values
# deliberately do not modify it: they define a new, explicitly selected run.
CANONICAL_F_GROUP_LIST = (8, 12, 16, 20, 24, 28, 32)
POINTS = {2: CANONICAL_F_GROUP_LIST,
          3: CANONICAL_F_GROUP_LIST,
          4: CANONICAL_F_GROUP_LIST}

# Policy display/invocation names were introduced before SimulationConfig's
# normalized implementation IDs.  Keep the aliases frozen here rather than
# comparing a descriptor's CLI spelling with an emitted display spelling.
# Each alias below resolves to the exact ID emitted by
# SimulationConfig::toString(SolutionTakePolicy).  It is intentionally not a
# permissive string normalizer: an unknown name is never an equivalent policy.
LEGACY_POLICY_NAME_TO_CANONICAL_POLICY_ID = {
    "pairwise_row_m1_local_first": "local_first",
    "directional_m1_local_first": "normalized_local_first",
    "directional_m1_early": "normalized_streaming_early",
    "directional_m1_global": "historical_directional_v2_global",
    "pairwise_row_m1_early": "early",
    "pairwise_row_m1_global": "group_global",
    "directional_v2_early": "normalized_local_first",
    "directional_m1_v2_early": "normalized_local_first",
    "group": "group_compressed_legacy",
    "group_compressed": "group_compressed_legacy",
    "group_greedy_rtl_canonical": "normalized_streaming_early",
    "group_no_scratch_v2": "normalized_early_deferred",
    "directional_v2_group_global": "historical_directional_v2_global",
    "directional_m1_v2_group_global": "historical_directional_v2_global",
    "historical_directional_v2_global": "historical_directional_v2_global",
    "directional_v2_group_global_canonical": "normalized_global",
    "directional_m1_global_canonical": "normalized_global",
    "two_pairwise_m1_local_first": "one_by_four_two_pairwise_early_v1",
    "two_pairwise_m1_early": "one_by_four_two_pairwise_release_aware_early_v1",
    "two_pairwise_m1_pair_global": "one_by_four_two_pairwise_pair_global_v1",
    "single_hop_m1_local_first": "one_by_four_single_hop_early_v1",
    "single_hop_m1_early": "one_by_four_single_hop_release_aware_early_v1",
    "single_hop_m1_global": "one_by_four_single_hop_global_v1",
    "one_by_four_two_pairwise_early_v1": "one_by_four_two_pairwise_early_v1",
    "one_by_four_two_pairwise_release_aware_early_v1": "one_by_four_two_pairwise_release_aware_early_v1",
    "one_by_four_two_pairwise_pair_global_v1": "one_by_four_two_pairwise_pair_global_v1",
    "one_by_four_single_hop_early_v1": "one_by_four_single_hop_early_v1",
    "one_by_four_single_hop_release_aware_early_v1": "one_by_four_single_hop_release_aware_early_v1",
    "one_by_four_single_hop_global_v1": "one_by_four_single_hop_global_v1",
}
CANONICAL_IMPLEMENTATION_POLICY_IDS = frozenset({
    "legacy", "local_first", "early", "group_compressed_legacy",
    "normalized_early_deferred", "normalized_streaming_early", "group_global",
    "historical_directional_v2_global", "normalized_global",
    "normalized_local_first", "hyp02_static_early", "hyp02_static_global",
    "g2x2_r_static_early", "g2x2_r_static_global",
    "l1x4_r_static_early", "l1x4_r_static_global",
    "one_by_four_two_pairwise_early_v1",
    "one_by_four_two_pairwise_release_aware_early_v1",
    "one_by_four_two_pairwise_pair_global_v1", "one_by_four_single_hop_early_v1",
    "one_by_four_single_hop_release_aware_early_v1",
    "one_by_four_single_hop_global_v1",
})
POLICY_DISPLAY_NAMES = {
    "local_no_sharing": "LOCAL", "directional_m1_local_first": "Directional V2 LOCAL-FIRST",
    "directional_m1_early": "Directional V2 EARLY", "directional_m1_global": "Directional V2 GLOBAL",
    "pairwise_row_m1_local_first": "Pairwise-row LOCAL-FIRST",
    "pairwise_row_m1_early": "Pairwise-row EARLY", "pairwise_row_m1_global": "Pairwise-row GLOBAL",
    "single_hop_m1_local_first": "Single-hop LOCAL-FIRST", "single_hop_m1_early": "Single-hop EARLY",
    "single_hop_m1_global": "Single-hop GLOBAL", "two_pairwise_m1_local_first": "Two-pairwise LOCAL-FIRST",
    "two_pairwise_m1_early": "Two-pairwise EARLY", "two_pairwise_m1_pair_global": "Two-pairwise PAIR-GLOBAL",
}


def canonical_implementation_policy_id(policy_name: str) -> str:
    """Resolve an audited legacy policy spelling to its stable emitted ID."""
    if policy_name in CANONICAL_IMPLEMENTATION_POLICY_IDS:
        return policy_name
    try:
        return LEGACY_POLICY_NAME_TO_CANONICAL_POLICY_ID[policy_name]
    except KeyError as error:
        raise ValueError(f"unknown policy implementation ID: {policy_name}") from error


def canonical_policies(n: int) -> list[dict[str, Any]]:
    """Return Matrix V2 policies explicitly supported by group size ``n``.

    The frozen directional V2 ConfigID contract exists only for N=2 and N=3.
    N=4 therefore uses explicit per-N membership rather than silently falling
    back to the historical generic GROUP_GLOBAL implementation.
    """
    policies = [
        {"id": "local_no_sharing", "layout": "2x2", "topology": "none",
         "share_row": 0, "share_col": 0, "solution": "legacy",
         "solution_class": "LOCAL", "candidate_contract": "LOCAL", "priority_class": "LOCAL_ONLY",
         "search_scope": "LOCAL", "backtracking": False,
         "paper_canonical": True, "n_support": [2, 3, 4]},
        # Directional m=2 has only the historical generic candidate contract.
        # It remains separately addressable through its legacy solution names,
        # but is deliberately outside the normalized matrix until a frozen
        # role/config contract is specified for every supported N.
        # Edge is a 2x2 physical sharing contract.  It must not be relabelled
        # as a 1x4 pair policy, whose Pair-GLOBAL solver is distinct.
        {"id": "pairwise_row_m1_local_first", "layout": "2x2", "topology": "edge",
         "share_row": 1, "share_col": 0, "solution": "local_first",
         "solution_class": "LOCAL_FIRST", "candidate_contract": "GENERIC_RECAM", "priority_class": "LOCAL_FIRST",
         "search_scope": "SEQUENTIAL_FIRST_LEGAL", "backtracking": False,
         "paper_canonical": True, "n_support": [2, 3, 4]},
        {"id": "pairwise_row_m1_early", "layout": "2x2", "topology": "edge",
         "share_row": 1, "share_col": 0, "solution": "early",
         "solution_class": "EARLY", "candidate_contract": "GENERIC_RECAM", "priority_class": "RELEASE_AWARE",
         "search_scope": "SEQUENTIAL_RANKED_COMMIT", "backtracking": False,
         "paper_canonical": True, "n_support": [2, 3, 4]},
        {"id": "pairwise_row_m1_global", "layout": "2x2", "topology": "edge",
         "share_row": 1, "share_col": 0, "solution": "group_global",
         "solution_class": "GLOBAL", "candidate_contract": "GENERIC_RECAM", "priority_class": "JOINT_ORACLE",
         "search_scope": "JOINT_GENERIC_GROUP_SEARCH", "backtracking": True,
         "paper_canonical": True, "n_support": [2, 3, 4]},
        {"id": "single_hop_m1_local_first", "layout": "1x4", "topology": "neighbor",
         "share_row": 1, "share_col": 0, "solution": "one_by_four_single_hop_early_v1",
         "solution_class": "LOCAL_FIRST", "candidate_contract": "R1B_1X4", "priority_class": "LOCAL_FIRST",
         "search_scope": "SEQUENTIAL_FIRST_LEGAL", "backtracking": False,
         "paper_canonical": True, "n_support": [2, 3, 4]},
        {"id": "single_hop_m1_early", "layout": "1x4", "topology": "neighbor",
         "share_row": 1, "share_col": 0, "solution": "one_by_four_single_hop_release_aware_early_v1",
         "solution_class": "EARLY", "candidate_contract": "R1B_1X4", "priority_class": "RELEASE_AWARE",
         "search_scope": "SEQUENTIAL_RANKED_COMMIT", "backtracking": False,
         "paper_canonical": True, "n_support": [2, 3, 4]},
        {"id": "single_hop_m1_global", "layout": "1x4", "topology": "neighbor",
         "share_row": 1, "share_col": 0, "solution": "one_by_four_single_hop_global_v1",
         "solution_class": "GLOBAL", "candidate_contract": "R1B_1X4", "priority_class": "JOINT_ORACLE",
         "search_scope": "JOINT_COMPLETE_TUPLE_SEARCH", "backtracking": True,
         "paper_canonical": True, "n_support": [2, 3, 4]},
        {"id": "two_pairwise_m1_local_first", "layout": "1x4", "topology": "pair",
         "share_row": 1, "share_col": 0, "solution": "one_by_four_two_pairwise_early_v1",
         "solution_class": "LOCAL_FIRST", "candidate_contract": "R1B_1X4", "priority_class": "LOCAL_FIRST",
         "search_scope": "SEQUENTIAL_FIRST_LEGAL", "backtracking": False,
         "paper_canonical": True, "n_support": [2, 3, 4]},
        {"id": "two_pairwise_m1_early", "layout": "1x4", "topology": "pair",
         "share_row": 1, "share_col": 0, "solution": "one_by_four_two_pairwise_release_aware_early_v1",
         "solution_class": "EARLY", "candidate_contract": "R1B_1X4", "priority_class": "RELEASE_AWARE",
         "search_scope": "SEQUENTIAL_RANKED_COMMIT", "backtracking": False,
         "paper_canonical": True, "n_support": [2, 3, 4]},
        # Preserve the established Pair-GLOBAL implementation rather than
        # silently substituting the generic four-SA GROUP_GLOBAL solver.
        {"id": "two_pairwise_m1_pair_global", "layout": "1x4", "topology": "pair",
         "share_row": 1, "share_col": 0, "solution": "one_by_four_two_pairwise_pair_global_v1",
         "solution_class": "PAIR_GLOBAL", "candidate_contract": "R1B_1X4", "priority_class": "PAIR_JOINT_ORACLE",
         "search_scope": "PAIR_COMPLETE_TUPLE_SEARCH", "backtracking": True,
         "paper_canonical": True, "n_support": [2, 3, 4]},
    ]
    if n in (2, 3):
        policies[1:1] = [
            {"id": "directional_m1_local_first", "layout": "2x2", "topology": "directional",
             "share_row": 1, "share_col": 1, "solution": "directional_v2_early",
             "solution_class": "LOCAL_FIRST", "candidate_contract": "DIRECTIONAL_V2", "priority_class": "LOCAL_FIRST",
             "search_scope": "SEQUENTIAL_FIRST_LEGAL", "backtracking": False,
             "paper_canonical": True, "n_support": [2, 3]},
            {"id": "directional_m1_early", "layout": "2x2", "topology": "directional",
             "share_row": 1, "share_col": 1, "solution": "group_greedy_rtl_canonical",
             "solution_class": "EARLY", "candidate_contract": "DIRECTIONAL_V2", "priority_class": "RELEASE_AWARE",
             "search_scope": "SEQUENTIAL_FIRST_LEGAL", "backtracking": False,
             "paper_canonical": True, "n_support": [2, 3]},
            {"id": "directional_m1_global", "layout": "2x2", "topology": "directional",
             "share_row": 1, "share_col": 1, "solution": "directional_v2_group_global",
             "solution_class": "GLOBAL", "candidate_contract": "DIRECTIONAL_V2", "priority_class": "JOINT_ORACLE",
             "search_scope": "JOINT_COMPLETE_TUPLE_SEARCH", "backtracking": True,
             "paper_canonical": True, "n_support": [2, 3]},
        ]
    if any(int(item["share_row"]) > n for item in policies):
        raise RuntimeError("R3 policy exceeds the selected RS")
    for policy in policies:
        policy["display_name"] = POLICY_DISPLAY_NAMES[str(policy["id"])]
    return policies


def sixcase_static_policies(n: int) -> list[dict[str, Any]]:
    """Return the N=2 final-archive static-policy preflight matrix.

    The local entry materializes the common corpus; the remaining six entries
    are the six DATE cases and retain their distinct static contracts.
    """
    if n != 2:
        raise ValueError("sixcase_static preset is limited to N=2 final-archive evidence")
    policies = [
        {"id": "local_no_sharing", "layout": "2x2", "topology": "none",
         "share_row": 0, "share_col": 0, "solution": "legacy",
         "solution_class": "LOCAL", "candidate_contract": "LOCAL", "priority_class": "LOCAL_ONLY",
         "search_scope": "LOCAL", "backtracking": False,
         "paper_canonical": True, "n_support": [2]},
        {"id": "g2x2_rc_early", "layout": "2x2", "topology": "directional",
         "share_row": 1, "share_col": 1, "solution": "hyp02_static_early",
         "solution_class": "EARLY", "candidate_contract": "HYP02_STATIC_81_PATH", "priority_class": "R_L_RB_B",
         "search_scope": "SEQUENTIAL_PREFIX_PATH_COMMIT", "backtracking": False,
         "paper_canonical": True, "n_support": [2]},
        {"id": "g2x2_rc_group", "layout": "2x2", "topology": "directional",
         "share_row": 1, "share_col": 1, "solution": "hyp02_static_global",
         "solution_class": "GROUP", "candidate_contract": "HYP02_STATIC_81_PATH", "priority_class": "P0_TO_P80_ASCENDING",
         "search_scope": "FIRST_LEGAL_STATIC_PATH", "backtracking": True,
         "paper_canonical": True, "n_support": [2]},
        {"id": "g2x2_r_early", "layout": "2x2", "topology": "directional",
         "share_row": 1, "share_col": 0, "solution": "g2x2_r_static_early",
         "solution_class": "EARLY", "candidate_contract": "G2X2_R_STATIC_81_PATH", "priority_class": "R_L_RB_B",
         "search_scope": "SEQUENTIAL_PREFIX_PATH_COMMIT", "backtracking": False,
         "paper_canonical": True, "n_support": [2]},
        {"id": "g2x2_r_group", "layout": "2x2", "topology": "directional",
         "share_row": 1, "share_col": 0, "solution": "g2x2_r_static_global",
         "solution_class": "GROUP", "candidate_contract": "G2X2_R_STATIC_81_PATH", "priority_class": "P0_TO_P80_ASCENDING",
         "search_scope": "FIRST_LEGAL_STATIC_PATH", "backtracking": True,
         "paper_canonical": True, "n_support": [2]},
        {"id": "l1x4_r_early", "layout": "1x4", "topology": "neighbor",
         "share_row": 1, "share_col": 0, "solution": "l1x4_r_static_early",
         "solution_class": "EARLY", "candidate_contract": "L1X4_R_STATIC_27_PATH", "priority_class": "R_L_RB_B",
         "search_scope": "SEQUENTIAL_PREFIX_PATH_COMMIT", "backtracking": False,
         "paper_canonical": True, "n_support": [2]},
        {"id": "l1x4_r_group", "layout": "1x4", "topology": "neighbor",
         "share_row": 1, "share_col": 0, "solution": "l1x4_r_static_global",
         "solution_class": "GROUP", "candidate_contract": "L1X4_R_STATIC_27_PATH", "priority_class": "P0_TO_P26_ASCENDING",
         "search_scope": "FIRST_LEGAL_STATIC_PATH", "backtracking": True,
         "paper_canonical": True, "n_support": [2]},
    ]
    for policy in policies:
        policy["display_name"] = str(policy["id"])
    return policies


def policies_for_preset(n: int, preset: str) -> list[dict[str, Any]]:
    if preset == "canonical":
        return canonical_policies(n)
    if preset == "sixcase_static":
        return sixcase_static_policies(n)
    raise ValueError(f"unknown policy preset: {preset}")


def point_seed(n: int, f_group: int, master_seed: int) -> int:
    """Frozen point seed mapping, deliberately independent of policy."""
    return master_seed + n * 1_000 + f_group


def selected_points(rs: int | None, cs: int | None,
                    f_group_list: str | None) -> dict[int, tuple[int, ...]]:
    """Return a validated subset without changing the frozen point defaults."""
    if (rs is None) != (cs is None) or (rs is not None and rs != cs):
        raise ValueError("R3 group sweep requires --rs and --cs to be equal")
    selected_n = tuple(POINTS) if rs is None else (rs,)
    result: dict[int, tuple[int, ...]] = {}
    requested = None if f_group_list is None else parse_f_group_list(f_group_list)
    for n in selected_n:
        if n not in POINTS:
            raise ValueError("R3 supports only RS=CS=2, RS=CS=3, or RS=CS=4")
        values = POINTS[n] if requested is None else requested
        result[n] = tuple(values)
    return result


def parse_f_group_list(value: str) -> tuple[int, ...]:
    """Parse a positive, distinct ordered runtime F_GROUP override."""
    if not value or any(not item.strip() for item in value.split(",")):
        raise ValueError("--f-group-list must be a comma-separated list of integers")
    try:
        values = tuple(int(item.strip()) for item in value.split(","))
    except ValueError as error:
        raise ValueError("--f-group-list must be a comma-separated list of integers") from error
    if any(item <= 0 for item in values):
        raise ValueError("--f-group-list values must be positive integers")
    if len(set(values)) != len(values):
        raise ValueError("--f-group-list values must be distinct")
    return values


def complete_result_rows(path: Path, samples: int) -> list[dict[str, str]] | None:
    """A sidecar is resumable only if it has exactly the requested group IDs."""
    if not path.is_file():
        return None
    rows = read_csv(path)
    if len(rows) != samples:
        return None
    if any(int(row["group_id"]) != index for index, row in enumerate(rows)):
        return None
    return rows


def validate_policy_sidecar(rows: list[dict[str, str]], policy: dict[str, Any],
                            point: dict[str, int], corpus_id: str) -> None:
    """Validate identity and every semantic/resource field before reuse.

    Legacy implementation spellings are accepted only through the audited
    mapping above; all other policy, topology, resource-point, and algorithm
    fields remain exact comparisons.
    """
    policy_id = str(policy["id"])
    expected_implementation = canonical_implementation_policy_id(str(policy["solution"]))
    required = ("canonical_policy_id", "implementation_policy_id", "policy_id",
                "solution_policy", "solution_class", "candidate_contract",
                "priority_class", "search_scope", "backtracking",
                "paper_canonical", "legacy_alias_of", "layout", "topology", "corpus_id",
                "share_row", "share_col", "N", "RS", "CS", "F_GROUP", "seed",
                "corpus_hash")
    if not rows or any(field not in rows[0] for field in required):
        raise RuntimeError(f"{policy_id}: sidecar lacks policy semantic metadata")
    expected = {
        "canonical_policy_id": policy_id,
        "solution_class": str(policy["solution_class"]),
        "candidate_contract": str(policy["candidate_contract"]),
        "priority_class": str(policy["priority_class"]),
        "search_scope": str(policy["search_scope"]),
        "backtracking": str(policy["backtracking"]).lower(),
        "paper_canonical": str(policy["paper_canonical"]).lower(),
        "legacy_alias_of": "-",
        "layout": str(policy["layout"]), "topology": str(policy["topology"]),
        "share_row": str(policy["share_row"]), "share_col": str(policy["share_col"]),
        "N": str(point["RS"]), "RS": str(point["RS"]), "CS": str(point["CS"]),
        "F_GROUP": str(point["F_GROUP"]), "seed": str(point["seed"]),
    }
    for row in rows:
        if row["corpus_id"] != corpus_id:
            raise RuntimeError(f"{policy_id}: replayed a different corpus")
        if any(row[field] != value for field, value in expected.items()):
            raise RuntimeError(f"{policy_id}: sidecar semantic metadata does not match descriptor")
        implementation_ids = (row["implementation_policy_id"], row["policy_id"],
                              row["solution_policy"])
        try:
            matching_implementation = all(
                canonical_implementation_policy_id(value) == expected_implementation
                for value in implementation_ids)
        except ValueError as error:
            raise RuntimeError(f"{policy_id}: sidecar implementation policy is unknown") from error
        if not matching_implementation:
            raise RuntimeError(f"{policy_id}: sidecar solution policy does not match descriptor")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def command_text(command: list[str]) -> str:
    return " ".join(command)


def invoke(command: list[str], log: Path) -> None:
    with log.open("w", encoding="utf-8") as stream:
        started = dt.datetime.now(dt.timezone.utc).isoformat()
        stream.write(f"started_utc={started}\ncommand={command_text(command)}\n")
        stream.flush()
        completed = subprocess.run(command, cwd=ROOT, stdout=stream,
                                   stderr=subprocess.STDOUT, text=True)
        stream.write(f"finished_utc={dt.datetime.now(dt.timezone.utc).isoformat()}\n")
    if completed.returncode:
        raise RuntimeError(f"command failed ({completed.returncode}): {command_text(command)}")


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))


def write_csv(path: Path, rows: Iterable[dict[str, Any]], fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=fields, extrasaction="raise")
        writer.writeheader()
        writer.writerows(rows)


def value_int(value: str) -> int:
    if value in ("", "-", "NA"):
        return 0
    return int(value)


def percentile(values: list[float], fraction: float) -> float | None:
    if not values:
        return None
    ordered = sorted(values)
    return ordered[max(0, math.ceil(len(ordered) * fraction) - 1)]


def wilson(successes: int, groups: int) -> tuple[float, float]:
    if groups == 0:
        raise ValueError("Wilson interval requires positive sample count")
    z = 1.959963984540054
    p = successes / groups
    denominator = 1.0 + z * z / groups
    centre = (p + z * z / (2.0 * groups)) / denominator
    radius = z * math.sqrt((p * (1.0 - p) + z * z / (4.0 * groups)) / groups) / denominator
    return centre - radius, centre + radius


def materialize_replay(corpus_rows: list[dict[str, str]], destination: Path,
                       f_group: int) -> list[tuple[int, int, int, int]]:
    counts: list[tuple[int, int, int, int]] = []
    with destination.open("w", encoding="utf-8") as target:
        for expected_group, row in enumerate(corpus_rows):
            if int(row["group_id"]) != expected_group:
                raise RuntimeError("corpus group IDs are not contiguous")
            buckets = row["sa_local_faults"].split("|")
            if len(buckets) != 4:
                raise RuntimeError("paired corpus must contain four SA buckets")
            per_sa: list[int] = []
            for bucket in buckets:
                faults = [] if not bucket else bucket.split(";")
                per_sa.append(len(faults))
                for encoded in faults:
                    target.write(encoded.replace(":", " ") + "\n")
            count = tuple(per_sa)
            if sum(count) != f_group:
                raise RuntimeError("fixed F_GROUP invariant failed while materializing corpus")
            counts.append(count)  # type: ignore[arg-type]
    return counts


def quantile_membership(stddevs: list[float], bins: int = 5) -> tuple[list[int], list[dict[str, Any]]]:
    """Return up to five deterministic equal-count rank bins.

    Smoke runs may contain fewer than five groups, in which case empty bins
    have no meaningful bounds and are omitted.  Formal and 1k runs still use
    the full rank-quintile contract.
    """
    if not stddevs:
        raise ValueError("rank-bin membership requires at least one group")
    bins = min(bins, len(stddevs))
    order = sorted(range(len(stddevs)), key=lambda index: (stddevs[index], index))
    membership = [0] * len(stddevs)
    summaries: list[dict[str, Any]] = []
    for bin_index in range(bins):
        start = (bin_index * len(order)) // bins
        end = ((bin_index + 1) * len(order)) // bins
        indices = order[start:end]
        for index in indices:
            membership[index] = bin_index + 1
        values = [stddevs[index] for index in indices]
        summaries.append({"imbalance_bin": bin_index + 1, "method": "rank_quintile_v1",
                          "lower_stddev": min(values), "upper_stddev": max(values),
                          "groups": len(values)})
    return membership, summaries


def policy_command(binary: Path, n: int, f_group: int, samples: int, seed: int,
                   policy: dict[str, Any], output_dir: Path, replay_file: Path | None) -> list[str]:
    command = [str(binary), str(n), str(n)]
    if replay_file is not None:
        command += ["--simplified-fault-file", str(replay_file)]
    command += ["--fault-model", "multinomial_uniform", "--fault-count", str(f_group),
                "--runs", str(samples), "--seed", str(seed), "--spatial", "mixed",
                "--layout", str(policy["layout"]), "--topology", str(policy["topology"]),
                "--shared-rows", str(policy["share_row"]), "--shared-columns", str(policy["share_col"]),
                "--solution-take", str(policy["solution"]), "--paper-cam-reuse",
                "--canonical-policy-id", str(policy["id"]), "--paper-canonical",
                str(bool(policy["paper_canonical"])).lower(), "--legacy-alias-of", "-",
                "--hybrid-cam-entry-width-bits", "20", "--summary-only",
                "--output-dir", str(output_dir)]
    return command


def check_global_dominance(point: str, corpus_rows: list[dict[str, str]],
                           successes: dict[str, list[int]], output_root: Path) -> None:
    comparisons = (
        # This is the frozen directional V2 containment relation.  It is not
        # interchangeable with the retained generic m=1 GLOBAL policy.
        ("directional_m1_early", "directional_m1_global"),
        # These 2x2 pairs share the generic candidate universe and physical
        # ledger contract.  The simulator independently asserts that generic
        # GROUP_GLOBAL contains a feasible generic EARLY selection.
        ("pairwise_row_m1_early", "pairwise_row_m1_global"),
        ("two_pairwise_m1_early", "two_pairwise_m1_pair_global"),
        ("single_hop_m1_early", "single_hop_m1_global"),
    )
    violations: list[dict[str, Any]] = []
    for subset, superset in comparisons:
        if subset not in successes or superset not in successes:
            continue
        for group_id, (left, right) in enumerate(zip(successes[subset], successes[superset])):
            if left and not right:
                violations.append({"point": point, "group_id": group_id,
                                   "subset_policy": subset, "global_policy": superset,
                                   "fault_coordinates": corpus_rows[group_id]["sa_local_faults"]})
    if violations:
        violation_path = output_root / "logs" / f"GLOBAL_DOMINANCE_VIOLATION_{point}.json"
        violation_path.write_text(json.dumps(violations, indent=2) + "\n", encoding="utf-8")
        raise RuntimeError(f"{point}: GLOBAL dominance violation; saved {violation_path}")


def aggregate_policy(point: dict[str, Any], policy: dict[str, Any], rows: list[dict[str, str]],
                     fault_stddevs: list[float]) -> dict[str, Any]:
    repaired = [value_int(row["repair_success"]) for row in rows]
    if len(repaired) != len(fault_stddevs):
        raise RuntimeError("policy/corpus row count mismatch")
    successes = sum(repaired)
    success_stddev = [fault_stddevs[i] for i, flag in enumerate(repaired) if flag]
    failure_stddev = [fault_stddevs[i] for i, flag in enumerate(repaired) if not flag]
    is_global = policy["id"] in (
        "directional_m1_global", "pairwise_row_m1_global",
        "single_hop_m1_global")
    # Pair-global is exact but pair-local rather than a four-SA GLOBAL DFS;
    # it deliberately has no GlobalSearchMetrics sidecar.
    search_nodes = [value_int(row["global_search_nodes_visited"]) for row in rows] if is_global else []
    remaining_rows = [value_int(row["remaining_rows"]) for row in rows]
    remaining_cols = [value_int(row["remaining_columns"]) for row in rows]
    borrowed_rows = [value_int(row["borrowed_rows"]) for row in rows]
    borrowed_cols = [value_int(row["borrowed_columns"]) for row in rows]
    ci_low, ci_high = wilson(successes, len(rows))
    return {
        **point, "policy": policy["id"], "layout": policy["layout"],
        "topology": policy["topology"], "sharing_policy": rows[0]["sharing_policy"],
        "solution_policy": rows[0]["solution_policy"], "m": policy["share_row"],
        "solution_class": policy["solution_class"],
        "candidate_contract": policy["candidate_contract"],
        "search_scope": policy["search_scope"],
        "backtracking": str(policy["backtracking"]).lower(),
        "paper_canonical": str(policy["paper_canonical"]).lower(),
        "groups": len(rows), "successes": successes, "failures": len(rows) - successes,
        "repair_rate": successes / len(rows), "ci95_low": ci_low, "ci95_high": ci_high,
        "mean_fault_stddev_success": statistics.fmean(success_stddev) if success_stddev else "NA",
        "mean_fault_stddev_failure": statistics.fmean(failure_stddev) if failure_stddev else "NA",
        "mean_remaining_rows": statistics.fmean(remaining_rows),
        "mean_remaining_columns": statistics.fmean(remaining_cols),
        "mean_borrowed_rows": statistics.fmean(borrowed_rows),
        "mean_borrowed_columns": statistics.fmean(borrowed_cols),
        "mean_search_nodes": statistics.fmean(search_nodes) if search_nodes else "NA",
        "p95_search_nodes": percentile([float(value) for value in search_nodes], 0.95) if search_nodes else "NA",
        "p99_search_nodes": percentile([float(value) for value in search_nodes], 0.99) if search_nodes else "NA",
        "max_search_nodes": max(search_nodes) if search_nodes else "NA",
        "cam_accounting_status": rows[0]["cam_accounting_status"],
    }


def paired_aggregate(point: dict[str, Any], successes: dict[str, list[int]]) -> list[dict[str, Any]]:
    comparisons = (
        ("directional_m1_local_first_vs_early", "directional_m1_local_first", "directional_m1_early"),
        ("directional_m1_early_vs_global", "directional_m1_early", "directional_m1_global"),
        ("edge_m1_local_first_vs_early", "pairwise_row_m1_local_first", "pairwise_row_m1_early"),
        ("edge_m1_early_vs_global", "pairwise_row_m1_early", "pairwise_row_m1_global"),
        ("two_pairwise_local_first_vs_early", "two_pairwise_m1_local_first", "two_pairwise_m1_early"),
        ("two_pairwise_early_vs_pair_global", "two_pairwise_m1_early", "two_pairwise_m1_pair_global"),
        ("single_hop_local_first_vs_early", "single_hop_m1_local_first", "single_hop_m1_early"),
        ("single_hop_early_vs_global", "single_hop_m1_early", "single_hop_m1_global"),
        ("two_pairwise_pair_global_vs_single_hop_global", "two_pairwise_m1_pair_global", "single_hop_m1_global"),
    )
    output: list[dict[str, Any]] = []
    for name, left_name, right_name in comparisons:
        if left_name not in successes or right_name not in successes:
            continue
        left, right = successes[left_name], successes[right_name]
        counts = defaultdict(int)
        for lhs, rhs in zip(left, right):
            counts[(lhs, rhs)] += 1
        output.append({**point, "comparison": name, "left_policy": left_name,
                       "right_policy": right_name, "left_pass_right_pass": counts[(1, 1)],
                       "left_pass_right_fail": counts[(1, 0)], "left_fail_right_pass": counts[(0, 1)],
                       "left_fail_right_fail": counts[(0, 0)],
                       "left_fail_right_pass_count": counts[(0, 1)]})
    return output


def make_plots(output_root: Path, summary: list[dict[str, Any]], paired: list[dict[str, Any]],
               imbalance: list[dict[str, Any]]) -> None:
    """Produce plots only from persisted aggregate CSV content."""
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except Exception as error:  # Plot failure must be explicit, never silent.
        raise RuntimeError(f"matplotlib is required for R3 plots: {error}") from error
    plot_dir = output_root / "plots"
    plot_dir.mkdir(exist_ok=True)
    rs_values = sorted({int(row["RS"]) for row in summary})
    for metric, filename, ylabel in (("repair_rate", "repair_rate_vs_f_group.png", "Group repair rate"),
                                     ("gain_vs_local_pp", "gain_vs_local_pp_vs_f_group.png", "Gain vs LOCAL (pp)")):
        fig, axes = plt.subplots(1, len(rs_values), figsize=(6 * len(rs_values), 4.5), sharey=False)
        if len(rs_values) == 1:
            axes = [axes]
        for axis, n in zip(axes, rs_values):
            by_policy: dict[str, list[dict[str, Any]]] = defaultdict(list)
            for row in summary:
                if row["RS"] == n:
                    by_policy[str(row["policy"])].append(row)
            for policy, entries in sorted(by_policy.items()):
                entries.sort(key=lambda row: int(row["F_GROUP"]))
                axis.plot([row["F_GROUP"] for row in entries], [row[metric] for row in entries], marker="o", label=policy)
            axis.set_title(f"RS=CS={n}")
            axis.set_xlabel("F_GROUP")
            axis.set_ylabel(ylabel)
            axis.grid(alpha=0.25)
        handles, labels = axes[0].get_legend_handles_labels()
        fig.legend(handles, labels, loc="center left", bbox_to_anchor=(1.01, 0.5), fontsize=7)
        fig.tight_layout(rect=(0, 0, 0.78, 1))
        fig.savefig(plot_dir / filename, dpi=180)
        plt.close(fig)

    # The rank-quintile aggregate is per corpus point and always records N.
    selected = {"local_no_sharing", "directional_m1_global", "single_hop_m1_global"}
    fig, axes = plt.subplots(1, len(rs_values), figsize=(5.5 * len(rs_values), 4.2), sharey=True)
    if len(rs_values) == 1:
        axes = [axes]
    for axis, n in zip(axes, rs_values):
        for policy in selected:
            entries = [row for row in imbalance if row["RS"] == n and row["policy"] == policy]
            # F_GROUP series are separate; use the formal mid-point for each x value.
            for f_group in sorted({int(row["F_GROUP"]) for row in entries}):
                values = sorted((row for row in entries if int(row["F_GROUP"]) == f_group), key=lambda row: row["imbalance_bin"])
                axis.plot([row["imbalance_bin"] for row in values], [row["repair_rate"] for row in values], marker="o", label=f"{policy}, F={f_group}")
        axis.set_title(f"RS=CS={n}")
        axis.set_xlabel("Fault-stddev rank quintile")
        axis.set_ylabel("Group repair rate")
        axis.set_xticks(range(1, 6))
        axis.grid(alpha=0.25)
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="center left", bbox_to_anchor=(1.01, 0.5), fontsize=6)
    fig.tight_layout(rect=(0, 0, 0.76, 1))
    fig.savefig(plot_dir / "repair_rate_vs_fault_imbalance.png", dpi=180)
    plt.close(fig)

    for comparison, filename in (("directional_m1_early_vs_global", "directional_m1_early_vs_global_paired_outcomes.png"),
                                 ("two_pairwise_pair_global_vs_single_hop_global", "two_pairwise_vs_single_hop_paired_outcomes.png")):
        records = [row for row in paired if row["comparison"] == comparison]
        labels = [f"N{row['RS']} F{row['F_GROUP']}" for row in records]
        fail_pass = [row["left_fail_right_pass"] for row in records]
        pass_fail = [row["left_pass_right_fail"] for row in records]
        fig, axis = plt.subplots(figsize=(10, 4.2))
        positions = list(range(len(records)))
        axis.bar(positions, fail_pass, label="left fail / right pass")
        axis.bar(positions, pass_fail, bottom=fail_pass, label="left pass / right fail")
        axis.set_xticks(positions, labels, rotation=45, ha="right")
        axis.set_ylabel("Paired group count")
        axis.set_title(comparison)
        axis.legend()
        axis.grid(axis="y", alpha=0.25)
        fig.tight_layout()
        fig.savefig(plot_dir / filename, dpi=180)
        plt.close(fig)


def git_text(command: list[str]) -> str:
    completed = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, check=False)
    return completed.stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-root", type=Path,
                        default=ROOT / "results/date2026/repair_rate/r3_formal_group")
    parser.add_argument("--samples", type=int, default=FORMAL_SAMPLES)
    parser.add_argument("--master-seed", type=int, default=MASTER_SEED)
    parser.add_argument("--binary", type=Path, default=ROOT / "build/bin/DynamicSpareSharing")
    parser.add_argument("--rs", type=int)
    parser.add_argument("--cs", type=int)
    parser.add_argument("--f-group-list")
    parser.add_argument("--mode", choices=("preflight", "formal", "custom"),
                        default="custom")
    parser.add_argument("--policy-preset", choices=("canonical", "sixcase_static"),
                        default="canonical")
    parser.add_argument("--resume", action="store_true",
                        help="reuse only complete per-policy sidecars in an existing root")
    args = parser.parse_args()
    output_root = args.output_root.resolve()
    binary = args.binary.resolve()
    if args.samples <= 0:
        raise SystemExit("--samples must be positive")
    try:
        points = selected_points(args.rs, args.cs, args.f_group_list)
        policy_matrix = {n: policies_for_preset(n, args.policy_preset)
                         for n in points}
    except ValueError as error:
        raise SystemExit(str(error)) from error
    if output_root.exists() and not args.resume:
        raise SystemExit(f"R3 output root must be fresh: {output_root}")
    if args.resume and not (output_root / "manifest" / "r3_manifest.json").is_file():
        raise SystemExit("--resume requires an existing R3 manifest")
    if not binary.is_file():
        raise SystemExit(f"DynamicSpareSharing binary not found: {binary}")

    for name in ("manifest", "corpus", "raw", "aggregate", "plots", "logs"):
        (output_root / name).mkdir(parents=True, exist_ok=args.resume)
    point_count = sum(len(fault_points) for fault_points in points.values())
    expected_evaluations = sum(
        len(policy_matrix[n]) * len(fault_points) * args.samples
        for n, fault_points in points.items())
    started_utc = dt.datetime.now(dt.timezone.utc).isoformat()
    manifest = {
        "schema_version": SCHEMA,
        "experiment_id": "date2026_r3_formal_group_repair_rate",
        "simulation_level": "group",
        "formal": args.samples == FORMAL_SAMPLES,
        "groups_per_point": args.samples,
        "unique_physical_group_evaluations": point_count * args.samples,
        "policy_group_evaluations": expected_evaluations,
        "fault_distribution_mode": "multinomial_uniform",
        "fault_contract": "Multinomial(F_GROUP; 0.25,0.25,0.25,0.25), exact per group",
        "scope": "group",
        "RS": sorted(points),
        "CS": sorted(points),
        "f_group_points": {str(n): list(values) for n, values in points.items()},
        "share_m": "canonical_m1; directional_m2_legacy_only",
        "policy_preset": args.policy_preset,
        "mode": args.mode,
        "output_root": str(output_root),
        "runner_identity": str(ROOT / "scripts/run_date2026_repair_sweep.sh"),
        "backend_identity": str(ROOT / "scripts/group/r3_formal_group_repair_rate.py"),
        "backend": "DynamicSpareSharing",
        "seed_contract": {f"n{n}_f{f}": point_seed(n, f, args.master_seed)
                          for n, values in points.items() for f in values},
        "master_seed": args.master_seed,
        "points": points,
        "policy_matrix": {str(n): policy_matrix[n] for n in points},
        "unsupported_policy_contracts": {
            "N4_DIRECTIONAL_V2_GLOBAL": "UNSUPPORTED: no frozen N4 ConfigID/candidate contract; excluded by per-N canonical membership",
            "directional_m1_group_global": "GENERIC_GROUP_GLOBAL_LEGACY: retained implementation, paper_canonical=false",
            "M2_POLICY_NORMALIZATION": "BLOCKED: generic m=2 capacityOptions has no frozen role/config release-borrow contract for all N",
        },
        "topologies": ["none", "directional", "edge", "pair", "neighbor"],
        "global_search_contract": "policy-specific: DIRECTIONAL_V2 GLOBAL retains all V2 slot/PatternID candidates and performs reversible A->B->C->D joint search with incremental ledger legality pruning; generic GROUP_GLOBAL remains a separate legacy candidate contract",
        "build_command": "make dynamic_sharing_b",
        "executable": str(binary), "executable_sha256": sha256(binary),
        "git_revision": git_text(["git", "rev-parse", "HEAD"]),
        "git_dirty_state": git_text(["git", "status", "--short"]),
        "started_utc": started_utc,
        "timestamp_utc": started_utc,
        "corpus_replay_identity": {
            "paired_corpus": "corpus/<point>/paired_corpus_v1.csv",
            "corpus_index": "manifest/corpus_index.csv",
            "replay_key": "point, group_id, seed, corpus_id, corpus_sha256",
        },
        "cam_policy": {"2x2": "GENERIC_CPP_CAPACITY; NOT RTL-CALIBRATED",
                       "1x4": "NOT_PROVEN"},
    }
    if not args.resume:
        (output_root / "manifest" / "r3_manifest.json").write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    summary_rows: list[dict[str, Any]] = []
    paired_rows: list[dict[str, Any]] = []
    imbalance_rows: list[dict[str, Any]] = []
    corpus_index: list[dict[str, Any]] = []
    commands: list[str] = []
    try:
        for n, f_points in points.items():
            for f_group in f_points:
                point_name = f"n{n}_f{f_group}"
                point = {"RS": n, "CS": n, "F_GROUP": f_group,
                         "seed": point_seed(n, f_group, args.master_seed)}
                point_raw = output_root / "raw" / point_name
                point_raw.mkdir(exist_ok=args.resume)
                policies = policy_matrix[n]
                source_policy = policies[0]
                local_dir = point_raw / str(source_policy["id"])
                local_rows = complete_result_rows(
                    local_dir / "paired_policy_results_v1.csv", args.samples)
                if local_rows is None:
                    if local_dir.exists() and not args.resume:
                        raise RuntimeError(f"{point_name}: local output directory already exists")
                    source = policy_command(binary, n, f_group, args.samples, point["seed"],
                                            source_policy, local_dir, None)
                    commands.append(command_text(source))
                    invoke(source, output_root / "logs" / f"{point_name}_{source_policy['id']}.log")
                corpus_rows = read_csv(local_dir / "paired_corpus_v1.csv")
                if len(corpus_rows) != args.samples:
                    raise RuntimeError(f"{point_name}: source corpus is incomplete")
                corpus_id = corpus_rows[0]["corpus_id"]
                if any(row["corpus_id"] != corpus_id for row in corpus_rows):
                    raise RuntimeError(f"{point_name}: corpus ID is not unique")
                corpus_dir = output_root / "corpus" / point_name
                corpus_dir.mkdir(exist_ok=args.resume)
                paired_corpus = corpus_dir / "paired_corpus_v1.csv"
                if not paired_corpus.exists():
                    shutil.copy2(local_dir / "paired_corpus_v1.csv", paired_corpus)
                replay_file = corpus_dir / "corpus.simplified.faults"
                counts = materialize_replay(corpus_rows, replay_file, f_group) if not replay_file.exists() else [
                    tuple(len([] if not entries else entries.split(";"))
                          for entries in row["sa_local_faults"].split("|"))
                    for row in corpus_rows]
                stddevs = [statistics.pstdev(count) for count in counts]
                memberships, bin_summary = quantile_membership(stddevs)
                for entry in bin_summary:
                    entry.update(point)
                write_csv(corpus_dir / "fault_imbalance_bins.csv", bin_summary,
                          ["RS", "CS", "F_GROUP", "seed", "imbalance_bin", "method",
                           "lower_stddev", "upper_stddev", "groups"])
                corpus_index.append({**point, "point": point_name, "corpus_id": corpus_id,
                                     "paired_corpus_sha256": sha256(corpus_dir / "paired_corpus_v1.csv"),
                                     "simplified_fault_sha256": sha256(replay_file), "groups": len(counts)})

                rows_by_policy: dict[str, list[dict[str, str]]] = {}
                successes: dict[str, list[int]] = {}
                for policy in policies:
                    policy_id = str(policy["id"])
                    run_dir = point_raw / policy_id
                    existing_rows = complete_result_rows(
                        run_dir / "paired_policy_results_v1.csv", args.samples)
                    if policy_id != "local_no_sharing" and existing_rows is None:
                        command = policy_command(binary, n, f_group, args.samples, point["seed"],
                                                 policy, run_dir, replay_file)
                        commands.append(command_text(command))
                        invoke(command, output_root / "logs" / f"{point_name}_{policy_id}.log")
                    result_rows = existing_rows if existing_rows is not None else read_csv(
                        run_dir / "paired_policy_results_v1.csv")
                    if len(result_rows) != args.samples:
                        raise RuntimeError(f"{point_name}/{policy_id}: incomplete result rows")
                    if any(int(row["group_id"]) != index for index, row in enumerate(result_rows)):
                        raise RuntimeError(f"{point_name}/{policy_id}: duplicate or noncontiguous group_id")
                    try:
                        validate_policy_sidecar(result_rows, policy, point, corpus_id)
                    except RuntimeError as error:
                        raise RuntimeError(f"{point_name}/{error}") from error
                    expected_physical_lines = 4 * (n + n)
                    if any(value_int(row["total_physical_group"]) != expected_physical_lines
                           for row in result_rows):
                        raise RuntimeError(
                            f"{point_name}/{policy_id}: physical spare budget mismatch")
                    rows_by_policy[policy_id] = result_rows
                    successes[policy_id] = [value_int(row["repair_success"]) for row in result_rows]
                    summary_rows.append(aggregate_policy(point, policy, result_rows, stddevs))
                    for bin_index, summary in enumerate(bin_summary, start=1):
                        indices = [i for i, member in enumerate(memberships) if member == bin_index]
                        passed = sum(successes[policy_id][i] for i in indices)
                        imbalance_rows.append({**point, "corpus_id": corpus_id, "policy": policy_id,
                                               **summary, "successes": passed, "failures": len(indices) - passed,
                                               "repair_rate": passed / len(indices)})
                check_global_dominance(point_name, corpus_rows, successes, output_root)
                paired_rows.extend(paired_aggregate(point, successes))
                print(f"R3 completed {point_name}: {len(policies) * args.samples} policy-group evaluations", flush=True)

        local_rates = {(row["RS"], row["F_GROUP"]): row["repair_rate"]
                       for row in summary_rows if row["policy"] == "local_no_sharing"}
        for row in summary_rows:
            row["gain_vs_local_pp"] = 100.0 * (row["repair_rate"] - local_rates[(row["RS"], row["F_GROUP"])])
        for row in imbalance_rows:
            local = next(item for item in imbalance_rows
                         if item["RS"] == row["RS"] and item["F_GROUP"] == row["F_GROUP"] and
                         item["imbalance_bin"] == row["imbalance_bin"] and item["policy"] == "local_no_sharing")
            row["gain_vs_local_pp"] = 100.0 * (row["repair_rate"] - local["repair_rate"])
        for row in paired_rows:
            groups = row["left_pass_right_pass"] + row["left_pass_right_fail"] + row["left_fail_right_pass"] + row["left_fail_right_fail"]
            row["groups"] = groups
            row["left_fail_right_pass_rate"] = row["left_fail_right_pass"] / groups

        summary_fields = ["RS", "CS", "F_GROUP", "seed", "policy", "layout", "topology", "sharing_policy", "solution_policy", "solution_class", "candidate_contract", "search_scope", "backtracking", "paper_canonical", "m", "groups", "successes", "failures", "repair_rate", "ci95_low", "ci95_high", "gain_vs_local_pp", "mean_fault_stddev_success", "mean_fault_stddev_failure", "mean_remaining_rows", "mean_remaining_columns", "mean_borrowed_rows", "mean_borrowed_columns", "mean_search_nodes", "p95_search_nodes", "p99_search_nodes", "max_search_nodes", "cam_accounting_status"]
        paired_fields = ["RS", "CS", "F_GROUP", "seed", "comparison", "left_policy", "right_policy", "groups", "left_pass_right_pass", "left_pass_right_fail", "left_fail_right_pass", "left_fail_right_fail", "left_fail_right_pass_rate", "left_fail_right_pass_count"]
        imbalance_fields = ["RS", "CS", "F_GROUP", "seed", "corpus_id", "policy", "imbalance_bin", "method", "lower_stddev", "upper_stddev", "groups", "successes", "failures", "repair_rate", "gain_vs_local_pp"]
        write_csv(output_root / "aggregate" / "r3_repair_rate_summary.csv", summary_rows, summary_fields)
        write_csv(output_root / "aggregate" / "r3_paired_outcomes.csv", paired_rows, paired_fields)
        write_csv(output_root / "aggregate" / "r3_fault_imbalance.csv", imbalance_rows, imbalance_fields)
        write_csv(output_root / "manifest" / "corpus_index.csv", corpus_index,
                  ["RS", "CS", "F_GROUP", "seed", "point", "corpus_id", "paired_corpus_sha256", "simplified_fault_sha256", "groups"])
        (output_root / "logs" / "commands.txt").write_text("\n".join(commands) + "\n", encoding="utf-8")
        make_plots(output_root, summary_rows, paired_rows, imbalance_rows)
        manifest["completed_utc"] = dt.datetime.now(dt.timezone.utc).isoformat()
        manifest["status"] = "COMPLETE"
        (output_root / "manifest" / "r3_manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        (output_root / "manifest" / "COMPLETE.json").write_text(json.dumps({"status": "COMPLETE", "schema_version": SCHEMA}, indent=2) + "\n", encoding="utf-8")
        print(f"R3 formal sweep complete: {expected_evaluations} policy-group evaluations in {output_root}")
        return 0
    except Exception as error:
        (output_root / "manifest" / "PARTIAL.json").write_text(json.dumps({"status": "PARTIAL", "error": str(error)}, indent=2) + "\n", encoding="utf-8")
        print(f"R3 formal sweep partial: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
