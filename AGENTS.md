# Repository Agent Instructions

You are working in a C++17 simulation repository for RECAM and hierarchical repair modeling.

## Project purpose

This repository models repair behavior for multiple hardware architectures, including:
- legacy / group-level RECAM paths
- hierarchical-RECAM-v2 device-wide sharing logic
- spare-line sharing and SRAM-RECAM variants

The goal is to keep simulation semantics faithful to the hardware model and to preserve experiment boundaries. Results from different architecture families are not interchangeable unless explicitly justified by the documentation.

## Read first

Before making changes, read the project entry documents and architecture references in this order:
1. project README / overview
2. architecture design notes
3. hierarchical RECAM specification
4. experiments / validation notes
5. RECAM specific behavior and hardware details

Focus on the documents that define:
- architecture boundaries
- resource ownership
- result semantics
- CLI parameter contracts
- output schema expectations

## Critical constraints

Follow these rules at all times:

- Do not mix legacy/group-level results with device-wide hierarchical results.
- Do not assume that repair-rate or capacity metrics are comparable across different architecture families.
- Preserve the distinct semantics of each simulator path.
- Do not rename CLI parameters or output fields casually.
- Do not broaden the scope of a fix unless it is necessary to resolve the root cause.
- Prefer the smallest correct change over large refactors.
- Preserve naming conventions, code organization, and simulation intent.

## Architecture-aware workflow

Before patching:
- Identify which architecture family the change belongs to.
- Confirm whether the code is in the legacy path or the hierarchical path.
- Check whether any experiment assumptions or output formats must remain unchanged.
- Trace the data flow before modifying logic.

If the issue is about a simulator behavior:
- identify the exact model or resource policy involved
- verify whether the bug is in the solver, resource bookkeeping, scheduling, or reporting logic
- fix the root cause rather than patching symptoms

## Coding expectations

- Keep the implementation consistent with the existing C++ style.
- Favor surgical edits with clear intent.
- Avoid unrelated cleanup while working on a fix.
- If behavior or CLI changes are required, update the relevant documentation in the same change.
- Keep comments factual and source-anchored; do not add speculative explanations.

## Build and validation

Use the project Makefile for compilation and validation.

Common commands:
- make all
- make hierarchical_recam_b
- make test_hierarchical_recam
- make test

Validation rules:
- Run the smallest relevant command that checks the changed behavior.
- Do not claim success without running the validation command.
- If a small focused check is not possible, explain the limitation clearly.
- When a fix changes behavior, verify the affected simulation path and relevant outputs.

## Change reporting

When reporting work, include:
- brief root cause summary
- files changed
- validation command used
- result of validation
- any remaining uncertainty or blocker

If validation fails:
- explain the failure precisely
- do not claim the fix works
- propose the next minimal debugging step

## Do not

- reinterpret results across architecture families without explicit evidence
- merge incompatible experiment outputs
- refactor unrelated code while fixing a targeted bug
- change parameter semantics without updating documentation
- assume a patch is valid without running the relevant build/test command
- make broad sweeping edits to satisfy a local symptom

## Final operating principle

Maintain fidelity to the hardware model, preserve experiment integrity, and prefer minimal, evidence-based corrections over broad changes. The purpose of this repository is research-grade simulation, not casual API reshaping or cross-architecture normalization.