.DEFAULT_GOAL := all

.PHONY:  all simulators tools clean
.PHONY:  gen_fault fault_generator_b fault_generator_rand_b
.PHONY:  organize_fault organize_fault_b
.PHONY:  basic_pe_b basic_pe_r
.PHONY:  rdr_b rdr_r
.PHONY:  sl_r sl_b
.PHONY:  sl_sram_r sl_sram_b
.PHONY:  dynamic_sharing_b dynamic_sharing_r dynamic_sram_recam_b dynamic_sram_recam_r hierarchical_recam_b hierarchical_recam_r
.PHONY:  test test_recam_overflow test_recam_paper_faithful test_remap_validator test_dynamic_spare_sharing_foundation test_dynamic_spare_sharing_policy test_solution_take_policy test_dynamic_spare_sharing_layout test_layout_2x2_regression test_dynamic_repair_rate_sweep test_dynamic_remap_output test_bira_latency test_fault_address_bist test_recam_common_models test_sram_recam_model test_sram_fault_collection_fifo test_sram_recam_dynamic_equivalence test_hierarchical_recam test_hierarchical_fault_models test_canonical_experiment test_canonical_sweep
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
	SharedLine_SRAM.cpp DynamicSpareSharing.cpp \
	DynamicSpareSharing_SRAM_RECAM.cpp HierarchicalRECAM.cpp ValidateRemap.cpp
APP_OBJS = $(patsubst %.cpp,$(APP_OBJDIR)/%.o,$(APP_SRCS))
TEST_SRCS = tests/recam_cam_overflow_test.cpp \
	tests/recam_paper_faithful_test.cpp tests/remap_validator_test.cpp \
	tests/dynamic_spare_sharing_foundation_test.cpp \
	tests/dynamic_spare_sharing_policy_test.cpp \
	tests/solution_take_policy_test.cpp \
	tests/dynamic_spare_sharing_layout_test.cpp \
	tests/bira_latency_test.cpp \
	tests/fault_address_bist_test.cpp \
	tests/recam_common_models_test.cpp \
	tests/sram_recam_model_test.cpp \
	tests/sram_fault_collection_fifo_test.cpp \
	tests/sram_recam_dynamic_equivalence_test.cpp \
	tests/hierarchical_recam_simulator_test.cpp \
	tests/canonical_experiment_test.cpp
