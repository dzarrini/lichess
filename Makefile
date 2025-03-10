# Compiler to use
CC = g++

# Compiler flags
CCFLAGS = -Wall -Wextra -pedantic -O3

# Target executable name
TARGET = lichess_binary

# Source files
SRC = lichess.cc

# Object files
OBJ = $(SRC:.cpp=.o)

# Default target to compile the program
all: $(TARGET)

# Rule to link the object files and create the executable
$(TARGET): $(OBJ)
	$(CC) $(CCFLAGS) -o $(TARGET) $(OBJ)

# Rule to compile the source files into object files
%.o: %.cc
	$(CC) $(CCFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJ) $(TARGET)

# Run the executable
run: $(TARGET)
	cat sample.txt | ./$(TARGET)

# Phony targets
.PHONY: all clean run
