#!/usr/bin/env python3
"""Assert a fresh R3 sidecar carries canonical, not inferred, metadata."""
import argparse
import csv
import json
from pathlib import Path

REQUIRED = {"canonical_policy_id", "implementation_policy_id", "priority_class",
            "paper_canonical", "legacy_alias_of", "layout", "topology",
            "share_row", "share_col", "N", "RS", "CS", "F_GROUP", "seed",
            "corpus_id", "corpus_hash"}

parser = argparse.ArgumentParser()
parser.add_argument("--root", required=True, type=Path)
args = parser.parse_args()
manifest = json.loads((args.root / "manifest" / "r3_manifest.json").read_text())
for n, policies in manifest["policy_matrix"].items():
    for policy in policies:
        sidecar = next((args.root / "raw").glob(f"n{n}_f*/{policy['id']}/paired_policy_results_v1.csv"))
        with sidecar.open(newline="", encoding="utf-8") as source:
            row = next(csv.DictReader(source))
        assert REQUIRED <= set(row), policy["id"]
        assert row["canonical_policy_id"] == policy["id"]
        assert row["implementation_policy_id"] == policy["solution"]
        assert row["solution_class"] == policy["solution_class"]
        assert row["priority_class"] == policy["priority_class"]
        assert row["layout"] == policy["layout"]
        assert row["topology"] == policy["topology"]
        assert row["share_row"] == str(policy["share_row"])
        assert row["share_col"] == str(policy["share_col"])
        if policy["id"] == "local_no_sharing":
            assert (row["solution_class"], row["candidate_contract"], row["priority_class"], row["search_scope"]) == ("LOCAL", "LOCAL", "LOCAL_ONLY", "LOCAL")
        if policy["id"] == "two_pairwise_m1_pair_global":
            assert row["solution_class"] == "PAIR_GLOBAL"
print("r3 metadata contract PASS")
