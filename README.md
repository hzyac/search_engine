# Search Engine

基于 C++ 的 Linux 搜索引擎学习项目，提供词语推荐和网页搜索。用户通过终端客户端连接服务端进行查询和文档导入。

构建采用分层 CMake：顶层组织配置、模块和安装；`src/apps/` 放程序入口，业务逻辑在各公共模块中；第三方依赖固定版本并校验 SHA-256。

## 环境

- Linux（使用 epoll、eventfd、文件锁和 POSIX socket）。
- 支持 C++17 的 GCC 或 Clang；CMake 3.24 或更新版本；Make 或 Ninja。
- 首次准备第三方依赖需要访问 GitHub，也可使用已有缓存或依赖安装目录离线构建。

## 编译并使用

在项目根目录执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
build/bin/server
```

server 首次启动时，会使用默认样例语料自动完成词典、网页库和倒排索引生成，然后加载数据并监听连接。后续启动复用已有数据。看到服务准备就绪后，在另一个终端启动客户端：

```bash
build/bin/client
```

输入 `1` 后输入 `search` 或 `火星`，查看候选词；输入 `2` 后输入 `火星`，查询网页；输入 `4` 导入文档文件，输入 `5` 查看导入状态；输入 `3`、`quit` 或 `exit` 退出客户端。服务端通过 Ctrl+C 结束。

程序自动从可执行文件所在的构建目录或安装前缀查找 `search.conf`，启动不依赖当前工作目录。也支持显式指定配置：

```bash
build/bin/server --config /path/to/search.conf
build/bin/client --config /path/to/search.conf
```

客户端可以直接指定服务地址：

```bash
build/bin/client --host 127.0.0.1 --port 8888
```

使用远程服务时，将服务端配置中的 `host` 改为相应的监听地址（例如 `0.0.0.0`），客户端通过 `--host` 指定服务端的实际 IPv4 地址。客户端仅需连接参数，可以单独复制二进制使用。

所有入口支持 `--help`。

## 在线导入与后台建库

先保持 server 运行，在另一个终端导入示例：

```bash
build/bin/client --import examples/documents.json
```

客户端会等待导入完成。看到“文档已可搜索”后，在交互客户端搜索 `木星` 或 `indexing` 即可看到新内容，候选词也会更新，无须重启 server。

也可以在交互菜单选择 `4`，输入本机 JSON 或 JSONL 文件路径。文件由 client 读取，发送的是文档内容；因此 client 与 server 不必共享文件系统。

文档格式：

```json
{
  "id": "article-001",
  "title": "木星探测新进展",
  "url": "https://example.org/jupiter",
  "content": "木星探测任务研究木星的大气和卫星。"
}
```

- `id` 是非空字符串，最多 512 字节，同一 ID 再次导入会完整替换原文档。
- `content` 是非空正文；`title` 和 `url` 可省略。
- 支持单个 JSON 对象、JSON 数组，或每行一个对象的 JSONL。导入文档按 ID 保留，不做近似去重。
- 原有语料初始化的文档以 URL 作为稳定 ID，可以用相同 ID 更新。

客户端自动把文件拆成小于约 60 KB 的请求，每批等待成功后继续。每批构建并发布一个完整版本；后续批次失败时，先前成功的批次仍然保留。单篇文档过大时需要先拆分。

处理过程：接收并记录导入请求 → 后台重建词典和倒排索引 → 校验新索引 → 切换当前版本。后台任务使用独立分词器，查询继续读取原版本；任务成功后，新请求使用新版本。缓存也按版本隔离。

同一数据目录同时处理一个导入任务，忙时返回明确错误。任务状态包括 `queued`、`processing`、`succeeded` 和 `failed`；`succeeded` 表示新文档已经可搜索。进程中保留最近 32 个任务，重启后保留最新成功任务及尚未完成或失败的任务信息。已记录但未完成的导入会在重启后继续处理，失败的导入需要重新提交。

第一版使用后台全量重建，适合小规模、批量导入。当前仍使用原有 TCP 协议，默认监听本机地址，尚未加入访问鉴权。

## 离线准备与存储

首次启动 server 会自动处理原有三类语料并生成初始版本。已经生成的旧格式数据会自动迁移，后续重启直接复用当前版本。

需要刷新原始语料或分词资源时，停止 server，再执行：

```bash
build/bin/prepare_data
# 使用自定义配置：
build/bin/prepare_data --config /path/to/search.conf
```

也可以使用 `cmake --build build --target prepare-data`。离线刷新保留已经在线导入的文档；相同 ID 的在线文档优先于原始语料。

默认数据布局：

```text
build/data/
├── CURRENT                当前版本记录
├── pending-import.json    未完成或失败的导入请求（存在时）
└── versions/
    └── index-XXXXXX/
        ├── documents.jsonl   完整文档集合
        ├── imports.jsonl     在线导入及更新的文档
        ├── seed.dict         原始中英文语料的补充词典
        ├── dict.dat
        ├── dictIndex.dat
        ├── newripepage.dat
        ├── newoffset.dat
        ├── invertIndex.dat
        ├── data.ready
        └── task.json         最新成功任务（在线导入生成时）
