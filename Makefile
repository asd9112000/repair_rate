.DEFAULT_GOAL := all

.PHONY:  all simulators tools clean
.PHONY:  gen_fault fault_generator_b fault_generator_rand_b
.PHONY:  organize_fault organize_fault_b
.PHONY:  basic_pe_b basic_pe_r
.PHONY:  rdr_b rdr_r
.PHONY:  sl_r sl_b
.PHONY:  sl_3way_r sl_3way_b
.PHONY:  sl_sram_r sl_sram_b
.PHONY:  dynamic_sharing_b dynamic_sharing_r
.PHONY:  test test_recam_overflow test_recam_paper_faithful test_remap_validator test_dynamic_spare_sharing_foundation test_dynamic_spare_sharing_policy test_dynamic_repair_rate_sweep test_dynamic_remap_output
.PHONY:  validate_remap_b validate_remap
.PHONY:  analyze_redundantrate analyze_spareline analyze_spareline_sram sim_DynamicSpareSharing



# compiler settings
CXX = g++
CXXFLAGS = -I ./inc -std=c++17 -DPRINTTIME=true -DNDEBUG
OPTFLAGS = -flto -funroll-loops -finline-functions -O3 -ffast-math -march=native
WARNINGS = -g -Wall -Wextra
DEPFLAGS = -MMD -MP

# Build/output directories
BUILD_DIR = build
OBJDIR = $(BUILD_DIR)/obj
APP_OBJDIR = $(OBJDIR)/apps
TEST_OBJDIR = $(OBJDIR)/tests
BINDIR = $(BUILD_DIR)/bin
TESTDIR = $(BUILD_DIR)/tests
REPORTDIR = reports
SRCDIR = src
INCDIR = inc

# Source and object files
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
APP_SRCS = basicPEarray.cpp RedundantRate.cpp SharedLine.cpp SharedLine_3way.cpp \
	SharedLine_SRAM.cpp DynamicSpareSharing.cpp ValidateRemap.cpp
APP_OBJS = $(patsubst %.cpp,$(APP_OBJDIR)/%.o,$(APP_SRCS))
TEST_SRCS = tests/recam_cam_overflow_test.cpp \
	tests/recam_paper_faithful_test.cpp tests/remap_validator_test.cpp \
	tests/dynamic_spare_sharing_foundation_test.cpp \
	tests/dynamic_spare_sharing_policy_test.cpp
