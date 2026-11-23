# HyperGit

HyperGit 是一个使用 C++20 编写的 Git 风格版本控制系统学习项目。

本项目以 Linus Torvalds 于 2005 年编写的早期 Git 实现为参考，学习版本控制系统中的对象存储、内容寻址、索引、提交历史和文件格式设计。

HyperGit 不以兼容 Git 为目标，而是希望在理解 Git 核心思想的基础上，使用现代 C++ 和跨平台设计重新实现一个类似 Git 的系统。

当前项目已经完成了跨平台 cache/index 文件的基础设计，包括：

- 二进制数据的读写；
- 大端序数据格式；
- SHA-256 摘要计算；
- `CacheHeader`、`CacheEntry` 和 `CacheIndex`；
- cache 文件的序列化和反序列化；
- cache 文件 checksum 校验；
- 基于 `std::filesystem::path` 的跨平台路径表示；
- 文件读写和相关测试。

项目仍处于早期开发阶段，后续将继续实现对象库、工作区管理、提交、分支和历史遍历等功能。

## Quick Start

### 环境要求

- C++20 编译器；
- CMake 3.20 或更高版本；
- OpenSSL；
- GoogleTest。

### 构建项目

```bash
cmake -S . -B build
cmake --build build
```

### 运行测试

```bash
ctest --test-dir build --output-on-failure
```

当前版本主要用于验证底层数据结构、二进制格式、SHA-256 和 cache 文件持久化功能。

随着项目继续开发，HyperGit 将逐步提供类似以下的命令行使用方式：

```bash
hypergit init
hypergit add <path>
hypergit commit
hypergit log
```

具体命令会随着对象库和命令行接口的实现逐步确定。

## 与 Git 的差异

HyperGit 借鉴 Git 的核心思想，但不追求与 Git 的命令、文件格式或仓库完全兼容。

| 对比项           | Git                                        | HyperGit                                                       |
|------------------|--------------------------------------------|----------------------------------------------------------------|
| 项目目标         | 面向实际使用的成熟版本控制系统             | 面向学习和设计实践的 Git 风格版本控制系统                      |
| 实现语言         | C                                          | C++20                                                          |
| 平台接口         | 大量使用 POSIX 和平台相关接口              | 尽量使用 C++ 标准库和跨平台接口                                |
| 哈希算法         | 早期设计使用 SHA-1，现代 Git 支持 SHA-256  | 默认设计使用 SHA-256                                           |
| cache/index 格式 | 使用 Git 自己的格式                        | 使用自定义的跨平台格式                                         |
| 整数编码         | 采用 Git 规定的格式                        | 统一使用大端序                                                 |
| 路径表示         | 受 Git 和平台实现影响                      | 内存中使用 `std::filesystem::path`，磁盘中保存规范化的相对路径 |
| 文件元数据       | 包含较多 Unix/POSIX 风格元数据             | 保留修改时间、文件大小和对象 ID，避免依赖 POSIX `stat` 字段    |
| cache 校验       | 使用校验摘要验证 cache 内容                | 使用 SHA-256 校验 cache 文件内容                               |
| 仓库兼容性       | 支持 Git 仓库和 Git 工具链                 | 不保证读取或写入现有 Git 仓库                                  |
| 当前功能         | 已具备完整的版本控制能力                   | 当前主要完成 cache/index 基础设施                              |
| 后续功能         | 已支持对象库、提交、分支、远程和合并等功能 | 计划逐步实现对象库、tree、commit、branch、remote 和 merge      |

HyperGit 的实现会优先关注设计理解、数据结构、文件格式、错误处理和可测试性，并在此基础上逐步发展为一个更加现代化的 Git 风格版本控制系统。
