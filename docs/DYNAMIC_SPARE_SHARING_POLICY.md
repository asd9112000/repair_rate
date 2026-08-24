# RECAM Dynamic Spare Sharing Simulation Specification

## 1. Objective

Extend the existing RECAM-based C++ simulator to evaluate resource-constrained spare sharing among four subarrays.

The four subarrays are arranged as:

```text
+---+---+
| A | B |
+---+---+
| C | D |
+---+---+
```

The simulator must evaluate the trade-off among:

* Repair Rate
* Spare Utilization
* Borrowed Spare Count
* CAM Storage Requirement
* Repair Matrix Size
* Number of Repair Candidates / Solutions
* Number of Analysis Attempts
* Estimated Repair Analysis Latency
* Hardware Cost Proxy

Do not rewrite the existing RECAM repair algorithm unless necessary.

---

# 2. Basic Parameters

Make the following values configurable:

```text
num_subarrays = 4

Rs = number of local spare rows per subarray
Cs = number of local spare columns per subarray

shared_row_num = m
shared_col_num = n

data_width
row_address_width
column_address_width

fault_count
simulation_runs
random_seed
```

Default experiment:

```text
Rs = 2
Cs = 2
num_subarrays = 4
```

---

# 3. Fault Model

Each simulation run generates faults independently for A, B, C, and D.

Support at least:

1. Uniform fault distribution
2. Non-uniform subarray fault distribution
3. Clustered / hotspot distribution
4. User-defined fault counts

Example:

```text
Uniform:
A = 5
B = 5
C = 5
D = 5

Moderate imbalance:
A = 2
B = 4
C = 5
D = 9

Strong imbalance:
A = 1
B = 2
C = 3
D = 14

Hotspot:
A = 1
B = 1
C = 1
D = 17
```

The actual fault objects should still contain:

```text
subarray_id
row_address
column_address
fault_type
```

Do not use only fault counts for redundancy analysis.

---

# 4. Baseline Policy: No Sharing

Each subarray owns:

```text
Rs spare rows
Cs spare columns
```

No spare can be borrowed.

Run the original RECAM repair analysis independently for A, B, C, and D.

For each subarray record:

```text
repair_success
used_rows
used_columns
unused_rows
unused_columns
pivot_fault_count
nonpivot_fault_count
address_CAM_entries_used
hybrid_CAM_entries_used
matrix_dimension
number_of_candidate_solutions
analysis_attempts
```

---

# 5. Policy 1: Directional Local Spare Sharing

No additional physical spare lines are introduced.

A, B, C, and D still have the same total physical number of spare lines as the baseline.

Let:

```text
sharedLine_num = m
```

Physical organization:

A and D:

```text
Cs local spare columns
Rs spare rows

m of the Rs rows are shareable with B/C.
```

B and C:

```text
Rs local spare rows
Cs spare columns

m of the Cs columns are shareable with A/D.
```

Functional capability:

A and D:

```text
available columns:
Cs <= C_available <= Cs + m

available rows:
Rs - m <= R_available <= Rs
```

B and C:

```text
available rows:
Rs <= R_available <= Rs + m

available columns:
Cs - m <= C_available <= Cs
```

Important:

A/D can borrow shared columns from B/C but cannot borrow shared rows.

B/C can borrow shared rows from A/D but cannot borrow shared columns.

A donor cannot lend the same spare line to multiple borrowers.

A borrowed spare must be unavailable to its original owner.

The simulator must track the ownership and current allocation of every shared spare.

---

# 6. Policy 2: Local + Global Spare Pool

Keep the total physical spare count equal to the baseline.

Example:

Baseline:

```text
4 subarrays × (2R + 2C)

Total:
8 spare rows
8 spare columns
```

For a global-pool experiment, redistribute some of these physical spares into:

```text
local spare pool
+
group-level global spare pool
```

Example:

```text
Each subarray:
1 local row
1 local column

Global group pool:
4 shared rows
4 shared columns
```

Total remains:

```text
8 rows
8 columns
```

Any subarray can request a spare from the global pool.

Track:

```text
global_rows_used
global_columns_used
requesting_subarray
remaining_global_rows
remaining_global_columns
```

---

# 7. Policy 3: Pairwise / Edge Sharing

Only physically adjacent subarrays may share redundancy.

Use:

```text
Horizontal column-sharing pairs:
A <-> B
C <-> D

Vertical row-sharing pairs:
A <-> C
B <-> D
```

No diagonal sharing:

```text
A cannot directly borrow from D
B cannot directly borrow from C
```

This policy is intended to reduce:

* routing complexity
* mux complexity
* number of borrowing candidates
* repair-analysis search space

while retaining more flexibility than no sharing.

---

# 8. Policy 4: Local-First Overflow Sharing

Always attempt local RECAM repair first.

Procedure:

```text
Stage 1:
Run RECAM using only local Rs and Cs.

If repair succeeds:
stop.

If repair fails:
enter Stage 2.

Stage 2:
allow at most one borrowed spare.

If repair still fails:
optionally enter Stage 3.

Stage 3:
allow additional borrowed spares up to the configured maximum.
```

Record separately:

```text
local_repair_success
repair_after_1_borrow
repair_after_2_borrows
repair_after_3_borrows
```

Also record:

```text
number_of_extra_analysis_attempts
```

The simulator should not enumerate maximum sharing cases when local repair has already succeeded.

---

# 9. Policy 5: Minimum Local Reserve

Each subarray must retain a guaranteed minimum redundancy.

Parameters:

```text
R_min
C_min
```

A subarray may lend a spare only when:

```text
remaining_rows > R_min
```

or

```text
remaining_columns > C_min
```

