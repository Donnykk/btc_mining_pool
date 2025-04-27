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

TARGETS = btc_node task_gen usr_server stratum_server

SRCS = $(wildcard src/*.cpp)
OBJS = $(patsubst src/%.cpp,obj/%.o,$(SRCS))

# 每个可执行文件的主文件
MAIN_SRCS = $(addprefix src/,$(addsuffix .cpp,$(TARGETS)))
MAIN_OBJS = $(patsubst src/%.cpp,obj/%.o,$(MAIN_SRCS))
BIN_TARGETS = $(addprefix bin/,$(TARGETS))
COMMON_SRCS = block_gen.cpp kafka_server.cpp task_validator.cpp tcp_server.cpp
COMMON_OBJS = $(addprefix obj/,$(COMMON_SRCS:.cpp=.o))

all: mkdirs $(BIN_TARGETS)

mkdirs:
	@mkdir -p obj bin

obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

bin/btc_node: obj/btc_node.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

bin/task_gen: obj/task_gen.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

bin/usr_server: obj/usr_server.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

bin/stratum_server: obj/stratum_server.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)


clean:
	rm -rf obj bin

.PHONY: all clean mkdirs