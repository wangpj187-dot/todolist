# Implementation Plan: Todolist 桌面摆件应用

**Feature**: `20260618_todolist-app` | **Date**: 2026-06-18 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `flux/changes/20260618_todolist-app/spec.md`

**Note**: This template is filled in by the skill `flux-plan` command. See `plugins/flux/core/commands/plan.md` for the execution workflow.

## Summary

开发一个运行在 Windows 和 Mac 平台的桌面待办事项管理摆件应用。采用 Qt 6 + QML 技术栈，实现待办事项 CRUD、优先级管理、分类标签、截止日期提醒、GitHub 云端备份、数据统计、主题自定义等功能。应用采用三层架构（表现层 QML / 业务逻辑层 C++ / 数据访问层 C++），使用 SQLite 作为本地数据库，libgit2 实现 GitHub 仓库同步。

## Technical Context

**Language/Version**: C++17, QML (Qt 6.5+)  
**Primary Dependencies**: Qt 6.5+ (Qt Quick, Qt SQL, Qt Charts), libgit2 1.7+, nlohmann/json 3.11+  
**Storage**: SQLite 3 (本地数据库), JSON (GitHub 备份格式)  
**Testing**: Qt Test  
**Target Platform**: Windows 10+, macOS 11+  
**Project Type**: single (桌面应用)  
**Performance Goals**: 启动时间 < 2s, UI 动画 60fps, 数据查询 < 50ms, 内存占用 < 100MB  
**Constraints**: 离线可用, 低内存占用, 快速启动, GitHub Token 加密存储  
**Scale/Scope**: 单用户应用, 预计 1-2 万行代码, 8-10 个核心界面组件

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

无 constitution 文件，跳过检查。

## Project Structure

### Documentation (this feature)

```text
flux/changes/20260618_todolist-app/
├── proposal.md          # 需求对齐文档 (Brainstorm 输出)
├── spec.md              # 功能/技术规范 (specify 输出)
├── plan.md              # 本文件 (flux-plan 输出)
├── research.md          # Phase 0 输出 (flux-plan 输出)
├── data-model.md        # Phase 1 输出 (flux-plan 输出)
├── quickstart.md        # Phase 1 输出 (flux-plan 输出)
├── contracts/           # Phase 1 输出 (flux-plan 输出)
└── tasks.md             # Phase 2 输出 (flux-tasks 输出)
```

### Source Code (repository root)

```text
todolist-app/
├── CMakeLists.txt              # CMake 构建配置
├── README.md                   # 项目说明
├── src/
│   ├── main.cpp                # 应用入口
│   ├── business/               # 业务逻辑层
│   │   ├── TodoService.h
│   │   ├── TodoService.cpp
│   │   ├── SyncService.h
│   │   ├── SyncService.cpp
│   │   ├── ReminderService.h
│   │   ├── ReminderService.cpp
│   │   ├── StatsService.h
│   │   ├── StatsService.cpp
│   │   ├── ThemeService.h
│   │   ├── ThemeService.cpp
│   │   ├── ConfigService.h
│   │   └── ConfigService.cpp
│   ├── data/                   # 数据访问层
│   │   ├── DatabaseManager.h
│   │   ├── DatabaseManager.cpp
│   │   ├── JsonSerializer.h
│   │   ├── JsonSerializer.cpp
│   │   ├── GitClient.h
│   │   └── GitClient.cpp
│   ├── models/                 # 数据模型
│   │   ├── Todo.h
│   │   ├── Category.h
│   │   └── Config.h
│   └── qml/                    # QML UI 层
│       ├── main.qml
│       ├── components/
│       │   ├── TodoList.qml
│       │   ├── TodoItem.qml
│       │   ├── TodoDetail.qml
│       │   ├── StatisticsView.qml
│       │   └── SettingsDialog.qml
│       └── themes/
│           ├── LightTheme.qml
│           └── DarkTheme.qml
├── assets/                     # 资源文件
│   ├── icons/
│   └── translations/
├── tests/                      # 测试代码
│   ├── unit/
│   ├── integration/
│   └── e2e/
└── docs/                       # 文档
    ├── API.md
    ├── DEPLOYMENT.md
    └── CONTRIBUTING.md
```

**Structure Decision**: 采用单项目结构，按照分层架构组织代码。业务逻辑层、数据访问层、表现层（QML）严格分离，便于维护和测试。

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

无违规项。

<!-- cli_version: 0.1.41 -->
