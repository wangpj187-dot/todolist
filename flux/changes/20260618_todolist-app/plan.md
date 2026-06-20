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

## UI / Interaction Addendum（2026-06-19）

本轮界面优化参考 Apple 系统“提醒事项”的信息架构与交互节奏，目标是解决当前 UI 错位、编辑/删除按钮不稳定，以及弹窗表单层级偏重的问题。

### 主界面

- 采用清单优先布局：顶部标题与新建按钮，中间智能清单快捷入口，下方搜索 / 状态 / 优先级 / 排序工具条，主体为待办列表。
- 智能清单首批提供：全部、今天、已计划、高优先级、已完成。
- 窄窗口下工具条自动换行，控件使用固定高度与响应式宽度，避免 ComboBox / SearchField 互相挤压造成错位。
- 列表项取消整卡点击层，复选框只负责完成 / 取消完成，编辑和删除按钮使用独立点击目标。

### 新建 / 编辑弹窗

- 表单首屏聚焦标题、备注、优先级、分类、截止日期，降低视觉噪音。
- 截止日期使用独立卡片和日历弹层，支持今天 / 明天快捷选择与清除日期。
- 空截止日期从 QML 传递为 `null`，由 C++ 服务层转换为无效 `QDateTime`，避免 QML 到 C++ 参数类型不兼容。

### 服务层

- 在 `TodoService` 增加轻量智能筛选模式，不改变数据库结构。
- `今天`、`已计划`、`高优先级` 智能清单在 C++ 中过滤，保持 QML 只负责展示与切换。

### Mac Reminders Follow-up（2026-06-19）

- 补齐标签入口：表单支持输入多个标签，列表项展示标签芯片，功能面板支持按标签筛选。
- 补齐旗标入口：使用系统标签 `旗标` 落地，支持表单勾选、列表行快捷切换和“有旗标”智能清单。
- 调整智能清单：`今天` 包含逾期未完成事项；新增 `有旗标`；保留 `已计划`、`高优先级`、`已完成`。
- 补齐键盘操作：支持标准新建和查找快捷键，查找会打开功能面板并聚焦搜索框。
- 暂不实现的高成本功能：子任务、分区、列表组、地点提醒、重复提醒、日历集成。这些需要新增数据模型或平台权限，后续独立设计。

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
