# Compiler and Flags
CXX = clang++
CXXFLAGS = -std=c++14 -g -I/opt/homebrew/opt/jsoncpp/include -I/opt/homebrew/opt/openssl/include
LDFLAGS = -L/opt/homebrew/opt/jsoncpp/lib -L/opt/homebrew/opt/openssl/lib -ljsoncpp -lcurl -lssl -lcrypto

# Source Files
SRCS = $(wildcard *.cpp)
# Object Files (replace .cpp with .o)
OBJS = $(SRCS:.cpp=.o)

# Target Executable
TARGET = test

# Build Rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the application
run: $(TARGET)
	./$(TARGET)

# Clean Up
clean:
	rm -f $(OBJS) $(TARGET)