Example:

```text
Rs = Cs = 2
R_min = C_min = 1
```

Therefore a subarray can never lend its final local spare row or column.

Sweep:

```text
R_min = 0, 1, 2
C_min = 0, 1, 2
```

---

# 10. Policy 6: Single-Dimension Borrowing

For each failed subarray, identify whether the repair deficit is row-dominant or column-dominant.

Allow borrowing from only one dimension in one repair attempt.

Example:

```text
local capacity = 2R + 2C

Allowed:
3R + 2C
or
2R + 3C

Not allowed:
3R + 3C
```

This policy is intended to prevent unnecessary growth of:

```text
Address CAM requirement
Hybrid CAM requirement
Matrix dimension
candidate repair solutions
```

---

# 11. Hardware-Cost Accounting

Do not only record the number of spares.

For every repair configuration calculate:

## Address CAM

Record:

```text
address_CAM_entries_active
address_CAM_entries_peak
address_CAM_entries_provisioned
```

Important distinction:

Active:
entries actually used in the current repair.

Peak:
maximum entries used during this run.

Provisioned:
hardware entries required to support the worst legal configuration of the policy.

Do not treat these three values as equivalent.

---

## Hybrid CAM

Record:

```text
hybrid_CAM_entries_active
hybrid_CAM_entries_peak
hybrid_CAM_entries_provisioned
```

Use the existing RECAM implementation / sizing rule.

Do not invent a new Hybrid-CAM sizing equation.

---

## Hybrid CAM Bit Cost

Make entry width configurable.

Record:

```text
hybrid_CAM_entry_width_bits
total_hybrid_CAM_bits
```

The simulator should support different data widths such as:

```text
16
32
64
128
256
```

so that the effect of WoW data width can be evaluated.

---

## Matrix

For every RECAM analysis:

```text
matrix_dimension = R_available + C_available
matrix_cells =
    matrix_dimension * matrix_dimension
```

Track both:

```text
active_matrix_cells
provisioned_matrix_cells
```

---

# 12. Repair Search Complexity

For a configuration containing R available spare rows and C available spare columns:

```text
candidate_solutions =
    choose(R + C, R)
```

Record:

```text
candidate_solutions
candidate_solutions_evaluated
failed_candidates
successful_candidate_index
```

Do not assume all candidates must always be evaluated.

---

# 13. Analysis Latency Model

Separate latency into components:

```text
fault_information_insert_cycles

fault_information_lookup_cycles

fault_information_read_cycles

matrix_generation_cycles

solution_generation_cycles

solution_evaluation_cycles

sharing_allocation_cycles
```

Calculate:

```text
total_analysis_cycles
```

Support two storage modes:

```text
CAM
SRAM
```

CAM mode and SRAM mode must use configurable latency parameters.

Do not hard-code SRAM as one-cycle.

---

# 14. Sharing-Specific Metrics

Record:

```text
borrowed_rows
borrowed_columns

lent_rows
lent_columns

borrow_requests
successful_borrows
failed_borrows

donor_starvation_count

repair_success_due_to_sharing

repair_failure_even_after_sharing
```

Define:

```text
sharing_gain =
repair_success_with_sharing
-
repair_success_without_sharing
```

---

# 15. Spare Utilization

Calculate:

```text
row_utilization =
used_spare_rows / physical_spare_rows

column_utilization =
used_spare_columns / physical_spare_columns

total_spare_utilization =
used_spares / total_physical_spares
```

Also calculate unused redundancy after repair.

---

# 16. Repair Efficiency Metrics

In addition to repair rate calculate:

```text
repair_gain_per_extra_CAM_entry

repair_gain_per_matrix_cell

repair_gain_per_analysis_cycle

repair_gain_per_borrowed_spare
```

These are important because a policy may improve repair rate while having poor hardware efficiency.

---

# 17. Main Parameter Sweep

Run at least:

```text
Rs = Cs = 2

sharedLine_num:
0
1
2
```

For each policy evaluate:

```text
Uniform
Moderate imbalance
Strong imbalance
Hotspot
```

For each combination run sufficiently many Monte Carlo trials.

Start with:

```text
10,000 trials
```

---

# 18. Required Output CSV

Each simulation run should generate one row containing:

```text
seed
fault_model
policy
Rs
Cs
shared_rows
shared_columns

fault_A
fault_B
fault_C
fault_D

repair_A
repair_B
repair_C
repair_D

group_repair_success

used_rows
used_columns

borrowed_rows
borrowed_columns

address_CAM_active
address_CAM_peak
address_CAM_provisioned

hybrid_CAM_active
hybrid_CAM_peak
hybrid_CAM_provisioned

hybrid_CAM_bits

matrix_cells_active
matrix_cells_provisioned

candidate_solutions
candidate_solutions_evaluated

analysis_attempts
analysis_cycles
```

---

# 19. Summary Output

After all Monte Carlo runs generate:

```text
Repair Rate
Average Spare Utilization
Average Borrow Count
P95 Borrow Count
Average CAM Requirement
Peak CAM Requirement
Average Matrix Size
Peak Matrix Size
Average Candidate Count
Average Analysis Attempts
Average Analysis Cycles
P95 Analysis Cycles
```

for every sharing policy and every fault distribution.

---

# 20. Important Implementation Requirement

Keep three concepts separate:

```text
physical spare count
effective accessible spare count
hardware analysis capacity
```

For example, a subarray may physically own:

```text
2R + 2C
```

but sharing may allow it to functionally access:

```text
2R + 3C
```

This must not automatically imply that physical spare count increased.

However, the RECAM analysis hardware may still need to be provisioned for the maximum supported effective configuration.

Track these quantities separately.
