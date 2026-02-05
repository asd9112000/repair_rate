.PHONY:  all fault_generator clean

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
clean:
	rm -rf $(OBJDIR) $(TARGET) *.txt


all: $(TARGET)
$(OBJDIR):
	@mkdir $(OBJDIR)

$(TARGET): main.cpp $(OBJS)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) -c $< -o $@




