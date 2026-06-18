# Quickstart: Todolist 桌面摆件应用

## 1. 项目概述

Todolist 是一个运行在 Windows 和 macOS 平台的桌面待办事项管理摆件应用。采用 Qt 6 + QML 技术栈，具有桌面摆件特性（悬浮窗口、主题自定义、动画效果），并支持使用 GitHub 仓库进行云端备份和多设备同步。

## 2. 环境准备

### 2.1 系统要求

| 平台 | 最低版本 |
|------|----------|
| Windows | Windows 10 1809+ |
| macOS | macOS 11.0 (Big Sur)+ |

### 2.2 开发环境

#### 必需工具

| 工具 | 版本要求 | 说明 |
|------|----------|------|
| Qt | 6.5+ | 包含 Qt Quick, Qt SQL, Qt Charts 模块 |
| CMake | 3.20+ | 构建系统 |
| C++ 编译器 | 支持 C++17 | MSVC 2019+, Clang 13+, GCC 11+ |
| Git | 2.30+ | 版本控制 |
| libgit2 | 1.7+ | Git 操作库 |
| nlohmann/json | 3.11+ | JSON 序列化库 |

#### Qt 模块

必需的 Qt 模块：
- `qtbase` - 基础模块
- `qtdeclarative` - QML 支持
- `qtquickcontrols2` - Qt Quick 控件
- `qtsql` - 数据库支持
- `qtcharts` - 图表组件
- `qttools` - 开发工具

### 2.3 安装依赖

#### Windows (使用 vcpkg)

```powershell
# 安装 vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 安装依赖
.\vcpkg install libgit2:x64-windows nlohmann-json:x64-windows

# 安装 Qt (使用官方安装器)
# 下载地址: https://www.qt.io/download
```

#### macOS (使用 Homebrew)

```bash
# 安装 Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install cmake libgit2 nlohmann-json

# 安装 Qt
brew install qt@6
```

#### Linux (Ubuntu 22.04+)

```bash
# 安装构建工具
sudo apt update
sudo apt install build-essential cmake git

# 安装 Qt 6
sudo apt install qt6-base-dev qt6-declarative-dev qt6-charts-dev \
                 qt6-sql-dev libqt6sql6-sqlite

# 安装其他依赖
sudo apt install libgit2-dev nlohmann-json3-dev
```

## 3. 获取源码

```bash
# 克隆仓库
git clone <repository-url>
cd todolist-app

# 初始化子模块（如果有）
git submodule update --init --recursive
```

## 4. 构建项目

### 4.1 配置 CMake

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.5.0 \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake  # Windows only
```

### 4.2 编译

```bash
# 编译（使用所有 CPU 核心）
cmake --build . --config Release --parallel

# 或者使用 make（Linux/macOS）
make -j$(nproc)

# 或者使用 MSBuild（Windows）
msbuild Todolist.sln /p:Configuration=Release /m
```

### 4.3 运行应用

```bash
# Linux/macOS
./src/Todolist

# Windows
.\src\Release\Todolist.exe
```

## 5. 首次运行配置

### 5.1 基础配置

1. 启动应用后，右键点击系统托盘图标
2. 选择 "设置"
3. 配置以下选项：
   - 主题：浅色/深色
   - 窗口透明度：0.5-1.0
   - 窗口置顶：开启/关闭
   - 提醒功能：开启/关闭

### 5.2 GitHub 同步配置

1. 创建 GitHub Personal Access Token：
   - 访问 https://github.com/settings/tokens
   - 点击 "Generate new token"
   - 选择 `repo` 权限
   - 生成并复制 Token

2. 在应用中配置 GitHub：
   - 打开设置 → 备份与同步
   - 输入 GitHub Token
   - 输入备份仓库地址（如 `username/todolist-backup`）
   - 输入分支名称（默认 `main`）
   - 点击 "测试连接" 验证配置

3. 首次同步：
   - 点击 "立即同步"
   - 应用会自动创建仓库（如果不存在）
   - 上传本地数据到 GitHub

## 6. 开发指南

### 6.1 项目结构

```
todolist-app/
├── CMakeLists.txt              # CMake 配置
├── src/
│   ├── main.cpp                # 应用入口
│   ├── business/               # 业务逻辑层
│   │   ├── TodoService.*       # 待办事项服务
│   │   ├── SyncService.*       # 同步服务
│   │   ├── ReminderService.*   # 提醒服务
│   │   ├── StatsService.*      # 统计服务
│   │   ├── ThemeService.*      # 主题服务
│   │   └── ConfigService.*     # 配置服务
│   ├── data/                   # 数据访问层
│   │   ├── DatabaseManager.*   # 数据库管理
│   │   ├── JsonSerializer.*    # JSON 序列化
│   │   └── GitClient.*         # Git 客户端
│   ├── models/                 # 数据模型
│   │   ├── Todo.h              # 待办事项模型
│   │   ├── Category.h          # 分类模型
│   │   └── Config.h            # 配置模型
│   └── qml/                    # QML UI 层
│       ├── main.qml            # 主窗口
│       ├── components/         # UI 组件
│       └── themes/             # 主题文件
├── assets/                     # 资源文件
├── tests/                      # 测试代码
└── docs/                       # 文档
```

### 6.2 添加新功能

#### 步骤 1: 添加数据模型（如需要）

```cpp
// src/models/NewModel.h
#pragma once
#include <QString>

