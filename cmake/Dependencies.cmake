# 更新依赖时，同时更新固定提交号和源码包 SHA-256。
set(SEARCH_DEPENDENCY_REVISION 1)
set(SEARCH_DEPENDENCIES cppjieba simhash json)

# CppJieba v5.0.3，包含配套的 limonp 和词典模型。
set(SEARCH_cppjieba_REPOSITORY yanyiwu/cppjieba)
set(SEARCH_cppjieba_COMMIT 32fd1ef01044940043cf65459e72fca01a3bf0e3)
set(SEARCH_cppjieba_SHA256 251f333b89582ce2e709c1a45efd4c9d456a6ea41fc038499b9da05809f74b23)

# Simhash
set(SEARCH_simhash_REPOSITORY yanyiwu/simhash)
set(SEARCH_simhash_COMMIT 24625edde9bd86ea0ba4deabd57e905d30b3a894)
set(SEARCH_simhash_SHA256 1f07a788d74cce6fde1d6d5e70d4670ef18980ccecd25b8ca44a28dbc0212860)

# JSON for Modern C++ v3.12.0
set(SEARCH_json_REPOSITORY nlohmann/json)
set(SEARCH_json_COMMIT 65ee68451d8eb2b5f3a30b410476ab83deb3289b)
set(SEARCH_json_SHA256 13ef31d691947940a08909f8e0772f1d7d68e5da1678ee812a49c4bb0c996b2f)

# 依赖安装目录中必须存在的文件，供准备和校验流程共用。
set(SEARCH_DEPENDENCY_REQUIRED_FILES
    include/cppjieba/Jieba.hpp
    include/limonp/Logging.hpp
    include/simhash/Simhasher.hpp
    include/nlohmann/json.hpp
    share/jieba/jieba.dict.utf8
    share/jieba/hmm_model.utf8
    share/jieba/user.dict.utf8
    share/jieba/idf.utf8
    share/jieba/stop_words.utf8
)
