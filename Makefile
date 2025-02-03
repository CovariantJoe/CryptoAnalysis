# Compiler
CXX = g++

# Compiler Flags
CXXFLAGS = -Wall -Wextra -std=c++17

# Linker Flags
LDFLAGS = -lsqlite3 -lcurl

# Sources and Objects
SRC = program.cpp database.cpp SendAlerts.cpp 
OBJ = $(SRC:.cpp=.o)

# Output Binary
TARGET = CryptoAnalysis

# Default Target
all: $(TARGET)

# Link objects to create the final binary
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

# Compile individual .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJ) $(TARGET)

# Rebuild everything
rebuild: clean all

