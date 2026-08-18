# CMake 学习与项目使用指南

> 本文面向正在开发 Qt/C++ IM 项目的开发者。内容先解释 CMake 的作用，再结合当前项目的真实配置、Qt6 编译、运行时部署和常见错误进行说明。

## 1. CMake 是什么

CMake 是一个跨平台的构建系统生成工具。它通常不直接编译 C++，而是读取项目中的 `CMakeLists.txt`，根据当前平台和工具链生成 Ninja、Makefile 或 Visual Studio 工程，再由这些工具调用编译器完成构建。

完整流程：

```text
CMakeLists.txt
      ↓ cmake 配置
检查编译器、Qt 和其他依赖
      ↓ 生成
Ninja / Makefile / Visual Studio 工程
      ↓ cmake --build
编译、链接、生成 exe 或库
```

| 工具 | 作用 |
| --- | --- |
| CMake | 读取构建规则并生成构建文件 |
| Ninja | 根据 `build.ninja` 执行快速增量编译 |
| Visual Studio | 可以作为编译器、构建工具和 IDE |
| MinGW GCC | Windows 上的 C/C++ 编译器工具链 |
| Qt | 提供 GUI、网络、数据库等 C++ 库 |

当前项目使用 Qt6、MinGW、Ninja 和 CMake：

```text
CMake → Ninja → MinGW g++ → im_client.exe
              ↘ Qt6 库和插件
```

## 2. CMake 的好处

- **跨平台**：同一套构建规则可以生成 Windows、Linux 和 macOS 所需的构建文件。
- **统一流程**：无论底层是 Ninja、Make 还是 Visual Studio，都可以使用 `cmake -S`、`cmake --build`。
- **依赖管理**：用 `find_package` 查找 Qt、Boost、OpenSSL 等库。
- **模块化**：用 `add_subdirectory` 拆分客户端、服务端、公共库和测试。
- **适合自动化**：本地、Ubuntu、Docker 和 CI/CD 可以复用同一套构建配置。
- **增量编译**：只重新编译发生变化的目标和源文件，缩短开发反馈时间。

## 3. 当前项目目录结构

当前项目根目录：

```text
C:\Users\gpf\Desktop\IM code\
├── AGENTS.md
├── .gitignore
├── CMakeLists.txt
├── client/
│   ├── CMakeLists.txt
│   └── src/
│       ├── main.cpp
│       ├── MainWindow.h
│       └── MainWindow.cpp
├── docs/
│   └── cmake-guide.md
└── test.cpp
```

构建目录与源码目录分离：

```text
build-ninja/       # Ninja 构建缓存和中间文件
build-vs/          # Visual Studio 构建缓存（如果使用）
```

构建目录不应提交到 Git，当前 `.gitignore` 已忽略 `build/` 和 `build-*/`。

## 4. 最小 CMake 项目

```cmake
cmake_minimum_required(VERSION 3.21)

project(Hello
    VERSION 1.0.0
    LANGUAGES CXX
)

add_executable(hello
    main.cpp
)
```

- `cmake_minimum_required`：声明最低 CMake 版本。
- `project`：定义项目名称、版本和语言。
- `add_executable`：创建可执行目标并列出源文件。
- `add_library`：创建静态库或动态库目标。

例如：

```cmake
add_library(protocol STATIC
    protocol.cpp
    protocol.h
)
```

## 5. 当前根目录 CMakeLists.txt 解析

当前根目录的关键配置：

```cmake
cmake_minimum_required(VERSION 3.21)

project(IMChat
    VERSION 0.1.0
    LANGUAGES CXX
)

include(CTest)

option(IM_BUILD_CLIENT "Build Qt desktop client" ON)
option(IM_BUILD_SERVER "Build Linux IM server" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(IM_BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(IM_BUILD_SERVER)
    add_subdirectory(server)
endif()
```

``CMAKE_CXX_STANDARD``、``CMAKE_CXX_STANDARD_REQUIRED`` 和 ``CMAKE_CXX_EXTENSIONS`` 表示使用并强制要求 C++17，且关闭编译器私有扩展。

Qt 自动处理选项放在 `client/CMakeLists.txt` 中，避免只构建 Ubuntu 服务端时引入 Qt 配置：

