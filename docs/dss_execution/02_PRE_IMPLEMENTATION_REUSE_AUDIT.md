# Pre-Implementation Reuse Audit Template

> 文件狀態：Template
> 適用範圍：DSS thesis execution; mandatory before RTL implementation
> 建立時間：2026-09-13T06:28:23+08:00
> 最後修改時間：2026-09-13T06:28:23+08:00

```text
PRE_IMPLEMENTATION_REUSE_AUDIT = MANDATORY
```

Use one completed instance of this template before authorizing any RTL change or
new RTL module.  It is an evidence artifact, not permission by itself.

## Target

```text
Target Phase:
Target Architecture Change:
Target policy/topology/resource point:
```

## Existing implementation and parameterization

| Item | Findings / evidence | Classification |
|---|---|---|
| Existing Modules | | |
| Existing Parameters | | |
| Verified Parameter Points | | |
| Hard-Coded Assumptions | | |

Classify relevant blocks using only:

```text
PARAMETERIZED_AND_VERIFIED
PARAMETERIZED_BUT_UNVERIFIED_FOR_TARGET
PARTIALLY_PARAMETERIZED
FIXED_TO_2_2_1
FIXED_CONFIG_TABLE
REUSABLE_REFERENCE_ONLY
REQUIRES_EXTENSION
REQUIRES_REDESIGN
```

The audit must inspect configuration decode, candidate tables, PatternID
width/order, `MAX_K`, matrix and CAM capacities, Hybrid capacity, transpose,
topology resources, ledger/donor encoding, role order, test assumptions, and
synthesis parameters.

## Reuse inventory

| Category | Candidate reuse | Compatibility / evidence | Required action |
|---|---|---|---|
| Reusable RTL | | | |
| Reusable Testbenches | | | |
| Reusable Golden Models | | | |
| Reusable Scripts | | | |
| Reusable Report Parsers | | | |
| Reusable Synthesis Constraints | | | |

## Frozen dependencies

```text
Frozen Semantics:
Frozen Interfaces:
Frozen Evidence:
```

State whether the candidate implementation preserves the required architecture,
policy, interface, golden, and baseline-result boundaries.  Identify every
shared-module regression risk to the frozen 2,2,1 baseline.

## Delta and validation

```text
Minimal Required Delta:
Regression Risks:
New Tests Required:
Reusable directed/random/golden regressions:
Reusable synthesis flow and report schema:
```

## Required ten-question conclusion

1. Which existing modules already implement related behavior?
2. Which parameters are genuinely supported, versus merely present?
3. Does reuse preserve the required architecture semantics?
4. Which interfaces, golden semantics, policies, and results are frozen?
5. Which verification can be reused?
6. Which top-levels, scripts, parsers, constraints, and report schemas reuse?
7. What is the smallest required architectural delta?
8. What hidden hard-coding exists?
9. Can shared-module edits regress the frozen baseline?
10. Which one recommendation follows?

```text
Recommendation: <select exactly one>
REUSE_AS_IS
EXTEND_EXISTING
PARAMETERIZE_EXISTING
NEW_MODULE_JUSTIFIED
REDESIGN_REQUIRED
ARCHITECTURALLY_NOT_JUSTIFIED

Authorization State:
RTL_IMPLEMENTATION_AUTHORIZED = YES / NO
```

If any required inspection or conclusion is incomplete, record `NO`.
