CXX      := g++
CXXFLAGS := -w -g

OBJ_DIR  := bin
TEST_DIR := test

# 通用依赖：分词器
JIEBA_OBJS := $(OBJ_DIR)/splitTool.o $(OBJ_DIR)/splitToolCppJieba.o

ALL: $(TEST_DIR)/createDictionary $(TEST_DIR)/createWebpage $(TEST_DIR)/createInvertedIndex $(TEST_DIR)/client $(TEST_DIR)/server

$(OBJ_DIR)/%.o: src/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

# 模块一：词典创建（离线）
$(TEST_DIR)/createDictionary: $(OBJ_DIR)/createDictionary.o $(OBJ_DIR)/dictProducer.o $(JIEBA_OBJS)
	$(CXX) $^ -o $@ $(CXXFLAGS)

# 模块二：网页库（离线）
$(TEST_DIR)/createWebpage: $(OBJ_DIR)/createWebpage.o $(OBJ_DIR)/webPage.o
	$(CXX) $^ -o $@ $(CXXFLAGS)

# 模块二：倒排索引（离线）
$(TEST_DIR)/createInvertedIndex: $(OBJ_DIR)/createInvertedIndex.o $(OBJ_DIR)/invertedIndex.o $(JIEBA_OBJS)
	$(CXX) $^ -o $@ $(CXXFLAGS)

# 服务端（在线）
$(TEST_DIR)/server: $(OBJ_DIR)/server.o \
  $(OBJ_DIR)/dataCenter.o \
  $(OBJ_DIR)/nonCopyable.o \
  $(OBJ_DIR)/inetAddress.o $(OBJ_DIR)/socket.o \
  $(OBJ_DIR)/acceptor.o $(OBJ_DIR)/socketIO.o $(OBJ_DIR)/tcpConnection.o \
  $(OBJ_DIR)/eventLoop.o $(OBJ_DIR)/tcpServer.o $(OBJ_DIR)/taskQueue.o \
  $(OBJ_DIR)/myTask.o $(OBJ_DIR)/threadPool.o $(OBJ_DIR)/echoServer.o \
  $(OBJ_DIR)/cache.o $(JIEBA_OBJS)
	$(CXX) $^ -o $@ $(CXXFLAGS)

# 客户端
$(TEST_DIR)/client: $(OBJ_DIR)/client.o
	$(CXX) $^ -o $@ $(CXXFLAGS)

.PHONY: clean rebuild
clean:
	$(RM) $(TEST_DIR)/createDictionary $(TEST_DIR)/server $(TEST_DIR)/createWebpage $(TEST_DIR)/createInvertedIndex $(TEST_DIR)/client $(OBJ_DIR)/*.o

rebuild: clean ALL