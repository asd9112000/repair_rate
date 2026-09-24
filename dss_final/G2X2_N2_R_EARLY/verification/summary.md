# Functional verification summary

- Physical analyzer: boundary values 0, 31, 32, 255, 256, 257, 4095, 8191;
  physical columns 1 and 257 do not alias.
- Immediate core oracle: 81 legal paths, 65,536 validity maps and 1,000 random
  vectors; zero prefix-legality mismatches.
- Streaming selected-address mux: committed config-2 output preserves physical
  columns 1 and 257 independently.
