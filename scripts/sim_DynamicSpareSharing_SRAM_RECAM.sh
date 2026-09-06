build/bin/DynamicSpareSharing_SRAM_RECAM 2 2 \
  --runs 1000 \
  --fault-count 20 \
  --seed 20260820 \
  --topology directional \
  --shared-lines 1 \
  --local-first \
  --policies serial,chunk2,chunk4,wide \
  --output-dir reports/dynamic_spare_sharing_sram_recam \
  --overwrite