TEST_OBJS = $(patsubst tests/%.cpp,$(TEST_OBJDIR)/%.o,$(TEST_SRCS))
DEPS = $(OBJS:.o=.d) $(APP_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

# Executables use no .o suffix. Object files are confined to build/obj.
BASIC_PE_TARGET = $(BINDIR)/basicPEarray
RDR_TARGET = $(BINDIR)/RedundantRate
SL_TARGET = $(BINDIR)/SharedLine
SL_SRAM_TARGET = $(BINDIR)/SharedLine_SRAM
SL_3WAY_TARGET = $(BINDIR)/SharedLine_3way
DYNAMIC_SHARING_TARGET = $(BINDIR)/DynamicSpareSharing
DYNAMIC_SRAM_RECAM_TARGET = $(BINDIR)/DynamicSpareSharing_SRAM_RECAM
HIERARCHICAL_RECAM_TARGET = $(BINDIR)/HierarchicalRECAM
REMAP_VALIDATOR_TARGET = $(BINDIR)/validate_remap
FAULT_GENERATOR_TARGET = $(BINDIR)/fault_generator
FAULT_GENERATOR_RAND_TARGET = $(BINDIR)/fault_generator_rand_amount
FAULT_ORGANIZER_TARGET = $(BINDIR)/fault_organizer

RECAM_OVERFLOW_TEST_TARGET = $(TESTDIR)/recam_cam_overflow_test
RECAM_PAPER_TEST_TARGET = $(TESTDIR)/recam_paper_faithful_test
REMAP_VALIDATOR_TEST_TARGET = $(TESTDIR)/remap_validator_test
DYNAMIC_FOUNDATION_TEST_TARGET = $(TESTDIR)/dynamic_spare_sharing_foundation_test
DYNAMIC_POLICY_TEST_TARGET = $(TESTDIR)/dynamic_spare_sharing_policy_test
SOLUTION_TAKE_TEST_TARGET = $(TESTDIR)/solution_take_policy_test
DYNAMIC_LAYOUT_TEST_TARGET = $(TESTDIR)/dynamic_spare_sharing_layout_test
BIRA_LATENCY_TEST_TARGET = $(TESTDIR)/bira_latency_test
FAULT_ADDRESS_BIST_TEST_TARGET = $(TESTDIR)/fault_address_bist_test
RECAM_COMMON_MODELS_TEST_TARGET = $(TESTDIR)/recam_common_models_test
SRAM_RECAM_MODEL_TEST_TARGET = $(TESTDIR)/sram_recam_model_test
SRAM_FAULT_FIFO_TEST_TARGET = $(TESTDIR)/sram_fault_collection_fifo_test
SRAM_RECAM_DYNAMIC_TEST_TARGET = $(TESTDIR)/sram_recam_dynamic_equivalence_test
HIERARCHICAL_RECAM_TEST_TARGET = $(TESTDIR)/hierarchical_recam_simulator_test
CANONICAL_EXPERIMENT_TEST_TARGET = $(TESTDIR)/canonical_experiment_test

clean:
	rm -rf $(BUILD_DIR)

all: simulators tools

simulators: basic_pe_b rdr_b sl_b sl_sram_b dynamic_sharing_b dynamic_sram_recam_b hierarchical_recam_b

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

$(DYNAMIC_SRAM_RECAM_TARGET): $(APP_OBJDIR)/DynamicSpareSharing_SRAM_RECAM.o $(OBJS) | $(BINDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(HIERARCHICAL_RECAM_TARGET): $(APP_OBJDIR)/HierarchicalRECAM.o $(OBJS) | $(BINDIR)
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

sl_sram_b: $(SL_SRAM_TARGET)

dynamic_sharing_b: $(DYNAMIC_SHARING_TARGET)

dynamic_sram_recam_b: $(DYNAMIC_SRAM_RECAM_TARGET)

hierarchical_recam_b: $(HIERARCHICAL_RECAM_TARGET)

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

$(SOLUTION_TAKE_TEST_TARGET): $(TEST_OBJDIR)/solution_take_policy_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_solution_take_policy: $(SOLUTION_TAKE_TEST_TARGET)
	./$(SOLUTION_TAKE_TEST_TARGET)

$(DYNAMIC_LAYOUT_TEST_TARGET): $(TEST_OBJDIR)/dynamic_spare_sharing_layout_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_dynamic_spare_sharing_layout: $(DYNAMIC_LAYOUT_TEST_TARGET)
	./$(DYNAMIC_LAYOUT_TEST_TARGET)

test_layout_2x2_regression: test_dynamic_spare_sharing_policy

$(BIRA_LATENCY_TEST_TARGET): $(TEST_OBJDIR)/bira_latency_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_bira_latency: $(BIRA_LATENCY_TEST_TARGET)
	./$(BIRA_LATENCY_TEST_TARGET)

$(FAULT_ADDRESS_BIST_TEST_TARGET): $(TEST_OBJDIR)/fault_address_bist_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_fault_address_bist: $(FAULT_ADDRESS_BIST_TEST_TARGET)
	./$(FAULT_ADDRESS_BIST_TEST_TARGET)

$(RECAM_COMMON_MODELS_TEST_TARGET): $(TEST_OBJDIR)/recam_common_models_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_recam_common_models: $(RECAM_COMMON_MODELS_TEST_TARGET)
	./$(RECAM_COMMON_MODELS_TEST_TARGET)

$(SRAM_RECAM_MODEL_TEST_TARGET): $(TEST_OBJDIR)/sram_recam_model_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_sram_recam_model: $(SRAM_RECAM_MODEL_TEST_TARGET)
	./$(SRAM_RECAM_MODEL_TEST_TARGET)

$(SRAM_FAULT_FIFO_TEST_TARGET): $(TEST_OBJDIR)/sram_fault_collection_fifo_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_sram_fault_collection_fifo: $(SRAM_FAULT_FIFO_TEST_TARGET)
	./$(SRAM_FAULT_FIFO_TEST_TARGET) \
		tests/golden/sram_fault_collection_fifo_expected.csv \
		$(TESTDIR)/sram_fault_collection_fifo_actual.csv

$(SRAM_RECAM_DYNAMIC_TEST_TARGET): $(TEST_OBJDIR)/sram_recam_dynamic_equivalence_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_sram_recam_dynamic_equivalence: $(SRAM_RECAM_DYNAMIC_TEST_TARGET)
	./$(SRAM_RECAM_DYNAMIC_TEST_TARGET)

$(HIERARCHICAL_RECAM_TEST_TARGET): $(TEST_OBJDIR)/hierarchical_recam_simulator_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_hierarchical_recam: $(HIERARCHICAL_RECAM_TEST_TARGET)
	./$(HIERARCHICAL_RECAM_TEST_TARGET)

$(CANONICAL_EXPERIMENT_TEST_TARGET): $(TEST_OBJDIR)/canonical_experiment_test.o $(OBJS) | $(TESTDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $^ -o $@

test_canonical_experiment: $(CANONICAL_EXPERIMENT_TEST_TARGET)
	./$(CANONICAL_EXPERIMENT_TEST_TARGET) \
		tests/golden/canonical_b0_expected.csv \
		$(TESTDIR)/canonical_b0_actual.csv \
		$(TESTDIR)/canonical_four_actual

test_canonical_sweep: $(HIERARCHICAL_RECAM_TARGET)
	python3 tests/canonical_sweep_test.py \
		./$(HIERARCHICAL_RECAM_TARGET) \
		./scripts/run_canonical_four_sweep.py

test_hierarchical_fault_models: $(HIERARCHICAL_RECAM_TARGET)
	python3 tests/hierarchical_fault_model_test.py \
		./$(HIERARCHICAL_RECAM_TARGET)

test_dynamic_repair_rate_sweep: $(DYNAMIC_SHARING_TARGET)
	python3 tests/dynamic_repair_rate_sweep_test.py ./$(DYNAMIC_SHARING_TARGET)

test_dynamic_remap_output: $(DYNAMIC_SHARING_TARGET) $(REMAP_VALIDATOR_TARGET)
	python3 tests/dynamic_remap_output_test.py \
		./$(DYNAMIC_SHARING_TARGET) ./$(REMAP_VALIDATOR_TARGET)

test: test_recam_overflow test_recam_paper_faithful test_remap_validator test_dynamic_spare_sharing_foundation test_dynamic_spare_sharing_policy test_solution_take_policy test_dynamic_spare_sharing_layout test_bira_latency test_fault_address_bist test_recam_common_models test_sram_recam_model test_sram_fault_collection_fifo test_sram_recam_dynamic_equivalence test_hierarchical_recam test_hierarchical_fault_models test_canonical_experiment test_canonical_sweep test_dynamic_repair_rate_sweep test_dynamic_remap_output

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

dynamic_sram_recam_r: $(DYNAMIC_SRAM_RECAM_TARGET)
	./$(DYNAMIC_SRAM_RECAM_TARGET) 2 2 \
		--topology directional --shared-lines 1 --local-first \
		--fault-count 20 --runs 100 --seed 20260820 \
		--policies serial,chunk2,chunk4,wide \
		--output-dir reports/dynamic_sram_recam

hierarchical_recam_r: $(HIERARCHICAL_RECAM_TARGET)
	./$(HIERARCHICAL_RECAM_TARGET) 2 2 \
		--groups 32 --fault-count 12 --topology edge \
		--shared-rows 1 --shared-columns 0 --local-first \
		--max-borrows 1 \
		--bira-engines 1 \
		--data-word-bits 16 \
		--cam-granularity word \
		--output-dir reports/hierarchical_recam


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


sim_DynamicSpareSharing: $(DYNAMIC_SHARING_TARGET)
	@./scripts/sim_DynamicSpareSharing.sh $(ARGS)

generate_DynamicSpareSharing_table_gallery:
	@./scripts/generate_DynamicSpareSharing_table_gallery.sh

sim_DynamicSpareSharing_SRAM_RECAM: $(DYNAMIC_SRAM_RECAM_TARGET)
	@./scripts/sim_DynamicSpareSharing_SRAM_RECAM.sh $(ARGS)