- `CMAKE_AUTOMOC`：处理含 Qt 元对象代码的类，例如含 `Q_OBJECT` 的类。
- `CMAKE_AUTOUIC`：自动处理 Qt Designer 的 `.ui` 文件。
- `CMAKE_AUTORCC`：自动处理 Qt 资源 `.qrc` 文件。

`IM_BUILD_CLIENT` 和 `IM_BUILD_SERVER` 让 Windows 客户端与 Ubuntu 服务端可以独立配置，避免客户端构建要求安装 Boost、PostgreSQL 和 Protobuf。`include(CTest)` 提供 `BUILD_TESTING` 开关。

条件化的 `add_subdirectory` 会按开关读取对应子项目。Ubuntu 服务端构建使用：

```bash
cmake -S . -B build-server -G Ninja \
  -DIM_BUILD_CLIENT=OFF \
  -DIM_BUILD_SERVER=ON \
  -DBUILD_TESTING=ON
```

## 6. 当前客户端 CMakeLists.txt 解析

```cmake
find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

qt_add_executable(im_client
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)

target_link_libraries(im_client
    PRIVATE
        Qt6::Widgets
)

target_include_directories(im_client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(im_client PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
```

### `find_package`

```cmake
find_package(Qt6 REQUIRED COMPONENTS Widgets)
```

查找 Qt6 Widgets 模块。 `REQUIRED` 表示找不到时配置失败。Qt 安装路径没有加入 PATH 时，可使用：

```powershell
cmake -S . -B build-ninja -G Ninja `
  -DCMAKE_PREFIX_PATH="D:\Qt6\6.11.1\mingw_64"
```

### `qt_add_executable`

创建 Qt 可执行目标 `im_client`。新增 `.cpp`、`.h` 或 `.ui` 文件后，应显式加入目标：

```cmake
qt_add_executable(im_client
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/LoginWindow.cpp
    src/LoginWindow.h
)
```

不建议使用 `file(GLOB ...)` 自动搜集源文件，因为新增文件后可能不会按预期触发重新配置。

### `target_link_libraries`

```cmake
target_link_libraries(im_client PRIVATE Qt6::Widgets)
```

将 Qt Widgets 链接到客户端。以后需要网络和 SQLite：

```cmake
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)

