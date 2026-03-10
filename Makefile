.PHONY:  all rdr_b rdr_r sl_r sl_b gen_fault clean

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

clean:
	rm -rf $(OBJDIR) $(TARGET) $(RDR_TARGET) $(SL_TARGET) *.txt
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


sl_b: $(SL_TARGET)
$(OBJDIR):
	@mkdir $(OBJDIR)
$(SL_TARGET): SharedLine.cpp $(OBJS)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) -c $< -o $@


gen_fault:
	(cd fault_generator && ./fault_generator.o --logic_units 4 --fixed_faults $(f) --stack_height $(s))

rdr_r:
	./RedundantRate.o $(s) $(s)  > RedundantRate.log
# 	./RedundantRate.o $(s) $(s) --rptName $(rptName) > RedundantRate.log

sl_r:
	./SharedLine.o $(s) $(s) > SharedLine.log