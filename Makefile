.PHONY:  all rdr_b rdr_r gen_fault clean

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
SLE_TARGET = SpareLineEfficiency.o

clean:
	rm -rf $(OBJDIR) $(TARGET) $(RDR_TARGET) $(SLE_TARGET) *.txt
all: $(TARGET)
$(OBJDIR):
	@mkdir $(OBJDIR)

$(TARGET): main.cpp $(OBJS)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) -c $< -o $@


rdr_b: $(RDR_TARGET)
$(OBJDIR):
	@mkdir $(OBJDIR)

$(RDR_TARGET): RedundantRate.cpp $(OBJS)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) -c $< -o $@


sle_b: $(SLE_TARGET)
$(OBJDIR):
	@mkdir $(OBJDIR)
$(SLE_TARGET): SpareLineEfficiency.cpp $(OBJS)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) -c $< -o $@


gen_fault:
	(cd fault_generator && ./fault_generator.o --logic_units 4 --fixed_faults $(f) --stack_height $(s))

rdr_r:
	./RedundantRate.o $(s) $(s) --rptName $(rptName) > RedundantRate.log

sle_r:
	./SpareLineEfficiency.o $(s) $(s) > SpareLineEfficiency.log