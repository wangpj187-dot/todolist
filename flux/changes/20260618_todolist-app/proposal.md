# Proposal: Todolist 应用开发方案

**Feature Directory**: `20260618_todolist-app`  
**Created**: 2026-06-18  
**Status**: Done  
**Input**: User description: "帮我整理一个todolist的开发方案"
<!-- Add local-file-reference if exists, One bullet per file reference: -->

---

## Clarification

### User Alignment

<!-- Q&A with the user. One bullet per clarification: -->
- Q: 应用类型是什么？ → A: Windows 和 Mac 桌面摆件应用
- Q: 功能范围？ → A: 完整版（基础待办 + 优先级/分类/截止日期/统计 + 多设备同步/云端备份 + 主题自定义 + 动画效果）
- Q: 云端备份方式？ → A: 使用 GitHub 仓库存储备份数据
- Q: 技术栈偏好？ → A: Qt (C++/QML)
- Q: UI 风格？ → A: 简约现代（扁平化设计，清爽简洁）

### Assumptions

<!-- Reasonable defaults assumed based on context and industry standards. Not explicitly confirmed with the user, but recorded here for visibility. User can review and correct if needed. -->
- **数据格式**: 使用 JSON 格式存储待办事项数据，便于 Git 版本控制和 GitHub 仓库同步
- **本地存储**: 使用 SQLite 作为本地数据库，提供高性能的数据查询和持久化
- **GitHub 集成**: 使用 libgit2 或 GitHub REST API 进行仓库操作，用户需配置 GitHub Token
- **同步策略**: 手动触发同步 + 自动定时同步（可配置），采用基于文件哈希的冲突检测
- **主题系统**: 基于 QML 的动态属性系统实现主题切换，提供浅色/深色主题默认配置
- **提醒机制**: 使用 Qt 的 QSystemTrayIcon 实现系统托盘通知，结合 QTimer 实现截止日期提醒
- **窗口特性**: 支持窗口置顶、无边框、可拖拽、透明度调节等桌面摆件特性
- **数据统计**: 本地计算完成率、分类占比、趋势图表等，使用 Qt Charts 实现可视化

---

## Design

### 技术方案选择

**已选方案：方案 A - Qt QML + C++ 现代分层架构**

**选择理由：**
- 桌面摆件应用需要丰富的动画效果和自定义主题，QML 声明式 UI 是最佳选择
- 分层架构清晰，便于维护和扩展
- Qt Quick 提供的手势支持、透明度控制、无边框窗口等特性非常适合桌面摆件场景
- 性能优秀，资源占用低

**其他考虑过的方案：**
- 方案 B（Qt Widgets）：UI 定制性和动画效果较弱，不适合桌面摆件的视觉需求
- 方案 C（混合架构）：架构复杂度高，需要维护两套 UI 系统

---

### 系统架构设计

#### 整体分层架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Presentation Layer (QML)                       │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │ Main Window │  │ Todo List   │  │ Detail View │  │ Statistics  │  │
│  │  (Floating) │  │  Component  │  │  Component  │  │  View       │  │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  │
│         │                │                │                │         │
│  ┌──────┴──────┐  ┌──────┴──────┐  ┌──────┴──────┐  ┌──────┴──────┐  │
│  │ Theme Sys   │  │ Animation   │  │ Gesture     │  │ Tray/Notify │  │
│  │  (Dynamic)  │  │  System     │  │  Handler    │  │  Manager    │  │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │
└───────────────────────────────────┬───────────────────────────────────┘
                                    │
                        ┌───────────┴───────────┐
                        │  QML/C++ Bridge       │
                        │  (Context Properties) │
                        └───────────┬───────────┘
                                    │
┌───────────────────────────────────┴───────────────────────────────────┐
│                        Business Logic Layer (C++)                      │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐        │
│  │ TodoService     │  │ SyncService     │  │ ReminderService │        │
│  │  - CRUD ops     │  │  - Git push/pull│  │  - Deadline     │        │
│  │  - Priority     │  │  - Conflict res.│  │    monitoring   │        │
│  │  - Category mgmt│  │  - Auto-sync    │  │  - Notifications│        │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘        │
│           │                     │                     │                 │
│  ┌────────┴────────┐  ┌────────┴────────┐  ┌────────┴────────┐        │
│  │ StatsService    │  │ ThemeService    │  │ ConfigService   │        │
│  │  - Completion   │  │  - Theme load/s.│  │  - Settings      │        │
│  │    rate         │  │  - Dynamic      │  │  - Persistence   │        │
│  │  - Trends       │  │    switching    │  │                 │        │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘        │
└───────────────────────────────────┬───────────────────────────────────┘
                                    │
