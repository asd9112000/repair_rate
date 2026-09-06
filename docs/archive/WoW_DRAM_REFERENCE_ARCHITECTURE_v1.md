# WoW DRAM Reference Architecture v1.0
## Baseline configuration for hierarchical RECAM / spare-sharing experiments

> 文件狀態：Archive
> 適用範圍：hierarchical-device
> 建立時間：Unknown
> 最後修改時間：2026-09-02T00:00:00+08:00
> 本文件權威主題：歷史版 WoW reference architecture；目前參數以 HIERARCHICAL_RECAM.md 為準

**Purpose.** This file freezes a single reference architecture for simulation. It separates: (1) parameters directly stated in a published WoW memory–logic architecture; (2) conventional DRAM microarchitecture parameters from SALP; (3) derived values; and (4) assumptions introduced by this research.

Do **not** present the entire table as a commercial product specification. The top-level organization is Micron-inspired; the subarray decomposition is a research model.

---

# 1. Primary references

## [R1] Micron WoW memory–logic architecture

Micron Technology, Inc., **“Input/Output Connections of Wafer-on-Wafer Bonded Memory and Logic,”** U.S. Patent Application US20230048628A1, published Feb. 16, 2023.

Relevant disclosed example:
- memory wafer wafer-on-wafer bonded to a logic wafer;
- 32 memory banks per die;
- 1 Gbit per bank;
- 32 Gbit total memory capacity;
- 64 sections per bank;
- 16 Mbit per section;
- 32 logic clusters corresponding to the 32 banks in the example;
- 8 GBUS interfaces between memory die and logic die;
- each GBUS is coupled to 4 banks;
- GBUS width = 256 bits;
- each GBUS services 4 Gbit of memory;
- LBUS width = 256 bits per 16-Mbit memory section.

Primary source: https://patents.google.com/patent/US20230048628A1/en

Equivalent Micron patent description: https://www.freepatentsonline.com/y2023/0051235.html

**Important:** the patent explicitly treats these as non-limiting examples. These values are an architectural anchor, not a universal WoW standard.

## [R2] DRAM subarray / tile organization

Y. Kim, V. Seshadri, D. Lee, J. Liu, and O. Mutlu, **“A Case for Exploiting Subarray-Level Parallelism (SALP) in DRAM,”** Proc. 39th IEEE/ACM International Symposium on Computer Architecture (ISCA), 2012. DOI: 10.1109/ISCA.2012.6237032.

Relevant organization:
- example DRAM bank: 32K rows × 8K cells per row;
- a bank is physically divided into a 2-D array of tiles;
- a typical tile cell array is 512 cells × 512 cells;
- all tiles in one horizontal row share global wordlines and activate/precharge in lockstep;
- one horizontal “row of tiles” is abstracted as one **subarray**;
- each subarray has a local row buffer and subarray row decoder;
- the example uses 512 rows per subarray.

Project page: https://www.istc-cc.cmu.edu/publications/papers/2012/SALP-isca12_abs.shtml

DOI: https://doi.org/10.1109/ISCA.2012.6237032

## [R3] RECAM baseline repair architecture

J. Yoon, H. Lee, Y. Moon, S. H. Shin, and S. Kang, **“A Built-In Self-Repair With Maximum Fault Collection and Fast Analysis Method for HBM,”** IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, vol. 44, no. 5, pp. 2014–2025, May 2025. DOI: 10.1109/TCAD.2024.3499903.

Relevant RECAM facts:
- HBM logic die contains the BISR hardware;
- BIRA fault collection uses an Address CAM and Hybrid CAM;
- the repair-analysis flow selects channels and processes them through the BISR;
- the paper states that CAM redundancy resources are available to all channels;
- offline Address CAM stores pivot-fault information;
- offline Hybrid CAM stores compressed nonpivot-fault information;
- online Address CAM stores fault addresses and channel information;
- online Hybrid CAM stores replacement data words;
- Address CAM entries = `Rs + Cs`;
- Hybrid CAM entries = `Rs(Cs - 1) + Cs(Rs - 1)`;
- examples: Rs=Cs=2 -> 4/4 entries; Rs=Cs=3 -> 6/12; Rs=Cs=4 -> 8/24;
- RECAM is a dynamic BIRA: software loop iterations are not hardware BIRA cycles.

