# Compiler and Flags
CXX = clang++
CXXFLAGS = -std=c++14 -g \
		-I/opt/homebrew/opt/jsoncpp/include \
		-I/opt/homebrew/opt/openssl/include \
		-I/opt/homebrew/opt/librdkafka/include \
		-I/opt/homebrew/opt/glog/include \
		-I/opt/homebrew/opt/gflags/include 

LDFLAGS = -L/opt/homebrew/opt/jsoncpp/lib \
		-L/opt/homebrew/opt/openssl/lib \
		-L/opt/homebrew/opt/librdkafka/lib \
		-L/opt/homebrew/opt/glog/lib \
		-L/opt/homebrew/opt/gflags/lib \
		-ljsoncpp -lcurl -lssl -lcrypto -lrdkafka -lglog -lgflags

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