TEST_OBJS = $(patsubst tests/%.cpp,$(TEST_OBJDIR)/%.o,$(TEST_SRCS))
DEPS = $(OBJS:.o=.d) $(APP_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

# Executables use no .o suffix. Object files are confined to build/obj.
BASIC_PE_TARGET = $(BINDIR)/basicPEarray
RDR_TARGET = $(BINDIR)/RedundantRate
SL_TARGET = $(BINDIR)/SharedLine
SL_SRAM_TARGET = $(BINDIR)/SharedLine_SRAM
SL_3WAY_TARGET = $(BINDIR)/SharedLine_3way
DYNAMIC_SHARING_TARGET = $(BINDIR)/DynamicSpareSharing
REMAP_VALIDATOR_TARGET = $(BINDIR)/validate_remap
FAULT_GENERATOR_TARGET = $(BINDIR)/fault_generator
FAULT_GENERATOR_RAND_TARGET = $(BINDIR)/fault_generator_rand_amount
FAULT_ORGANIZER_TARGET = $(BINDIR)/fault_organizer

RECAM_OVERFLOW_TEST_TARGET = $(TESTDIR)/recam_cam_overflow_test
RECAM_PAPER_TEST_TARGET = $(TESTDIR)/recam_paper_faithful_test
REMAP_VALIDATOR_TEST_TARGET = $(TESTDIR)/remap_validator_test
DYNAMIC_FOUNDATION_TEST_TARGET = $(TESTDIR)/dynamic_spare_sharing_foundation_test
DYNAMIC_POLICY_TEST_TARGET = $(TESTDIR)/dynamic_spare_sharing_policy_test

clean:
	rm -rf $(BUILD_DIR)

all: simulators tools

simulators: basic_pe_b rdr_b sl_b sl_3way_b sl_sram_b dynamic_sharing_b

tools: validate_remap_b fault_generator_b fault_generator_rand_b organize_fault_b

$(OBJDIR) $(APP_OBJDIR) $(TEST_OBJDIR) $(BINDIR) $(TESTDIR):
	@mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $(DEPFLAGS) -c $< -o $@

$(APP_OBJDIR)/%.o: %.cpp | $(APP_OBJDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $(DEPFLAGS) -c $< -o $@

$(TEST_OBJDIR)/%.o: tests/%.cpp | $(TEST_OBJDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEPS)

$(BASIC_PE_TARGET): $(APP_OBJDIR)/basicPEarray.o $(OBJS) | $(BINDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(RDR_TARGET): $(APP_OBJDIR)/RedundantRate.o $(OBJS) | $(BINDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(SL_TARGET): $(APP_OBJDIR)/SharedLine.o $(OBJS) | $(BINDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(SL_3WAY_TARGET): $(APP_OBJDIR)/SharedLine_3way.o $(OBJS) | $(BINDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(SL_SRAM_TARGET): $(APP_OBJDIR)/SharedLine_SRAM.o $(OBJS) | $(BINDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(DYNAMIC_SHARING_TARGET): $(APP_OBJDIR)/DynamicSpareSharing.o $(OBJS) | $(BINDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(REMAP_VALIDATOR_TARGET): $(APP_OBJDIR)/ValidateRemap.o $(OBJS) | $(BINDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(FAULT_GENERATOR_TARGET): fault_generator/fault_generator.cpp | $(BINDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $< -o $@

$(FAULT_GENERATOR_RAND_TARGET): fault_generator/fault_generator_rand_amount.cpp | $(BINDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $< -o $@

$(FAULT_ORGANIZER_TARGET): fault_generator/fault_organizer.cpp \
	src/FaultOrganizer.cpp inc/FaultOrganizer.hpp inc/Fault.hpp | $(BINDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) \
		fault_generator/fault_organizer.cpp src/FaultOrganizer.cpp -o $@

#===============================================
#
# 					Compile
#
#===============================================

basic_pe_b: $(BASIC_PE_TARGET)

rdr_b: $(RDR_TARGET)

sl_b: $(SL_TARGET)

sl_3way_b: $(SL_3WAY_TARGET)

sl_sram_b: $(SL_SRAM_TARGET)

dynamic_sharing_b: $(DYNAMIC_SHARING_TARGET)

validate_remap_b: $(REMAP_VALIDATOR_TARGET)

fault_generator_b: $(FAULT_GENERATOR_TARGET)

fault_generator_rand_b: $(FAULT_GENERATOR_RAND_TARGET)

organize_fault_b: $(FAULT_ORGANIZER_TARGET)

$(RECAM_OVERFLOW_TEST_TARGET): $(TEST_OBJDIR)/recam_cam_overflow_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_recam_overflow: $(RECAM_OVERFLOW_TEST_TARGET)
	./$(RECAM_OVERFLOW_TEST_TARGET)

$(RECAM_PAPER_TEST_TARGET): $(TEST_OBJDIR)/recam_paper_faithful_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_recam_paper_faithful: $(RECAM_PAPER_TEST_TARGET)
	./$(RECAM_PAPER_TEST_TARGET)

$(REMAP_VALIDATOR_TEST_TARGET): $(TEST_OBJDIR)/remap_validator_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_remap_validator: $(REMAP_VALIDATOR_TEST_TARGET)
	./$(REMAP_VALIDATOR_TEST_TARGET)

$(DYNAMIC_FOUNDATION_TEST_TARGET): $(TEST_OBJDIR)/dynamic_spare_sharing_foundation_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_dynamic_spare_sharing_foundation: $(DYNAMIC_FOUNDATION_TEST_TARGET)
	./$(DYNAMIC_FOUNDATION_TEST_TARGET)

$(DYNAMIC_POLICY_TEST_TARGET): $(TEST_OBJDIR)/dynamic_spare_sharing_policy_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_dynamic_spare_sharing_policy: $(DYNAMIC_POLICY_TEST_TARGET)
	./$(DYNAMIC_POLICY_TEST_TARGET)

test_dynamic_repair_rate_sweep: $(DYNAMIC_SHARING_TARGET)
	python3 tests/dynamic_repair_rate_sweep_test.py ./$(DYNAMIC_SHARING_TARGET)

test_dynamic_remap_output: $(DYNAMIC_SHARING_TARGET) $(REMAP_VALIDATOR_TARGET)
	python3 tests/dynamic_remap_output_test.py \
		./$(DYNAMIC_SHARING_TARGET) ./$(REMAP_VALIDATOR_TARGET)

test: test_recam_overflow test_recam_paper_faithful test_remap_validator test_dynamic_spare_sharing_foundation test_dynamic_spare_sharing_policy test_dynamic_repair_rate_sweep test_dynamic_remap_output

validate_remap: $(REMAP_VALIDATOR_TARGET)
	./$(REMAP_VALIDATOR_TARGET) \
		$(if $(faults),$(faults),./fault_generator/faults.faults) \
		$(if $(remap),$(remap),./reports/SharedLine_SRAM/RemapTable.txt) \
		$(if $(pattern),$(pattern),)

#===============================================
#
# 					Execute
#
#===============================================

gen_fault: $(FAULT_GENERATOR_TARGET)
	./$(FAULT_GENERATOR_TARGET) \
		--fixed_faults $(f) --stack_height $(s) --fault_mode normal \
		--output fault_generator/faults.faults

organize_fault: $(FAULT_ORGANIZER_TARGET)
	./$(FAULT_ORGANIZER_TARGET) \
		fault_generator/faults_simplified.faults \
		fault_generator/faults_organized.faults $(if $(g),$(g),)

basic_pe_r: $(BASIC_PE_TARGET)
	@mkdir -p $(REPORTDIR)/basicPEarray
	./$(BASIC_PE_TARGET) $(s) $(s) > $(REPORTDIR)/basicPEarray/basicPEarray.log

rdr_r: $(RDR_TARGET)
	@mkdir -p $(REPORTDIR)/RedundantRate
	./$(RDR_TARGET) $(s) $(s) $(if $(rptName),--rptName $(rptName)) \
		> $(REPORTDIR)/RedundantRate/RedundantRate.log

sl_r: $(SL_TARGET)
	@mkdir -p $(REPORTDIR)/SharedLine
	./$(SL_TARGET) $(s) $(s) > $(REPORTDIR)/SharedLine/SharedLine.log

sl_3way_r: $(SL_3WAY_TARGET)
	@mkdir -p $(REPORTDIR)/SharedLine_3way
	./$(SL_3WAY_TARGET) $(s) $(s) > $(REPORTDIR)/SharedLine_3way/SharedLine_3way.log

sl_sram_r: $(SL_SRAM_TARGET)
	@mkdir -p $(REPORTDIR)/SharedLine_SRAM
	./$(SL_SRAM_TARGET) $(s) $(s) \
		$(if $(paperCamReuse),--paper-cam-reuse,$(if $(buffer),--buffer $(buffer),)) \
		> $(REPORTDIR)/SharedLine_SRAM/SharedLine_SRAM.log

dynamic_sharing_r: $(DYNAMIC_SHARING_TARGET)
	./$(DYNAMIC_SHARING_TARGET) 2 2 \
		--topology directional --shared-lines 1 --local-first \
		--fault-count 20 --runs 10000 --seed 20260820 \
		--hybrid-cam-entry-width-bits 64 \
		--output-dir reports/dynamic_spare_sharing


remaptable_dynamic_sharing_r: $(DYNAMIC_SHARING_TARGET)
	./$(DYNAMIC_SHARING_TARGET) 2 2 \
	--simplified-fault-file fault_generator/faults_simplified.faults \
	--topology directional \
	--shared-lines 1 \
	--local-first \
	--write-remap-tables \
	--output-dir \
	reports/dynamic_spare_sharing/integration_run


#================================================
# Script targets
# ===============================================
analyze_spareline: $(SL_TARGET) $(FAULT_GENERATOR_TARGET)
	@./scripts/analyze_SpareLine.sh

analyze_spareline_sram:
	@./scripts/analyze_SpareLine_SRAM.sh \
		$(if $(buffer),--buffer $(buffer),) \
		$(if $(paperCamReuse),--paper-cam-reuse,) \
		$(ARGS)

analyze_redundantrate: $(RDR_TARGET) $(FAULT_GENERATOR_TARGET)
	@./scripts/analyze_RedundantRate.sh


sim_DynamicSpareSharing:
	@./scripts/sim_DynamicSpareSharing.sh $(ARGS)

generate_DynamicSpareSharing_table_gallery:
	@./scripts/generate_DynamicSpareSharing_table_gallery.sh
