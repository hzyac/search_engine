# 项目自定义分词词典

本目录只维护 `user.dict.utf8` 中的项目自定义词条，应随源码提交。

第三方基础词典、HMM 模型、IDF 和停用词表由 `make` 从 SimHash 自带的 cppjieba 中获取，默认安装到 `/usr/local/share/search-engine/dict/`，不在项目中保存副本。

修改自定义词条后执行 `make`，安装脚本会同步更新已安装的用户词典。重新生成离线数据后再重启服务端。
