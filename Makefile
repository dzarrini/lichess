# Compiler to use
CC = gcc

# Compiler flags
CCFLAGS = -Wall -Wextra -pedantic -O3 -g

# Target executable name
TARGET = lichess_binary

# Source files
SRC = lichess.c

# Object files
OBJ = $(SRC:.cpp=.o)

# Default target to compile the program
all: $(TARGET)

# Rule to link the object files and create the executable
$(TARGET): $(OBJ)
	$(CC) $(CCFLAGS) -o $(TARGET) $(OBJ)

# Rule to compile the source files into object files
%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJ) $(TARGET)

# Run the executable
run: $(TARGET)
	time cat sample.txt | ./$(TARGET)

# Phony targets
.PHONY: all clean run
