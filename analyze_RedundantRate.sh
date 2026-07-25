## ===================================================
## This script has been moved to Makefile,
## You can run the Makefile to execute the analysis.
##
## Commend: make analyze_redundant_rate
## ===================================================

# Loop through fault numbers 8 to 14
for fault_num in {8..14}; do
    # Generate faults with the current fault number and stack height 250
    make gen_fault f=$fault_num s=250

    # Loop through s_line values 3 to 5
    for s_line in {3..5}; do
        # Run rdr_r with the current s_line and fault_num, and set rptName accordingly
        make rdr_r s=$s_line rptName="${s_line}_${s_line}_${fault_num}"
    done
done