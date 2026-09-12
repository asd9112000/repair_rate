#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 REPORT_DIR" >&2
    exit 2
fi

report_dir=$1
area_report="$report_dir/area.rpt"
qor_report="$report_dir/qos.rpt"
reference_report="$report_dir/references.rpt"
for report in "$area_report" "$qor_report" "$reference_report"; do
    [[ -f $report ]] || { echo "missing report: $report" >&2; exit 2; }
done

field_after_colon() {
    awk -F: -v label="$1" '$1 ~ label {gsub(/^[[:space:]]+|[[:space:]]+$/, "", $2); print $2; exit}' "$2"
}

leaf_cells=$(field_after_colon 'Leaf Cell Count' "$qor_report")
combinational_cells=$(field_after_colon 'Combinational Cell Count' "$qor_report")
sequential_cells=$(field_after_colon 'Sequential Cell Count' "$qor_report")
total_cell_area=$(field_after_colon 'Total cell area' "$area_report")
sequential_cell_area=$(field_after_colon 'Noncombinational area' "$area_report")
nand2_area=$(awk '$1 == "NAND2X1" {print $3; exit}' "$reference_report")
nand2_name=NAND2X1
if [[ -z $nand2_area && -f $report_dir/nand2_reference.rpt ]]; then
    nand2_name=$(awk -F= '$1 == "NAND2_REFERENCE_CELL" {print $2; exit}' "$report_dir/nand2_reference.rpt")
    nand2_area=$(awk -F= '$1 == "NAND2_REFERENCE_AREA" {print $2; exit}' "$report_dir/nand2_reference.rpt")
fi
[[ -n $nand2_area ]] || { echo "NAND2X1 reference is absent from synthesis reports" >&2; exit 2; }
gate_equivalents=$(awk -v area="$total_cell_area" -v nand2="$nand2_area" \
    'BEGIN { printf "%.2f", area / nand2 }')

printf '%s\n' \
    "MAPPED_LEAF_CELL_COUNT=$leaf_cells" \
    "COMBINATIONAL_CELL_COUNT=$combinational_cells" \
    "SEQUENTIAL_CELL_COUNT=$sequential_cells" \
    "SEQUENTIAL_CELL_AREA=$sequential_cell_area" \
    "TOTAL_CELL_AREA=$total_cell_area" \
    'GATE_COUNT_BASIS=NAND2X1-equivalent area normalization' \
    "NAND2X1_REFERENCE_CELL=$nand2_name" \
    "NAND2X1_CELL_AREA=$nand2_area" \
    "NAND2X1_GATE_EQUIVALENTS=$gate_equivalents" \
    > "$report_dir/gate_count.rpt"
