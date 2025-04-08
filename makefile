# Makefile for the sidecar_launcher program with system-installed json-c on CentOS

# Compiler to use
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Linker flags
LDFLAGS = -ljson-c -lm

# Target executable name
TARGET = sidecar_launcher

# Project source files
SOURCES = sidecar_launcher.c
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Link object files with system json-c library
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile project source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJECTS) $(TARGET)

# Phony targets
.PHONY: all clean
