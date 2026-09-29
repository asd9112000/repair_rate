# Exact-corpus identity

- Vectors: `10000`
- Seed: `0x20260928`
- Generator: deterministic 32-bit xorshift, in this exact order:
  `x ^= x << 13; x ^= x >> 17; x ^= x << 5;`
- Corpus changed: `NO`
- New seed: `NO`
- Synthetic latency stimulus: `NO`

The full-top replay wrappers map the generated 32-bit word into four
per-SA eight-bit source words. The canonical and CA-LIVE instances receive
the same generated corpus during each replay.
