

# DRAM Capacity and Address Structure

本筆記整理 hierarchical RECAM 模擬所使用的 DRAM baseline，以及不同
`word_bits` 對 address 結構的影響。

## 1. Reference baseline

主要依據：

1. `docs/HIERARCHICAL_RECAM.md`
2. `HierarchicalRECAM.cpp`
3. `experiments/moderate_repair_study.json`
4. `scripts/device/canonical_four/sweep.py`

### DRAM hierarchy

```text
Device: 32 Gbit = 4 GiB
└── 8 channel-like domains
	└── 4 banks per domain
		└── 64 repair groups per bank
			└── 4 subarrays per group: A/B/C/D
				└── 512 rows
					└── 8192 physical cell columns per row
```

Derived totals:

| Item | Value |
|---|---:|
| Total banks | `8 x 4 = 32` |
| Total repair groups | `8 x 4 x 64 = 2048` |
| Total subarrays | `2048 x 4 = 8192` |
| Capacity / subarray | `512 x 8192 = 4 Mbit` |
| Capacity / repair group | `4 x 4 Mbit = 16 Mbit` |
| Capacity / bank | `64 x 16 Mbit = 1 Gbit` |
| Device capacity | `32 x 1 Gbit = 32 Gbit = 4 GiB` |

The simulator normally models only the first `N` repair groups for an experiment
(`--groups`), but the hardware geometry remains the full 2048-group device.

Common simulation parameters:

- `memory_rows = 512`
- `memory_columns = 8192`
- `Rs = Cs = 2` or `3` in the moderate repair study
- `data-word-bits = 256` in the baseline study
- `bira-engines = 1` unless explicitly overridden
- `cam-granularity = word` unless explicitly overridden

The moderate study uses `word_bits = 256`, fault model `moderate`, and spatial
model `mixed`. Its device profiles model 2, 32, or 64 groups; this is a sample
count, not a change to the physical DRAM capacity.

## 2. Address structure: `word_bits = 256`

### Physical cell address

```text
[domain:3][bank:2][group:6][subarray:2][row:9][cell_column:13]
								  total = 35 bits
```

Bit calculation:

| Field | Range | Bits |
|---|---:|---:|
| Domain | `8` | `log2(8) = 3` |
| Bank | `4` | `log2(4) = 2` |
| Repair group | `64` | `log2(64) = 6` |
| Subarray | `4` | `log2(4) = 2` |
| Row | `512` | `log2(512) = 9` |
| Physical cell column | `8192` | `log2(8192) = 13` |
| **Total** |  | **35** |

`HBMID` is fixed to one device in this model and therefore contributes 0 bits.
The 35-bit result also matches the total cell capacity: `2^35 bits = 32 Gbit`.

### Word-level address / Global CAM tag

```text
8192 physical cell columns / 256 bits per word = 32 words per row
word_column bits = log2(32) = 5
```

```text
[domain:3][bank:2][group:6][subarray:2][row:9][word_column:5]
								  total = 27 bits
```

This 27-bit value is the logical Global CAM reuse tag. It is not the complete
physical cell address. The physical cell column inside a 256-bit word is
represented by the fault mask rather than by `word_column`.

### Byte address

The complete device has `4 GiB = 2^32 bytes`, so a byte address requires
32 bits. A 256-bit word is 32 bytes, hence:

```text
word address: 27 bits + byte offset: 5 bits = 32 bits
```

## 3. Recalculation: `word_bits = 16`

The physical DRAM geometry is unchanged:

- Device capacity: `32 Gbit = 4 GiB`
- Rows / subarray: `512`
- Physical cell columns / row: `8192`
- Physical cell address: **35 bits**

Only the word grouping changes:

```text
8192 physical cell columns / 16 bits per word = 512 words per row
word_column bits = log2(512) = 9
```

The resulting word-level Global CAM tag is:

```text
[domain:3][bank:2][group:6][subarray:2][row:9][word_column:9]
								  total = 31 bits
```

| Address form | Calculation | Length |
|---|---|---:|
| Physical cell address | `3 + 2 + 6 + 2 + 9 + 13` | **35 bits** |
| Local subarray word address | `9 + 9` | **18 bits** |
| Global word-level CAM tag | `3 + 2 + 6 + 2 + 9 + 9` | **31 bits** |
| Byte address | `31 + log2(16/8)` | **32 bits** |

A 16-bit word contains 2 bytes, so its byte offset is 1 bit. Therefore the
device byte-address width remains 32 bits:

```text
word address: 31 bits + byte offset: 1 bit = 32 bits
```

## 4. Summary

| Configuration | Physical cell address | Word-column bits | Global word tag | Byte address |
|---|---:|---:|---:|---:|
| `word_bits = 256` | 35 bits | 5 | 27 bits | 32 bits |
| `word_bits = 16` | 35 bits | 9 | 31 bits | 32 bits |

The key distinction is:

- **35 bits**: full physical cell address of the modeled 32-Gbit device.
- **27 or 31 bits**: Global CAM word-level tag, depending on `word_bits`.
- **32 bits**: byte address over the full 4-GiB device.