```

每次查询持有一套不可变索引，切换后旧版本在最后一个读者退出时清理。新版本构建失败会保留原版本。一个数据目录只允许一个 server 或离线建库进程写入，因此 server 运行期间不能同时执行 `prepare_data`。

## TCP 请求

沿用本机字节序的 4 字节有符号长度头，长度为正文的字节数；正文首字节为请求类型，后面是 UTF-8 JSON。请求正文最大 65535 字节。

| 类型 | JSON 示例 | 返回 |
| --- | --- | --- |
| `1` | `{"query":"木星"}` | 候选词数组 |
| `2` | `{"query":"木星"}` | 网页字符串数组，文档中的 `<id>` 为稳定 ID |
| `3` | `{"documents":[{"id":"1","content":"木星探测"}]}` | 已接收任务及 `task_id` |
| `4` | `{"task_id":123}` 或 `{}` | 指定任务状态，或最新状态 |

响应使用相同长度头，正文为 UTF-8 JSON；错误包含 `error` 字段。导入接收成功不等于建库完成，应检查任务状态或等待客户端导入命令返回。

## 配置与语料

`conf/search.conf.in` 在构建时生成 `build/conf/search.conf`，使用 `examples/corpus/` 中的小型演示语料；`conf/search.conf` 用于安装目录，通过相对路径定位安装后的资源。

配置格式为每行 `键 值`，路径相对于配置文件解析；路径允许空格，整行注释以 `#` 开头。不支持行尾注释或 shell 环境变量展开。

| 配置项 | 含义 |
| --- | --- |
| `jieba_dir` | 分词词典、HMM、IDF、用户词典及分词停用词 |
| `stop_words_eng`、`stop_words_zh` | 项目维护的中英文停用词 |
| `corpus_eng`、`corpus_zh` | 用于词语推荐的英文、中文语料目录 |
| `corpus_web` | 网页语料目录 |
| `data_dir` | 文档、索引版本和导入任务的持久化目录 |
| `host`、`port` | 服务监听或客户端连接地址，默认 `127.0.0.1:8888` |
| `threads`、`queue_size` | 服务端工作线程数及任务队列容量 |

使用真实语料时，复制配置，修改输入目录和 `data_dir`，再启动 server。当前只读取输入目录中直接包含的普通文件。网页输入采用 `<item>` 格式，`<title>`、`<link>`、`<description>` 各占一行，`</item>` 单独一行；当前解析器不是通用 XML 解析器。

仓库保留小型演示语料和 `examples/documents.json` 导入示例，完整语料可在项目外准备后通过 client 导入。第三方依赖已经支持下载；真实语料从 GitHub Release 自动下载的方案尚未接入。

## 第三方依赖

