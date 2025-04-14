# Compiler and Flags
CXX = clang++
CXXFLAGS = -std=c++14 -g \
        -I/opt/homebrew/opt/jsoncpp/include \
        -I/opt/homebrew/opt/openssl/include \
        -I/opt/homebrew/opt/librdkafka/include \
        -I/opt/homebrew/opt/glog/include \
        -I/opt/homebrew/opt/gflags/include \
        -I/opt/homebrew/opt/mysql/include \
        -I./include \
		-I./include/utils

LDFLAGS = -L/opt/homebrew/opt/jsoncpp/lib \
        -L/opt/homebrew/opt/openssl/lib \
        -L/opt/homebrew/opt/librdkafka/lib \
        -L/opt/homebrew/opt/glog/lib \
        -L/opt/homebrew/opt/gflags/lib \
        -L/opt/homebrew/opt/mysql/lib \
        -ljsoncpp -lcurl -lssl -lcrypto -lrdkafka -lglog -lgflags -lmysqlclient -lsqlite3

# 查找所有源文件和头文件
SRCS = $(shell find src -name '*.cpp')

# 生成目标文件路径,保持目录结构
OBJS = $(patsubst src/%.cpp,obj/%.o,$(SRCS))

# 确保构建目录存在
OBJDIRS = $(sort $(dir $(OBJS)))

# Target Executable
TARGET = main

# Build Rules
all: mkdirs $(TARGET)

# 创建所需目录
mkdirs:
	@mkdir -p $(OBJDIRS)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $< to $@"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the application
run: $(TARGET)
	./$(TARGET)

# Clean Up
clean:
	rm -rf obj $(TARGET)