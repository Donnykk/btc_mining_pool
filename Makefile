# Compiler and Flags
CXX = clang++
CXXFLAGS = -std=c++14 -g \
		-I/opt/homebrew/opt/jsoncpp/include \
		-I/opt/homebrew/opt/openssl/include \
		-I/opt/homebrew/opt/librdkafka/include \
		-I/opt/homebrew/opt/glog/include \
		-I/opt/homebrew/opt/gflags/include \
		-I/opt/homebrew/opt/mysql/include \
		-I./include

LDFLAGS = -L/opt/homebrew/opt/jsoncpp/lib \
		-L/opt/homebrew/opt/openssl/lib \
		-L/opt/homebrew/opt/librdkafka/lib \
		-L/opt/homebrew/opt/glog/lib \
		-L/opt/homebrew/opt/gflags/lib \
		-L/opt/homebrew/opt/mysql/lib \
		-ljsoncpp -lcurl -lssl -lcrypto -lrdkafka -lglog -lgflags -lmysqlclient -lsqlite3

# Source Files
SRCS = $(wildcard src/*.cpp)
# Object Files (replace .cpp with .o)
OBJS = $(SRCS:src/%.cpp=obj/%.o)

# Target Executable
TARGET = main

# Build Rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

obj/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the application
run: $(TARGET)
	./$(TARGET)

# Clean Up
clean:
	rm -f $(OBJS) $(TARGET)