┌───────────────────────────────────┴───────────────────────────────────┐
│                         Data Access Layer (C++)                        │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐        │
│  │ DatabaseManager │  │ JsonSerializer  │  │ GitClient       │        │
│  │  - SQLite       │  │  - Todo <-> JSON│  │  - libgit2      │        │
│  │  - CRUD queries │  │    conversion   │  │  - GitHub API    │        │
│  │  - Migrations   │  │  - Backup/restore│ │  - Auth (Token)  │        │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘        │
└─────────────────────────────────────────────────────────────────────────┘
```

#### 架构说明

1. **表现层（QML）**：
   - 主窗口采用无边框悬浮设计，支持拖拽、置顶、透明度调节
   - 组件化设计：TodoList、TodoDetail、StatisticsView 等独立组件
   - 主题系统基于 QML 动态属性，支持运行时切换
   - 动画系统使用 Qt Quick Animations，实现平滑过渡效果

2. **业务逻辑层（C++）**：
   - TodoService：核心待办事项管理，包含 CRUD、优先级、分类管理
   - SyncService：GitHub 同步引擎，处理 push/pull、冲突检测与解决
   - ReminderService：截止日期监控，系统托盘通知
   - StatsService：数据统计与分析
   - ThemeService：主题加载与切换
   - ConfigService：用户配置持久化

3. **数据访问层（C++）**：
   - DatabaseManager：SQLite 数据库封装，提供数据持久化
   - JsonSerializer：JSON 序列化/反序列化，用于 GitHub 备份
   - GitClient：libgit2 封装，处理 Git 仓库操作

#### 核心设计原则

- 关注点分离：UI、业务逻辑、数据访问严格分层
- 依赖倒置：高层模块不依赖低层模块，都依赖抽象
- 单一职责：每个类/组件只有一个变更理由
- 可测试性：业务逻辑层不依赖 UI，便于单元测试

---

### 核心组件设计

#### 数据模型

**Todo 实体**：
```
Todo {
    id: UUID (主键)
    title: string (标题，必填，最大长度 200)
    description: string (描述，可选)
    priority: enum (LOW=1, MEDIUM=2, HIGH=3, URGENT=4)
    category: string (分类标签，可选)
    status: enum (PENDING=1, IN_PROGRESS=2, COMPLETED=3, CANCELLED=4)
    dueDate: DateTime (截止日期，可选)
    createdAt: DateTime (创建时间)
    updatedAt: DateTime (更新时间)
    completedAt: DateTime (完成时间，可选)
    tags: list<string> (标签列表，可选)
}
```

**Category 实体**：
```
Category {
    id: UUID (主键)
    name: string (分类名称，唯一，必填)
    color: string (颜色标识，HEX 格式)
    icon: string (图标名称，可选)
    createdAt: DateTime
}
```

**Config 实体**：
```
Config {
    theme: string (主题名称，默认 "light")
    autoSync: bool (自动同步，默认 true)
    syncInterval: int (同步间隔，分钟，默认 30)
    githubToken: string (GitHub Token，加密存储)
    githubRepo: string (GitHub 仓库地址)
    githubBranch: string (分支名称，默认 "main")
    windowOpacity: float (窗口透明度，0.5-1.0，默认 0.9)
    alwaysOnTop: bool (窗口置顶，默认 true)
    reminderEnabled: bool (提醒开关，默认 true)
}
```

---

### 数据流程设计

#### 待办事项 CRUD 流程

```
用户操作 (QML UI)
    │
    ▼
TodoService (C++)
    ├─► 验证输入数据
    ├─► 调用 DatabaseManager 执行 SQL
    │    ├─► INSERT/UPDATE/DELETE/SELECT
    │    └─► 返回操作结果
    ├─► 更新内存缓存
    ├─► 触发 QML 信号更新 UI
    └─► 标记数据为"待同步"状态
```

#### GitHub 同步流程

```
同步触发 (手动/定时)
    │
    ▼