`cmake/Dependencies.cmake` 集中声明 CppJieba、随附 limonp、Simhash 和 JSON 的来源、固定提交号和源码包 SHA-256。头文件、分词词典与模型统一准备到依赖安装目录，下载内容不进入 Git。

默认安装目录是 `build/third-party/install/`，源码包缓存是 `build/third-party/downloads/`，已有相同版本会直接复用。

也可以独立准备依赖：

```bash
cmake -S third-party -B build/deps \
  -DCMAKE_INSTALL_PREFIX="$PWD/build/deps/install"

cmake -S . -B build/offline -DCMAKE_BUILD_TYPE=Debug \
  -DSEARCH_THIRDPARTY_ROOT="$PWD/build/deps/install" \
  -DSEARCH_ALLOW_DOWNLOAD=OFF
cmake --build build/offline --parallel 2
```

当前依赖都是头文件库，独立工程在 CMake 配置阶段完成下载、校验和安装。依赖目录优先级为：显式 `SEARCH_THIRDPARTY_ROOT`、同名环境变量、构建目录内的自动安装。显式目录无效时会报错。

离线机器可复制整个依赖安装目录，或通过 `SEARCH_DEPENDENCY_CACHE` 指定已有源码包缓存。`SEARCH_ALLOW_DOWNLOAD=OFF` 禁止下载，仍允许用缓存解压安装。升级依赖时同时更新提交号和校验值，重新准备依赖、重建数据并验证查询。

## 构建选项

| 选项 | 默认值 | 含义 |
| --- | --- | --- |
| `SEARCH_BUILD_TOOLS` | `ON` | 构建可手动调用的 `prepare_data` 和 `prepare-data` 目标；server 自动准备数据不受此开关影响 |
| `SEARCH_ALLOW_DOWNLOAD` | `ON` | 允许下载缺失的固定版本第三方依赖 |
| `SEARCH_THIRDPARTY_ROOT` | 自动选择 | 已准备好的第三方依赖目录 |
| `SEARCH_DEPENDENCY_CACHE` | 构建目录内 | 依赖源码包缓存目录 |
| `SEARCH_ENABLE_ASAN` | `OFF` | 开启 AddressSanitizer 和 UndefinedBehaviorSanitizer |

## CI 与测试预留

`.github/workflows/` 仅保留目录占位，暂不配置 GitHub Actions。本地日常构建使用 Debug。

`tests/unit/` 和 `tests/tck/` 仅保留目录占位，目前没有测试用例，也未接入 CMake 或 CI。项目构建和运行不依赖 Python，测试入口后续按需添加。

## 安装

```bash
cmake --install build --prefix "$PWD/build/install"
build/install/bin/server
```

另一个终端：

```bash
build/install/bin/client
```

安装后的程序同样自动查找配置、首次建库并复用数据。安装包含程序、词典模型、停用词和演示语料；数据默认写入安装前缀下的 `var/search_engine/`，可通过配置改为其他有写权限的目录。

## 目录

```text
cmake/                    构建配置、依赖版本和资源安装规则
conf/                     配置模板及安装配置
include/search_engine/    项目头文件
src/apps/server/          服务端程序入口
src/apps/client/          终端客户端入口
src/apps/prepare_data/    统一离线建库入口
src/common/               配置、文档模型与公共代码
src/text/                 分词适配
src/concurrency/          线程池、队列与缓存
src/net/                  TCP 和事件循环
src/offline/              完整离线处理流程及各阶段实现
src/search/               查询快照、后台导入、在线检索与请求调度
resources/                项目停用词
examples/                 演示语料与 JSON 导入示例
tests/                    预留测试目录，暂未接入构建
  unit/                   单元测试目录占位
  tck/                    TCK 目录占位
third-party/              独立依赖准备规则
build/                    本地生成内容，不提交 Git
```

项目许可尚未指定，第三方来源与许可记录见 [THIRD_PARTY.md](THIRD_PARTY.md)。