DOI: https://doi.org/10.1109/TCAD.2024.3499903

---

# 2. Frozen reference configuration

| Hierarchy / parameter | Baseline value | How obtained | Evidence status |
|---|---:|---|---|
| Total DRAM capacity | **32 Gbit = 4 GiB** | 32 × 1 Gbit banks | **Directly from R1** |
| Total banks | **32** | R1 example | **Directly from R1** |
| Bank capacity | **1 Gbit = 128 MiB** | R1 example | **Directly from R1** |
| Sections per bank | **64** | R1 example | **Directly from R1** |
| Section capacity | **16 Mbit = 2 MiB** | R1 example | **Directly from R1** |
| GBUS domains | **8** | R1 example | **Directly from R1** |
| Banks per GBUS | **4** | R1 example | **Directly from R1** |
| GBUS width | **256 bits** | R1 example | **Directly from R1** |
| LBUS width | **256 bits per section** | R1 example | **Directly from R1** |
| Logic clusters | **32** | R1 example | **Directly from R1** |
| Banks per logic cluster | **1 in example** | 32 clusters / 32 banks | **Directly from R1 example** |
| Tile cell array | **512 × 512 cells** | SALP typical tile | **Literature-based R2** |
| Rows per subarray | **512** | SALP organization | **Literature-based R2** |
| Cells per modeled subarray row | **8192 bits** | SALP-like horizontal width | **Research modeling choice supported by R2 example** |
| Tiles per modeled subarray | **16 horizontal tiles** | 8192 / 512 | **Derived** |
| Subarray capacity | **4 Mbit = 512 KiB** | 512 × 8192 bits | **Derived modeling value** |
| Subarrays per 16-Mbit section | **4** | 16 / 4 Mbit | **Our modeling assumption** |
| 4-SA sharing group capacity | **16 Mbit = 2 MiB** | 4 × 4 Mbit | **Derived from our model** |
| Sharing groups per bank | **64** | one group mapped to each R1 section | **Our modeling assumption** |
| Subarrays per bank | **256** | 64 × 4 | **Derived** |
| Total sharing groups | **2048** | 32 × 64 | **Derived** |
| Total subarrays | **8192** | 2048 × 4 | **Derived** |
| Local spare rows / SA | **Rs = 2** | RECAM baseline | **R3-based baseline** |
| Local spare columns / SA | **Cs = 2** | RECAM baseline | **R3-based baseline** |
| Primary WoW data granularity | **256 bits** | LBUS/GBUS baseline | **WoW-inspired modeling choice from R1** |
| Sharing group topology | **4 SA arranged 2×2** | project proposal | **Our architecture** |
| Main inter-SA sharing | **constrained, row-first** | project proposal | **Our architecture** |

---

# 3. Simulator hierarchy

```text
WoW Device — 32 Gbit / 4 GiB
│
├── GBUS-domain 0  [modeled as a channel-like domain]
│   ├── Bank 0 — 1 Gbit
│   │   ├── Section / Repair Group 0 — 16 Mbit
│   │   │   ├── Subarray A — 4 Mbit
│   │   │   ├── Subarray B — 4 Mbit
│   │   │   ├── Subarray C — 4 Mbit
│   │   │   └── Subarray D — 4 Mbit
│   │   ├── ...
│   │   └── Group 63
│   ├── Bank 1
│   ├── Bank 2
│   └── Bank 3
├── ...
└── GBUS-domain 7
    └── 4 banks
```

**Terminology rule:** R1 calls these interfaces **GBUS**, not JEDEC HBM channels. In this research each four-bank GBUS service domain is abstracted as one **channel-like domain**.