SyncService
    ├─► 检查网络连接
    ├─► 验证 GitHub Token
    ├─► 调用 GitClient
    │    ├─► Pull: 拉取远程变更
    │    │    └─► 检测冲突 (基于文件哈希)
    │    │         ├─► 无冲突: 合并到本地
    │    │         └─► 有冲突: 提示用户选择版本
    │    ├─► Push: 推送本地变更
    │    │    └─► JSON 序列化待办数据
    │    │         └─► 提交到 GitHub 仓库
    │    └─► 返回同步结果
    ├─► 更新同步状态
    └─► 通知 UI 显示同步结果
```

#### 提醒流程

```
ReminderService 启动
    │
    ▼
QTimer 定时检查 (每分钟)
    ├─► 查询所有未完成且有截止日期的 Todo
    ├─► 比较当前时间与截止日期
    │    ├─► 即将到期 (提前 1 小时): 显示预告提醒
    │    ├─► 已到期: 显示到期提醒
    │    └─► 已过期: 显示过期提醒
    └─► 调用 QSystemTrayIcon 显示系统通知
         └─► 用户点击通知 → 打开应用并定位到相关 Todo
```

---

### 错误处理策略

| 错误类型 | 处理策略 | 用户反馈 |
|---------|---------|---------|
| 数据库连接失败 | 重试 3 次，间隔 1 秒；失败则使用内存模式 | 弹窗提示，建议重启应用 |
| GitHub 认证失败 | 清除无效 Token，提示重新配置 | 弹窗引导用户重新设置 GitHub Token |
| 网络连接失败 | 缓存待同步数据，网络恢复后自动重试 | 状态栏显示离线模式图标 |
| 同步冲突 | 保留两个版本，提示用户选择合并策略 | 冲突解决对话框，展示差异 |
| 数据验证失败 | 拒绝操作，高亮错误字段 | 表单内联错误提示 |
| 文件读写错误 | 记录日志，尝试备用路径 | 错误提示 + 日志导出选项 |

---

### 测试策略

#### 测试金字塔

```
          /\       E2E Tests (10%)
         /  \      - 完整用户流程
        /____\     - 跨组件集成
       /      \
      /        \   Integration Tests (20%)
     /__________\  - 服务层交互
    /            \ - 数据访问层
   /              \
  /________________\ Unit Tests (70%)
                     - 业务逻辑
                     - 工具函数
                     - 数据验证
```

#### 核心测试场景

1. **单元测试**：
   - TodoService：CRUD 操作、优先级排序、分类过滤
   - SyncService：冲突检测、JSON 序列化、Git 操作模拟
   - ReminderService：截止日期计算、通知触发逻辑
   - DatabaseManager：SQL 查询、事务处理、迁移脚本

2. **集成测试**：
   - 完整的创建→编辑→完成→删除流程
   - 本地数据→JSON 序列化→GitHub 同步→反向恢复
   - 主题切换→UI 更新→配置持久化

3. **端到端测试**：
   - 首次启动→配置 GitHub→创建待办→同步→另一设备恢复
   - 离线模式创建→联网后自动同步→冲突解决

---

### 项目结构

```
todolist-app/
├── CMakeLists.txt              # CMake 构建配置
├── README.md                   # 项目说明
├── src/
│   ├── main.cpp                # 应用入口
│   ├── business/               # 业务逻辑层
│   │   ├── TodoService.h/cpp
│   │   ├── SyncService.h/cpp
│   │   ├── ReminderService.h/cpp
│   │   ├── StatsService.h/cpp
│   │   ├── ThemeService.h/cpp
│   │   └── ConfigService.h/cpp
│   ├── data/                   # 数据访问层
│   │   ├── DatabaseManager.h/cpp
│   │   ├── JsonSerializer.h/cpp
│   │   └── GitClient.h/cpp
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

---

## Capabilities

### New Capabilities
- **待办事项管理**：创建、查看、编辑、删除、标记完成待办事项
- **优先级与分类**：支持 4 级优先级和自定义分类标签
- **截止日期提醒**：截止日期监控和系统托盘通知
- **GitHub 云端备份**：使用 GitHub 仓库存储备份数据，支持多设备同步
- **数据统计**：完成率、分类占比、趋势图表等可视化统计
- **主题自定义**：浅色/深色主题切换，支持自定义主题
- **桌面摆件特性**：窗口置顶、无边框、可拖拽、透明度调节
- **动画效果**：平滑的过渡动画和交互动效

### Modified Capabilities
- 无（全新项目）

### Unchanged Capabilities
- 无（全新项目）