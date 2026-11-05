# Third-party dependencies

版本和下载包校验值以 `cmake/Dependencies.cmake` 为准。

| 组件 | 上游及固定版本 | 许可记录 |
| --- | --- | --- |
| CppJieba | https://github.com/yanyiwu/cppjieba ，v5.0.3 / `32fd1ef01044940043cf65459e72fca01a3bf0e3` | 上游该版本 README 标注 MIT；同时使用其源码包内的 limonp 和词典模型 |
| limonp | 随上述 CppJieba 提交中的 `deps/limonp` 获取 | 保留源码中现有说明；不单独追踪另一个版本 |
| Simhash | https://github.com/yanyiwu/simhash ，`24625edde9bd86ea0ba4deabd57e905d30b3a894` | 该源码包没有独立 LICENSE 文件，不在此替上游指定许可证；`jenkins.h` 内保留其 Public Domain 说明 |
| JSON for Modern C++ | https://github.com/nlohmann/json ，v3.12.0 / `65ee68451d8eb2b5f3a30b410476ab83deb3289b` | MIT，保留上游 LICENSE 和头文件声明 |

依赖准备流程保留原始头文件声明，并将上游已有 LICENSE、COPYING、README 文件复制至安装目录的 `share/licenses/`。业务项目安装时一并保留这些文件。

Simhash、附带资源和完整语料的再分发许可不能仅凭自动下载推定；公开发布或分发时仍需根据其上游说明确认。完整新闻语料不随当前源码树分发，仓库中的小型演示网页和文本为本次重构新建的样例。

旧的第三方副本、大语料和生成文件已从当前源码目录清理。原项目已有大文件仍可能存在于 Git 历史中，历史记录未重写。