target_link_libraries(im_client PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)
```

### `target_include_directories`

```cmake
target_include_directories(im_client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
```

将 `client/src` 加入头文件搜索路径，使代码可以直接写 `#include "MainWindow.h"`。

### 目标属性

```cmake
set_target_properties(im_client PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
```

`WIN32_EXECUTABLE` 在 Windows 上隐藏控制台窗口；`MACOSX_BUNDLE` 让 macOS 目标成为应用包。

## 7. PRIVATE、PUBLIC 和 INTERFACE

| 关键字 | 当前目标使用 | 依赖当前目标的其他目标使用 |
| --- | --- | --- |
| `PRIVATE` | 是 | 否 |
| `PUBLIC` | 是 | 是 |
| `INTERFACE` | 否 | 是 |

客户端直接使用 Qt Widgets，因此使用：

```cmake
target_link_libraries(im_client PRIVATE Qt6::Widgets)
```

如果公共协议库的头文件暴露了某个依赖，可以使用 `PUBLIC`：

```cmake
target_link_libraries(protocol PUBLIC Qt6::Core)
```

## 8. Qt6 项目配置

### Widgets、Network 和 Sql

```cmake
find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Network
    Sql
)

target_link_libraries(im_client PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)
```

对应代码可以使用：

```cpp
#include <QTcpSocket>
#include <QSqlDatabase>
```

### Qt Designer 的 `.ui` 文件

将 `client/ui/LoginWindow.ui` 加入目标：

```cmake
qt_add_executable(im_client
    src/main.cpp
    src/LoginWindow.cpp
    src/LoginWindow.h
    ui/LoginWindow.ui
)
```

客户端目录已经开启 `CMAKE_AUTOUIC`，因此 Qt 会自动处理 `.ui` 文件。

### Qt 资源 `.qrc` 文件

```cmake
qt_add_executable(im_client
    src/main.cpp
    resources/resources.qrc
)
```

客户端目录已经开启 `CMAKE_AUTORCC`，因此 Qt 会自动生成资源代码。

## 9. 构建目录和生成器

推荐使用源码外构建：

```powershell
cmake -S . -B build-ninja -G Ninja
```

这样可以保持源码目录整洁，并同时维护不同编译器或配置的构建目录。

常用生成器：

```text
Ninja                  快速，适合命令行和 Qt Creator
MinGW Makefiles        使用 MinGW 的 Makefile
Visual Studio 17 2022  生成 Visual Studio 工程
Unix Makefiles         Linux/macOS 常见 Makefile
```

不同生成器不能共用一个构建目录。例如使用过 Visual Studio 的 `build` 不能直接改成 Ninja，应使用 `build-ninja` 或 `build-vs`。

## 10. 当前项目的实际构建流程

当前工具链：

```text
Qt 6.11.1
MinGW 13.1
Ninja
CMake 4.4.2
```

PowerShell 命令：

```powershell
cd "C:\Users\gpf\Desktop\IM code"

$env:Path = "D:\Qt6\Tools\mingw1310_64\bin;D:\Qt6\Tools\Ninja;C:\Program Files\CMake\bin;$env:Path"

cmake -S . -B build-ninja -G Ninja `
  -DCMAKE_PREFIX_PATH="D:\Qt6\6.11.1\mingw_64"

cmake --build build-ninja --parallel 2
```

参数含义：

- `-S .`：源码目录为当前目录。
- `-B build-ninja`：构建目录为 `build-ninja`。
- `-G Ninja`：使用 Ninja 生成器。
- `-D...`：设置 CMake 配置变量。
- `--parallel 2`：最多并行两个编译任务。

程序输出位置：

```text
build-ninja/client/im_client.exe
```

运行：

```powershell
.\build-ninja\client\im_client.exe
```

## 11. Qt 程序发布和 DLL 部署

CMake 负责编译和链接，但不会自动把 Qt DLL 复制到 exe 旁边。直接双击时，如果 DLL 不在程序目录或系统 PATH 中，就会出现“找不到 Qt6Core.dll”等错误。

使用 Qt 自带的 `windeployqt`：

```powershell
& "D:\Qt6\6.11.1\mingw_64\bin\windeployqt.exe" `
  ".\build-ninja\client\im_client.exe"
```

部署后常见文件：

| 文件或目录 | 作用 |
| --- | --- |
| `Qt6Core.dll` | Qt 核心功能 |
| `Qt6Gui.dll` | GUI、窗口系统和绘图 |
| `Qt6Widgets.dll` | Widgets 控件 |
| `libgcc_s_seh-1.dll` | MinGW GCC 运行时 |
| `libstdc++-6.dll` | C++ 标准库运行时 |
| `libwinpthread-1.dll` | MinGW 线程运行时 |
| `platforms/qwindows.dll` | Windows Qt 平台插件 |

发布时不要只复制 exe，应将 exe、DLL 和插件目录一起复制或打包。

## 12. 常用 CMake 命令

```powershell
# 配置并生成构建文件
cmake -S . -B build-ninja -G Ninja

# 编译
cmake --build build-ninja

# 查看目标
cmake --build build-ninja --target help

# 清理编译产物
cmake --build build-ninja --target clean

# 运行测试（项目启用测试后）
ctest --test-dir build-ninja --output-on-failure
```

修改 `CMakeLists.txt` 或依赖后，重新配置：

```powershell
cmake -S . -B build-ninja
```

Ninja 通常使用单配置构建目录：

```powershell
cmake -S . -B build-debug -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_PREFIX_PATH="D:\Qt6\6.11.1\mingw_64"

cmake --build build-debug
```

Release 同理，只需改为 `-DCMAKE_BUILD_TYPE=Release`。Visual Studio 是多配置生成器：

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022"
cmake --build build-vs --config Debug
```

## 13. 常见问题排查

### CMake 不在 PATH

症状：

```text
cmake is not recognized
```

临时解决：

```powershell
$env:Path = "C:\Program Files\CMake\bin;$env:Path"
```

### Qt6 找不到

确认 `CMAKE_PREFIX_PATH` 指向 Qt 安装前缀，而不是 `bin` 目录：

```powershell
cmake -S . -B build-ninja -G Ninja `
  -DCMAKE_PREFIX_PATH="D:\Qt6\6.11.1\mingw_64"
```

### 生成器不匹配

症状：

```text
generator does not match the generator used previously
```

原因是构建目录缓存了其他生成器。推荐换新目录，例如 `build-ninja` 和 `build-vs` 分开使用。

### 新增源文件后没有参与编译

将新文件加入 `qt_add_executable`，再重新配置：

```powershell
cmake -S . -B build-ninja
cmake --build build-ninja
```

### Debug/Release 混用

不同配置使用不同构建目录，不能混用对应 DLL。对相应 exe 重新执行对应配置的 `windeployqt`。

### 双击 exe 缺少 DLL

执行：

```powershell
& "D:\Qt6\6.11.1\mingw_64\bin\windeployqt.exe" `
  ".\build-ninja\client\im_client.exe"
```

确认 Qt DLL、MinGW DLL 与 exe 位于同一目录。

### Qt 平台插件缺失

确认存在：

```text
build-ninja/client/platforms/qwindows.dll
```

通常重新执行 `windeployqt` 即可解决。

## 14. 后续 IM 项目扩展

### 登录窗口

新增 `LoginWindow.cpp` 和 `LoginWindow.h`，加入 `qt_add_executable`。初期让登录界面只负责输入和页面跳转，不把网络实现直接混入界面类。

### Qt Network 和 SQLite

客户端需要网络和本地数据库时：

```cmake
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)