Recommended wording:

> “We model each four-bank GBUS domain in the referenced Micron WoW architecture as one channel-like memory partition.”

---

# 4. Subarray model

One modeled subarray:

```text
Rows          = 512
Bits per row  = 8192
Capacity      = 512 × 8192
              = 4,194,304 bits
              = 4 Mbit
              = 512 KiB
```

Physical abstraction:

```text
                     8192 cells / row

+------+------+------+------+----- ... -----+------+
|512×  |512×  |512×  |512×  |              |512×  |
|512   |512   |512   |512   |              |512   |
+------+------+------+------+----- ... -----+------+
  #0     #1     #2     #3                    #15

        16 horizontal 512×512 tiles
        operating as one modeled subarray
```

R2 supports the concepts of a typical 512×512 tile, 512-row subarray, and 8K-cell row width. The exact statement **“a Micron 16-Mbit section physically contains four such subarrays” is NOT in R1**. That decomposition is our modeling assumption.

---

# 5. Four-subarray repair-sharing domain

```text
+-----------+-----------+
|     A     |     B     |
|   4 Mbit  |   4 Mbit  |
+-----------+-----------+
|     C     |     D     |
|   4 Mbit  |   4 Mbit  |
+-----------+-----------+

Group total = 16 Mbit
```

Each SA initially owns:

```text
2 local spare rows
2 local spare columns
```

Each group therefore contains physically:

```text
8 spare rows
8 spare columns
```

Total physical redundancy must remain fixed for no-sharing vs sharing comparisons unless the experiment explicitly studies added redundancy.

---

# 6. Capacity consistency

```text
4 Mbit / SA × 4 SA / group = 16 Mbit / group
16 Mbit / group × 64 groups / bank = 1024 Mbit = 1 Gbit / bank
1 Gbit / bank × 32 banks = 32 Gbit = 4 GiB
```

---

# 7. Channel-like-domain consistency

```text
8 GBUS × 4 banks / GBUS = 32 banks
4 banks × 1 Gbit = 4 Gbit serviced by one domain
8 × 4 Gbit = 32 Gbit total
```

---

# 8. Address hierarchy

For a 4-GiB byte-addressable model:

| Field | Count | Bits |
|---|---:|---:|
| GBUS/channel-like domain | 8 | 3 |
| Bank within domain | 4 | 2 |
| Group/section within bank | 64 | 6 |
| Subarray within group | 4 | 2 |
| Row within subarray | 512 | 9 |
| Byte within 8192-bit / 1024-byte row | 1024 | 10 |
| **Total** |  | **32 bits** |

```text
[ Domain ][ Bank ][ Group ][ SA ][ Row ][ ByteOffset ]
    3b       2b      6b      2b    9b       10b
```

For a 256-bit = 32-byte modeled data word:

```text
1024 bytes / row ÷ 32 bytes / word = 32 words / row
ColumnWord = 5 bits
ByteInWord = 5 bits
```

Word-level repair tag before metadata:

```text
Domain      3
Bank        2
Group       6
SA          2
Row         9
ColumnWord  5
----------------
Total      27 bits
```

The 27-bit value is derived from this model, not directly stated by R1 or R3.

---

# 9. Cell column vs 256-bit repair word

Local line repair may use physical cell coordinates:

```text
row      = 0..511
cell_col = 0..8191
```

For online CAM/SRAM reuse at 256-bit data-word granularity:

```text
word_col = floor(cell_col / 256)
word_col = 0..31
```

Possible model rule:

> Multiple residual faulty cells in the same 256-bit word consume one online data-replacement entry because the full word is redirected.

This is a **research modeling assumption** and should remain configurable.

---

# 10. Local spare cell-overhead sanity check

Per SA:

```text
Normal cells  = 512 × 8192 = 4,194,304
2 spare rows  = 2 × 8192   = 16,384
2 spare cols  = 2 × 512    = 1,024
Spare cells   = 17,408
```

```text
17,408 / 4,194,304 ≈ 0.415%
```

