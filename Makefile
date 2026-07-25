.PHONY:  all clean
.PHONY:  gen_fault
.PHONY:  rdr_b rdr_r
.PHONY:  sl_r sl_b
.PHONY:  sl_3way_r sl_3way_b
.PHONY:  sl_sram_r sl_sram_b
.PHONY: analyze_redundant_rate analyze_spare_line analyze_spare_line_3way
# compiler settings
CXX = g++
CXXFLAGS = -I ./inc -std=c++17 -DPRINTTIME=true -DNDEBUG
OPTFLAGS = -flto -funroll-loops -finline-functions -O3 -ffast-math -march=native
WARNINGS = -g -Wall -Wextra

# Directories
SRCDIR = src
OBJDIR = obj
INCDIR = inc

# Source and object files
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

TARGET = main.o
RDR_TARGET = RedundantRate.o
SL_TARGET = SharedLine.o
SL_SRAM_TARGET = SharedLine_SRAM.o

all: $(TARGET)

clean:
	rm -rf $(OBJDIR) $(TARGET) $(RDR_TARGET) $(SL_TARGET) $(SL_SRAM_TARGET) *.txt SharedLine.log SharedLineReport

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) -c $< -o $@

$(TARGET): main.cpp $(OBJS)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(RDR_TARGET): RedundantRate.cpp $(OBJS)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(SL_TARGET): SharedLine.cpp $(OBJS)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(SL_3WAY_TARGET): SharedLine_3way.cpp $(OBJS)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(SL_SRAM_TARGET): SharedLine_SRAM.cpp $(OBJS)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

#===============================================
#
# 					Compile
#
#===============================================

rdr_b: $(RDR_TARGET)

sl_b: $(SL_TARGET)

sl_3way_b: $(SL_3WAY_TARGET)

sl_sram_b: $(SL_SRAM_TARGET)

#===============================================
#
# 					Execute
#
#===============================================

gen_fault:
	(cd fault_generator && ./fault_generator.o --logic_units 4 --fixed_faults $(f) --stack_height $(s))

rdr_r:
	./RedundantRate.o $(s) $(s) $(if $(rptName),--rptName $(rptName)) > RedundantRate.log

sl_r:
	./SharedLine.o $(s) $(s) > SharedLine.log

sl_3way_r:
	./SharedLine_3way.o $(s) $(s) > SharedLine_3way.log

sl_sram_r:
	./SharedLine_SRAM.o $(s) $(s) > SharedLine_SRAM.log

#===============================================
#
# Analyze by fault number and spare line
#
#===============================================

analyze_redundant_rate:
	@set -e; \
	for fault_num in $$(seq 8 14); do \
		$(MAKE) gen_fault f=$$fault_num s=250; \
		for s_line in $$(seq 3 5); do \
			$(MAKE) rdr_r s=$$s_line rptName="$${s_line}_$${s_line}_$${fault_num}"; \
		done; \
	done

analyze_spare_line:
	@set -e; \
	rm -f SharedLine.log; \
	rm -f ./SharedLineReport/repairRates.csv; \
	mkdir -p ./SharedLineReport; \
	for fault_num in $$(seq 8 16); do \
		$(MAKE) gen_fault f=$$fault_num s=250; \
		for s_line in $$(seq 2 6); do \
			./SharedLine.o $$s_line $$s_line --rptName "SL_RepairReport_r$${s_line}_c$${s_line}_f$${fault_num}.rpt" --faultNum $$fault_num >> SharedLine.log 2>&1; \
		done; \
	done; \
	echo "FaultNum,SpareLine,RepairRate,RepairRate_RECAM" >> ./SharedLineReport/repairRates.csv; \
	grep "RepairRate:" SharedLine.log | while read -r line; do \
		fault_num=$$(echo "$$line" | grep -oP 'faultNum: \K\d+'); \
		spare_line=$$(echo "$$line" | grep -oP 'SpareLine: \K\d+'); \
		repair_rate=$$(echo "$$line" | grep -oP 'RepairRate: \K[0-9.]+'); \
		repair_rate_recam=$$(echo "$$line" | grep -oP 'RepairRate_RECAM: \K[0-9.]+'); \
		echo "$$fault_num,$$spare_line,$$repair_rate,$$repair_rate_recam" >> ./SharedLineReport/repairRates.csv; \
	done


# analyze_spare_line_3way:
# 	@set -e; \
# 	rm -f SharedLine_3wayReport.log; \
# 	rm -f ./SharedLine_3wayReport/repairRates.csv; \
# 	mkdir -p ./SharedLine_3wayReport; \
# 	for fault_num in $$(seq 8 16); do \
# 		$(MAKE) gen_fault f=$$fault_num s=250; \
# 		for s_line in $$(seq 2 6); do \
# 			./SharedLine_3way.o $$s_line $$s_line --rptName "SL_RepairReport_r$${s_line}_c$${s_line}_f$${fault_num}.rpt" --faultNum $$fault_num >> SharedLine_3wayReport.log 2>&1; \
# 		done; \
# 	done; \
# 	echo "FaultNum,SpareLine,RepairRate,RepairRate_RECAM" >> ./SharedLine_3wayReport/repairRates.csv; \
# 	grep "RepairRate:" SharedLine_3wayReport.log | while read -r line; do \
# 		fault_num=$$(echo "$$line" | grep -oP 'faultNum: \K\d+'); \
# 		spare_line=$$(echo "$$line" | grep -oP 'SpareLine: \K\d+'); \
# 		repair_rate=$$(echo "$$line" | grep -oP 'RepairRate: \K[0-9.]+'); \
# 		repair_rate_recam=$$(echo "$$line" | grep -oP 'RepairRate_RECAM: \K[0-9.]+'); \
# 		echo "$$fault_num,$$spare_line,$$repair_rate,$$repair_rate_recam" >> ./SharedLine_3wayReport/repairRates.csv; \
# 	done