# Makefile for Monte Carlo π Estimator

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3
LDFLAGS = -lm

# Target executable
TARGET = monte_carlo_pi

# Source files
SOURCES = monte_carlo_pi.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Run with default settings
run: $(TARGET)
	./$(TARGET)

# Run with progress display
run-progress: $(TARGET)
	./$(TARGET) -p

# Run with custom sample count
run-samples: $(TARGET)
	./$(TARGET) -n 10000000 -p

# Generate CSV with intermediate results
run-csv: $(TARGET)
	./$(TARGET) -n 1000000 -s results.csv -i

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build the executable"
	@echo "  clean        - Remove build files"
	@echo "  run          - Run with default settings (1M samples)"
	@echo "  run-progress - Run with progress display"
	@echo "  run-samples  - Run with 10M samples and progress"
	@echo "  run-csv      - Generate CSV with intermediate results"
	@echo "  help         - Show this help message"

.PHONY: all clean run run-progress run-samples run-csv help 