This is only a **cell-array redundancy ratio**. It excludes spare decoders, eFuse, BIST/BIRA, CAM/SRAM, comparators, matrix logic, sharing muxes/routing, and logic-die interconnect.

---

# 11. RECAM hardware-placement correction

The project must no longer model RECAM CAM as a separate physical CAM for every tile or subarray.

R3 shows the BISR on the logic die:

```text
DRAM dies
     │
     ▼
Logic die
+-------------------------------+
| BISR                          |
|   BIST                        |
|   BIRA                        |
|     Address CAM               |
|     Hybrid CAM                |
|     Matrix / analyzer         |
+-------------------------------+
```

R3 also selects channels through the repair-analysis flow, includes channel bits in online Address-CAM entries, and states that CAM redundancy resources are available to all channels.

Default interpretation:

> RECAM CAM is a **logic-die BISR resource**, time-multiplexed during offline analysis and reused as a shared online redundancy resource, not `CAM_per_tile × number_of_tiles`.

See `CODEX_HIERARCHICAL_RECAM_SIM_SPEC_v2.md` for the simulator contract.

---

# 12. Baseline configuration block

```yaml
architecture_version: WoW-v1.0

total_capacity_gbit: 32

gbus_domains: 8
banks_per_domain: 4
total_banks: 32
bank_capacity_gbit: 1

groups_per_bank: 64
group_capacity_mbit: 16

subarrays_per_group: 4
subarrays_per_bank: 256
total_subarrays: 8192

subarray_rows: 512
subarray_cols_bits: 8192
subarray_capacity_mbit: 4

tile_rows: 512
tile_cols: 512
tiles_per_subarray: 16

spare_rows_per_sa: 2
spare_cols_per_sa: 2

data_word_bits: 256
```

Suggested sensitivity parameters:

```yaml
shared_rows_m: [0, 1, 2]
online_global_reuse_entries: [4, 8, 16, 32, 64, 128, 256]
bira_engine_count: [1, 2, 4, 8]
data_word_bits_sensitivity: [16, 64, 128, 256]
```

---

# 13. Citation-ready methodology paragraph

> We adopt a Micron-disclosed wafer-on-wafer memory–logic architecture as the top-level reference. The disclosed example contains a 32-Gbit memory die organized into 32 1-Gbit banks, with each bank divided into 64 16-Mbit sections. Eight 256-bit global buses (GBUSes) each service four banks, while a 256-bit local bus (LBUS) is associated with each 16-Mbit section [R1]. Since the patent does not disclose the internal DRAM subarray organization of a section, we further use the conventional subarray organization described by Kim et al. [R2], in which a typical tile contains a 512×512-cell array and a horizontal row of tiles forms a subarray. For simulation, each 16-Mbit section is therefore modeled as a four-subarray repair domain, with each 4-Mbit subarray containing 512 rows × 8192 cells per row. This four-subarray decomposition is a modeling assumption rather than a claimed Micron physical implementation.

---

# 14. Things that must NOT be claimed

Do not write:
- “WoW DRAM universally has 8 channels.”
- “WoW DRAM word width is always 256 bits.”
- “Micron says each 16-Mbit section contains exactly four subarrays.”
- “Every WoW subarray is exactly 512×8192.”
- “RECAM has one CAM per subarray/tile.”
- “0.415% is the complete BISR area overhead.”

Prefer:
- “Micron-inspired WoW reference architecture.”
- “GBUS domain modeled as a channel-like partition.”
- “256-bit interface used as the baseline data granularity.”
- “Four-subarray section decomposition is our modeling assumption.”
- “RECAM CAM is modeled as logic-die BISR hardware shared/time-multiplexed across repair domains.”

---

# 15. Version-control rule

Treat this document as architecture **v1.0**. Any future change to bank count, section count, subarrays per section, SA dimensions, data width, spare count, BISR engine count, or CAM-reuse scope must increment the architecture version and be recorded in experiment metadata.
