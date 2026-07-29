
.PHONY:  all clean
.PHONY:  gen_fault
.PHONY:  rdr_b rdr_r
.PHONY:  sl_r sl_b
.PHONY:  sl_3way_r sl_3way_b
.PHONY:  sl_sram_r sl_sram_b
.PHONY:  analyze_redundantrate analyze_spareline analyze_spare_line_3way



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

# TARGET = main.o
RDR_TARGET = RedundantRate.o
SL_TARGET = SharedLine.o
SL_SRAM_TARGET = SharedLine_SRAM.o
SL_3WAY_TARGET = SharedLine_3way.o

clean:
	rm -rf $(OBJDIR) $(TARGET) $(RDR_TARGET) $(SL_TARGET) *.txt *.log *.o
# all: $(TARGET)


$(OBJDIR):
	@mkdir -p $(OBJDIR)
	@mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) -c $< -o $@

# $(TARGET): main.cpp $(OBJS)
# 	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

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
	(cd fault_generator && ./fault_generator.o  --fixed_faults $(f) --stack_height $(s) --fault_mode normal)

organize_fault:
	g++ -I ./inc -std=c++17 -Wall -Wextra fault_generator/fault_organizer.cpp src/FaultOrganizer.cpp -o fault_generator/fault_organizer.o
	./fault_generator/fault_organizer.o fault_generator/faults_simplified.faults fault_generator/faults_organized.faults $(if $(g),$(g),)

rdr_r:
	./RedundantRate.o $(s) $(s) $(if $(rptName),--rptName $(rptName)) > RedundantRate.log
	./RedundantRate.o $(s) $(s) $(if $(rptName),--rptName $(rptName)) > RedundantRate.log

sl_r:
	./SharedLine.o $(s) $(s) > SharedLine.log

sl_3way_r:
	./SharedLine_3way.o $(s) $(s) > SharedLine_3way.log

sl_sram_r:
	./SharedLine_SRAM.o $(s) $(s) > SharedLine_SRAM.log


#================================================
# Script targets
# ===============================================
analyze_spareline:
	@./scripts/analyze_SpareLine.sh

analyze_redundantrate:
	@./scripts/analyze_RedundantRate.sh
