CXX      := g++
CXXFLAGS ?= -g -std=c++20
LDLIBS   += -pthread

PREFIX   ?= /usr/local
OBJ_DIR  ?= bin
TEST_DIR ?= test
DEPS_MANIFEST := $(PREFIX)/share/search-engine/deps/versions.env
HEADERS := $(wildcard include/*.hpp)

# /usr/local/include 和 /usr/include 已在 g++ 默认搜索路径中。
DEP_CPPFLAGS := -DSEARCH_ENGINE_DEFAULT_DICT_DIR='"$(PREFIX)/share/search-engine/dict"'
ifeq ($(filter /usr /usr/local,$(PREFIX)),)
DEP_CPPFLAGS += -I$(PREFIX)/include
endif

.DEFAULT_GOAL := build

# 通用依赖：分词器
JIEBA_OBJS := $(OBJ_DIR)/splitTool.o $(OBJ_DIR)/splitToolCppJieba.o
PROGRAMS := $(addprefix $(TEST_DIR)/,createDictionary createWebpage createInvertedIndex client server)

ALL: $(PROGRAMS) | check-deps data

$(PROGRAMS): | $(TEST_DIR)

$(OBJ_DIR)/%.o: src/%.cpp Makefile deps/versions.env $(HEADERS) $(DEPS_MANIFEST) | $(OBJ_DIR) check-deps
	$(CXX) $(CPPFLAGS) $(DEP_CPPFLAGS) $(CXXFLAGS) -pthread -MMD -MP -c $< -o $@

$(OBJ_DIR) $(TEST_DIR) data:
	mkdir -p $@

# 默认入口：依赖缺失或版本变化时先安装，再编译全部程序。
build: install-deps
	+$(MAKE) ALL

deps:
	bash scripts/deps.sh fetch "$(PREFIX)"

install-deps:
	bash scripts/deps.sh install "$(PREFIX)"

check-deps:
	@bash scripts/deps.sh check "$(PREFIX)"

$(DEPS_MANIFEST):
	@bash scripts/deps.sh check "$(PREFIX)"

# 模块一：词典创建（离线）
$(TEST_DIR)/createDictionary: $(OBJ_DIR)/createDictionary.o $(OBJ_DIR)/dictProducer.o $(JIEBA_OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# 模块二：网页库（离线）
$(TEST_DIR)/createWebpage: $(OBJ_DIR)/createWebpage.o $(OBJ_DIR)/webPage.o
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# 模块二：倒排索引（离线）
$(TEST_DIR)/createInvertedIndex: $(OBJ_DIR)/createInvertedIndex.o $(OBJ_DIR)/invertedIndex.o $(JIEBA_OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# 服务端（在线）
$(TEST_DIR)/server: $(OBJ_DIR)/server.o \
  $(OBJ_DIR)/dataCenter.o \
  $(OBJ_DIR)/nonCopyable.o \
  $(OBJ_DIR)/inetAddress.o $(OBJ_DIR)/socket.o \
  $(OBJ_DIR)/acceptor.o $(OBJ_DIR)/socketIO.o $(OBJ_DIR)/tcpConnection.o \
  $(OBJ_DIR)/eventLoop.o $(OBJ_DIR)/tcpServer.o $(OBJ_DIR)/taskQueue.o \
  $(OBJ_DIR)/myTask.o $(OBJ_DIR)/threadPool.o $(OBJ_DIR)/echoServer.o \
  $(OBJ_DIR)/cache.o $(JIEBA_OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# 客户端
$(TEST_DIR)/client: $(OBJ_DIR)/client.o
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(TEST_DIR)/dependencies_test: test/dependencies_test.cpp $(OBJ_DIR)/splitTool.o $(OBJ_DIR)/splitToolCppJieba.o $(HEADERS) Makefile $(DEPS_MANIFEST) | $(TEST_DIR) check-deps
	$(CXX) $(CPPFLAGS) $(DEP_CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $< $(OBJ_DIR)/splitTool.o $(OBJ_DIR)/splitToolCppJieba.o -o $@ $(LDLIBS)

$(TEST_DIR)/cppjieba_test: test/cppjieba_test.cpp include/dependencyPaths.hpp Makefile $(DEPS_MANIFEST) | $(TEST_DIR) check-deps
	$(CXX) $(CPPFLAGS) $(DEP_CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $< -o $@ $(LDLIBS)

$(TEST_DIR)/test_simhash: test/test_simhash.cpp include/dependencyPaths.hpp Makefile $(DEPS_MANIFEST) | $(TEST_DIR) check-deps
	$(CXX) $(CPPFLAGS) $(DEP_CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $< -o $@ $(LDLIBS)

check: $(TEST_DIR)/dependencies_test $(TEST_DIR)/cppjieba_test $(TEST_DIR)/test_simhash
	$(TEST_DIR)/dependencies_test

.PHONY: ALL build deps install-deps check-deps check clean rebuild
clean:
	$(RM) $(TEST_DIR)/createDictionary $(TEST_DIR)/server $(TEST_DIR)/createWebpage $(TEST_DIR)/createInvertedIndex $(TEST_DIR)/client $(TEST_DIR)/dependencies_test $(TEST_DIR)/cppjieba_test $(TEST_DIR)/test_simhash $(OBJ_DIR)/*.o $(OBJ_DIR)/*.d

rebuild:
	+$(MAKE) clean
	+$(MAKE) build

-include $(wildcard $(OBJ_DIR)/*.d)