target_link_libraries(im_client PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)
```

### 服务端

建立 `server/` 后，根目录可写：

```cmake
add_subdirectory(client)
add_subdirectory(server)
```

服务端可以使用 Boost.Asio、PostgreSQL 和独立的 `server/CMakeLists.txt`。

### 测试

```cmake
enable_testing()
add_subdirectory(tests)
```

测试目标建立后，用 `ctest --test-dir build-ninja --output-on-failure` 运行。

### Linux Ubuntu

Linux 上通常使用 Bash：

```bash
cmake -S . -B build-linux -G Ninja
cmake --build build-linux
```

同一套目标化 CMake 写法可以复用，差异主要在依赖安装、编译器和部署方式。

## 15. CMake 速查表

| 命令或变量 | 用途 |
| --- | --- |
| `cmake_minimum_required` | 设置最低 CMake 版本 |
| `project` | 声明项目名称、版本和语言 |
| `set` | 设置变量或 CMake 选项 |
| `add_subdirectory` | 加入子目录构建规则 |
| `add_executable` | 创建普通可执行目标 |
| `qt_add_executable` | 创建 Qt 可执行目标 |
| `add_library` | 创建库目标 |
| `find_package` | 查找外部依赖 |
| `target_link_libraries` | 为目标链接库 |
| `target_include_directories` | 设置头文件搜索目录 |
| `target_compile_features` | 设置 C++ 标准能力 |
| `target_compile_options` | 设置目标编译参数 |
| `target_compile_definitions` | 设置预处理宏 |
| `enable_testing` | 开启测试支持 |
| `add_test` | 注册测试命令 |
| `cmake -S -B` | 配置源码和构建目录 |
| `cmake --build` | 编译指定构建目录 |

## 16. 学习路线

### 第一阶段：能编译

掌握 `cmake_minimum_required`、`project`、`add_executable`、`set`、`cmake -S -B` 和 `cmake --build`。

### 第二阶段：能开发 Qt 项目

掌握 `find_package`、`qt_add_executable`、`target_link_libraries`、`AUTOMOC`、`AUTOUIC`、`AUTORCC`、`CMAKE_PREFIX_PATH` 和 Debug/Release 构建。

### 第三阶段：能维护 IM 工程

掌握 `add_subdirectory`、`add_library`、`PRIVATE/PUBLIC/INTERFACE`、测试目标、Qt Network、SQLite、Windows DLL 部署和 Linux/Ubuntu 构建。

### 第四阶段：项目变大后再学习

学习 CMake Presets、`FetchContent`、自定义 Find 模块、安装和导出 Package、CPack、CI/CD 与交叉编译。

最重要的链路：

```text
源文件
  ↓ qt_add_executable / add_library
find_package 查找依赖
  ↓
target_link_libraries 链接依赖
  ↓
cmake -S -B 配置
  ↓
cmake --build 编译
  ↓
windeployqt 部署 Qt 运行时
```
