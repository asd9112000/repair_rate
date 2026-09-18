#!/usr/bin/env python3
"""Derive canonical R3 metadata without rewriting frozen raw sidecars."""

from __future__ import annotations

import argparse
import csv
import json
import hashlib
from pathlib import Path


FIELDS = [
    "N", "RS", "CS", "F_GROUP", "seed", "group_id", "corpus_id",
    "corpus_hash", "canonical_policy_id", "implementation_policy_id",
    "solution_class", "candidate_contract", "priority_class", "search_scope",
    "backtracking", "paper_canonical", "legacy_alias_of", "layout", "topology",
    "share_row", "share_col", "raw_policy_id", "repair_success",
]


def priority_class(descriptor: dict[str, object]) -> str:
    if "priority_class" in descriptor:
        return str(descriptor["priority_class"])
    solution_class = str(descriptor["solution_class"])
    if solution_class == "LOCAL": return "LOCAL_ONLY"
    if solution_class == "LOCAL_FIRST": return "LOCAL_FIRST"
    if solution_class == "EARLY": return "RELEASE_AWARE"
    if solution_class == "PAIR_GLOBAL": return "PAIR_JOINT_ORACLE"
    return "JOINT_ORACLE"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input-root", required=True, type=Path)
    args = parser.parse_args()
    root = args.input_root.resolve()
    manifest = json.loads((root / "manifest" / "r3_manifest.json").read_text())
    output = root / "aggregate" / "r3_normalized_policy_metadata.csv"
    unmappable = 0
    ambiguous = 0
    written = 0
    with output.open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=FIELDS)
        writer.writeheader()
        for n_text, faults in manifest["points"].items():
            n = int(n_text)
            descriptors = {item["id"]: item for item in manifest["policy_matrix"][n_text]}
            for f_group in faults:
                point = f"n{n}_f{f_group}"
                corpus_hash = sha256(root / "corpus" / point / "paired_corpus_v1.csv")
                for canonical_id, descriptor in descriptors.items():
                    sidecar = root / "raw" / point / canonical_id / "paired_policy_results_v1.csv"
                    if not sidecar.is_file():
                        unmappable += 1
                        continue
                    with sidecar.open(newline="", encoding="utf-8") as source:
                        rows = list(csv.DictReader(source))
                    for row in rows:
                        implementation = row["implementation_policy_id"] if "implementation_policy_id" in row else row["policy_id"]
                        if implementation != descriptor["solution"]:
                            unmappable += 1
                            continue
                        writer.writerow({
                            "N": n, "RS": n, "CS": n, "F_GROUP": f_group,
                            "seed": row.get("seed", manifest["seed_contract"][f"n{n}_f{f_group}"]),
                            "group_id": row["group_id"], "corpus_id": row["corpus_id"],
                            "corpus_hash": row.get("corpus_hash", corpus_hash),
                            "canonical_policy_id": canonical_id,
                            "implementation_policy_id": implementation,
                            "solution_class": descriptor["solution_class"],
                            "candidate_contract": descriptor["candidate_contract"],
                            "priority_class": priority_class(descriptor),
                            "search_scope": descriptor["search_scope"],
                            "backtracking": str(descriptor["backtracking"]).lower(),
                            "paper_canonical": str(descriptor["paper_canonical"]).lower(),
                            "legacy_alias_of": "-", "layout": descriptor["layout"],
                            "topology": descriptor["topology"], "share_row": descriptor["share_row"],
                            "share_col": descriptor["share_col"], "raw_policy_id": row["policy_id"],
                            "repair_success": row["repair_success"],
                        })
                        written += 1
    print(json.dumps({"output": str(output), "rows": written,
                      "unmappable_results": unmappable,
                      "ambiguous_mappings": ambiguous}, sort_keys=True))
    return 0 if unmappable == 0 and ambiguous == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