class NewModel {
public:
    // ... 模型定义
};
```

#### 步骤 2: 添加服务接口

```cpp
// src/business/INewService.h
#pragma once
#include <QObject>

class INewService : public QObject {
    Q_OBJECT
public:
    virtual ~INewService() = default;
    // ... 接口定义
signals:
    // ... 信号定义
};
```

#### 步骤 3: 实现服务

```cpp
// src/business/NewService.h
#pragma once
#include "INewService.h"

class NewService : public INewService {
    Q_OBJECT
public:
    // ... 实现接口
};
```

#### 步骤 4: 注册到 QML

```cpp
// src/main.cpp
QQmlApplicationEngine engine;
NewService newService;
engine.rootContext()->setContextProperty("newService", &newService);
```

#### 步骤 5: 创建 QML 组件

```qml
// src/qml/components/NewComponent.qml
import QtQuick 2.15

Item {
    // ... UI 实现
}
```

### 6.3 代码规范

#### C++ 规范

- 遵循 C++17 标准
- 使用 Qt 编码风格（lowerCamelCase 方法名，UpperCamelCase 类名）
- 头文件使用 `#pragma once`
- 指针使用 Qt 智能指针（`QScopedPointer`, `QSharedPointer`）
- 内存管理：父对象机制管理子对象生命周期

#### QML 规范

- 使用 Qt Quick Controls 2
- 组件使用 UpperCamelCase 命名
- 属性使用 lowerCamelCase
- 信号使用 lowerCamelCase
- 遵循 QML 编码最佳实践

#### Git 提交规范

```
<type>(<scope>): <subject>

<body>
```

Type: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `chore`

示例：
```
feat(todo): add priority filtering

- Add priority filter dropdown
- Implement filtering logic in TodoService
- Add unit tests for filtering
```

## 7. 测试

### 7.1 运行单元测试

```bash
cd build
ctest -C Release --output-on-failure

# 或者直接运行测试可执行文件
./tests/unit/TodolistTests
```

### 7.2 运行集成测试

```bash
./tests/integration/IntegrationTests
```

### 7.3 代码覆盖率

```bash
# 使用 gcov/lcov 生成覆盖率报告
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make -j$(nproc)
make coverage
```

## 8. 打包发布

### 8.1 Windows 打包

```powershell
# 使用 windeployqt 收集依赖
windeployqt.exe --release --qmldir ../src/qml src/Release/Todolist.exe

# 使用 NSIS 或 Inno Setup 创建安装包
# 或者使用 cpack
cpack -C Release
```

### 8.2 macOS 打包

```bash
# 使用 macdeployqt 收集依赖
macdeployqt Todolist.app -qmldir=../src/qml

# 创建 DMG
hdiutil create -volname Todolist -srcfolder Todolist.app -ov -format UDZO Todolist.dmg

# 或者使用 cpack
cpack -C Release
```

### 8.3 版本号管理

遵循语义化版本（SemVer）：
- 主版本号：不兼容的 API 变更
- 次版本号：向下兼容的功能新增
- 修订号：向下兼容的问题修正

## 9. 常见问题

### 9.1 构建问题

**Q: CMake 找不到 Qt**
A: 设置 `CMAKE_PREFIX_PATH` 环境变量指向 Qt 安装路径：
```bash
export CMAKE_PREFIX_PATH=/path/to/Qt/6.5.0/gcc_64
```

**Q: 找不到 libgit2**
A: 确保 libgit2 已安装，或者使用 vcpkg 管理依赖。

### 9.2 运行问题

**Q: 应用启动后窗口不显示**
A: 检查系统托盘图标，应用可能最小化到托盘了。

**Q: GitHub 同步失败**
A: 检查以下几点：
   - Token 是否正确且具有 `repo` 权限
   - 仓库地址格式是否正确（`owner/repo`）
   - 网络连接是否正常
   - GitHub 是否可以正常访问

### 9.3 数据问题

**Q: 数据丢失了怎么办**
A: 可以从 GitHub 恢复：
   1. 打开设置 → 备份与同步
   2. 点击 "从 GitHub 恢复"
   3. 选择要恢复的时间点

**Q: 如何手动备份数据**
A: 数据库文件位置：
   - Windows: `%APPDATA%\Todolist\todolist.db`
   - macOS: `~/Library/Application Support/Todolist/todolist.db`

## 10. 文档索引

- [功能规范](spec.md)
- [技术方案](plan.md)
- [数据模型](data-model.md)
- [接口契约](contracts/)
- [API 文档](../docs/API.md)
- [部署文档](../docs/DEPLOYMENT.md)
- [贡献指南](../docs/CONTRIBUTING.md)

<!-- cli_version: 0.1.41 -->
