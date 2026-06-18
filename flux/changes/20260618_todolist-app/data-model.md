# Data Model: Todolist 桌面摆件应用

## 1. 实体定义

### 1.1 Todo（待办事项）

**表名**: `todos`

| 字段名 | 类型 | 约束 | 描述 |
|--------|------|------|------|
| `id` | UUID | PRIMARY KEY, NOT NULL | 待办事项唯一标识 |
| `title` | VARCHAR(200) | NOT NULL | 待办事项标题 |
| `description` | TEXT | NULLABLE | 待办事项详细描述 |
| `priority` | INTEGER | NOT NULL, DEFAULT 2 | 优先级：1=低, 2=中, 3=高, 4=紧急 |
| `category_id` | UUID | FOREIGN KEY, NULLABLE | 分类 ID，关联 categories 表 |
| `status` | INTEGER | NOT NULL, DEFAULT 1 | 状态：1=待处理, 2=进行中, 3=已完成, 4=已取消 |
| `due_date` | DATETIME | NULLABLE | 截止日期时间 |
| `created_at` | DATETIME | NOT NULL | 创建时间 |
| `updated_at` | DATETIME | NOT NULL | 更新时间 |
| `completed_at` | DATETIME | NULLABLE | 完成时间 |
| `sync_status` | INTEGER | NOT NULL, DEFAULT 0 | 同步状态：0=未同步, 1=已同步, 2=冲突 |
| `sync_hash` | VARCHAR(64) | NULLABLE | 同步哈希，用于冲突检测 |

**索引**:
- `idx_todos_status`: `status`
- `idx_todos_priority`: `priority`
- `idx_todos_due_date`: `due_date`
- `idx_todos_category_id`: `category_id`
- `idx_todos_created_at`: `created_at`
- `idx_todos_sync_status`: `sync_status`

**验证规则**:
- `title`: 非空，长度 1-200 字符
- `priority`: 必须在 1-4 范围内
- `status`: 必须在 1-4 范围内
- `due_date`: 如果设置，必须晚于当前时间（可选验证）

**状态转换**:
```
PENDING (1) → IN_PROGRESS (2) → COMPLETED (3)
     ↓                ↓                ↓
CANCELLED (4) ←────────┴────────────────┘
```

---

### 1.2 Category（分类）

**表名**: `categories`

| 字段名 | 类型 | 约束 | 描述 |
|--------|------|------|------|
| `id` | UUID | PRIMARY KEY, NOT NULL | 分类唯一标识 |
| `name` | VARCHAR(50) | UNIQUE, NOT NULL | 分类名称 |
| `color` | VARCHAR(7) | NOT NULL, DEFAULT '#3B82F6' | 颜色标识（HEX 格式，如 #RRGGBB） |
| `icon` | VARCHAR(50) | NULLABLE | 图标名称 |
| `created_at` | DATETIME | NOT NULL | 创建时间 |
| `updated_at` | DATETIME | NOT NULL | 更新时间 |

**索引**:
- `idx_categories_name`: `name` (UNIQUE)

**验证规则**:
- `name`: 非空，长度 1-50 字符，唯一
- `color`: 必须匹配 HEX 颜色格式 `^#[0-9A-Fa-f]{6}$`

---

### 1.3 Tag（标签）

**表名**: `tags`

| 字段名 | 类型 | 约束 | 描述 |
|--------|------|------|------|
| `id` | UUID | PRIMARY KEY, NOT NULL | 标签唯一标识 |
| `name` | VARCHAR(30) | UNIQUE, NOT NULL | 标签名称 |
| `color` | VARCHAR(7) | NOT NULL, DEFAULT '#10B981' | 颜色标识 |
| `created_at` | DATETIME | NOT NULL | 创建时间 |

**索引**:
- `idx_tags_name`: `name` (UNIQUE)

**验证规则**:
- `name`: 非空，长度 1-30 字符，唯一

---

### 1.4 TodoTag（待办事项-标签关联）

**表名**: `todo_tags`

| 字段名 | 类型 | 约束 | 描述 |
|--------|------|------|------|
| `todo_id` | UUID | PRIMARY KEY, FOREIGN KEY, NOT NULL | 待办事项 ID |
| `tag_id` | UUID | PRIMARY KEY, FOREIGN KEY, NOT NULL | 标签 ID |
| `created_at` | DATETIME | NOT NULL | 创建时间 |

**索引**:
- `idx_todo_tags_todo_id`: `todo_id`
- `idx_todo_tags_tag_id`: `tag_id`

---

### 1.5 Config（配置）

**表名**: `config`

| 字段名 | 类型 | 约束 | 描述 |
|--------|------|------|------|
| `id` | INTEGER | PRIMARY KEY, DEFAULT 1 | 配置 ID（单条记录，始终为 1） |
| `theme` | VARCHAR(20) | NOT NULL, DEFAULT 'light' | 主题名称：light/dark/custom |
| `auto_sync` | BOOLEAN | NOT NULL, DEFAULT true | 是否自动同步 |
| `sync_interval` | INTEGER | NOT NULL, DEFAULT 30 | 自动同步间隔（分钟） |
| `github_token` | VARCHAR(200) | NULLABLE | GitHub Token（加密存储） |
| `github_repo` | VARCHAR(200) | NULLABLE | GitHub 仓库地址 |
| `github_branch` | VARCHAR(100) | NOT NULL, DEFAULT 'main' | GitHub 分支名称 |
| `window_opacity` | FLOAT | NOT NULL, DEFAULT 0.9 | 窗口透明度（0.5-1.0） |
| `always_on_top` | BOOLEAN | NOT NULL, DEFAULT true | 窗口是否置顶 |
| `reminder_enabled` | BOOLEAN | NOT NULL, DEFAULT true | 是否启用提醒 |
| `reminder_advance_minutes` | INTEGER | NOT NULL, DEFAULT 60 | 提前提醒时间（分钟） |
| `window_x` | INTEGER | NULLABLE | 窗口位置 X 坐标 |
| `window_y` | INTEGER | NULLABLE | 窗口位置 Y 坐标 |
| `window_width` | INTEGER | NOT NULL, DEFAULT 400 | 窗口宽度 |
| `window_height` | INTEGER | NOT NULL, DEFAULT 600 | 窗口高度 |
| `created_at` | DATETIME | NOT NULL | 创建时间 |
| `updated_at` | DATETIME | NOT NULL | 更新时间 |

**验证规则**:
- `theme`: 必须是 'light', 'dark', 'custom' 之一
- `sync_interval`: 必须 >= 5 分钟
- `window_opacity`: 必须在 0.5-1.0 范围内
- `reminder_advance_minutes`: 必须 >= 0

---

### 1.6 SyncLog（同步日志）

**表名**: `sync_logs`

| 字段名 | 类型 | 约束 | 描述 |
|--------|------|------|------|
| `id` | UUID | PRIMARY KEY, NOT NULL | 日志唯一标识 |
| `sync_type` | VARCHAR(20) | NOT NULL | 同步类型：manual/auto |
| `direction` | VARCHAR(20) | NOT NULL | 同步方向：push/pull/both |
| `status` | VARCHAR(20) | NOT NULL | 状态：success/failed/conflict |
| `message` | TEXT | NULLABLE | 同步消息/错误信息 |
| `changes_count` | INTEGER | NOT NULL, DEFAULT 0 | 变更数量 |
| `started_at` | DATETIME | NOT NULL | 开始时间 |
| `completed_at` | DATETIME | NULLABLE | 完成时间 |

**索引**:
- `idx_sync_logs_status`: `status`
- `idx_sync_logs_started_at`: `started_at`

---

## 2. 实体关系图

```
┌─────────────┐       ┌─────────────┐       ┌─────────────┐
│   Todo      │       │  Category   │       │    Tag      │
├─────────────┤       ├─────────────┤       ├─────────────┤
│ id (PK)     │       │ id (PK)     │       │ id (PK)     │
│ title       │       │ name        │       │ name        │
│ description │       │ color       │       │ color       │
│ priority    │       │ icon        │       │ created_at  │
│ status      │       │ created_at  │       └─────────────┘
│ due_date    │       │ updated_at  │              ▲
│ category_id │───┐   └─────────────┘              │
│ created_at  │   │                                  │
│ updated_at  │   │                                  │
│ completed_at│   │                                  │
│ sync_status │   │                                  │
│ sync_hash   │   │                                  │
└─────────────┘   │                                  │
        │         │                                  │
        │         └── category_id (FK)               │
        │                                            │
        ▼                                            │
┌─────────────┐                                     │
│  TodoTag    │                                     │
├─────────────┤                                     │
│ todo_id (PK)│─────────────────────────────────────┘
│ tag_id (PK) │
│ created_at  │
└─────────────┘

┌─────────────┐       ┌─────────────┐
│   Config    │       │  SyncLog    │
├─────────────┤       ├─────────────┤
│ id (PK)     │       │ id (PK)     │
│ theme       │       │ sync_type   │
│ auto_sync   │       │ direction   │
│ ...         │       │ status      │
│ created_at  │       │ message     │
│ updated_at  │       │ changes_count│
└─────────────┘       │ started_at  │
                      │ completed_at│
                      └─────────────┘
```

---

## 3. JSON 序列化格式（GitHub 备份）

### 3.1 备份文件结构

```
todolist-backup/
├── todos.json          # 所有待办事项
├── categories.json     # 所有分类
├── tags.json           # 所有标签
├── config.json         # 配置信息（不含敏感数据）
└── metadata.json       # 备份元数据
```

### 3.2 todos.json 格式

```json
{
  "version": "1.0",
  "exported_at": "2026-06-18T12:00:00Z",
  "todos": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "title": "完成项目设计文档",
      "description": "编写详细的技术设计方案",
      "priority": 3,
      "category_id": "550e8400-e29b-41d4-a716-446655440001",
      "status": 2,
      "due_date": "2026-06-20T18:00:00Z",
      "created_at": "2026-06-18T10:00:00Z",
      "updated_at": "2026-06-18T11:30:00Z",
      "completed_at": null,
      "tags": ["工作", "重要"]
    }
  ]
}
```

### 3.3 metadata.json 格式

```json
{
  "version": "1.0",
  "backup_id": "550e8400-e29b-41d4-a716-446655440002",
  "created_at": "2026-06-18T12:00:00Z",
  "client_version": "1.0.0",
  "todos_count": 42,
  "categories_count": 5,
  "tags_count": 10,
  "hash": "sha256:abcdef1234567890..."
}
```

---

## 4. 数据库迁移

### 4.1 初始迁移 (v1)

```sql
-- 创建 todos 表
CREATE TABLE IF NOT EXISTS todos (
    id TEXT PRIMARY KEY,
    title TEXT NOT NULL CHECK (length(title) BETWEEN 1 AND 200),
    description TEXT,
    priority INTEGER NOT NULL DEFAULT 2 CHECK (priority BETWEEN 1 AND 4),
    category_id TEXT,
    status INTEGER NOT NULL DEFAULT 1 CHECK (status BETWEEN 1 AND 4),
    due_date TEXT,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    completed_at TEXT,
    sync_status INTEGER NOT NULL DEFAULT 0 CHECK (sync_status BETWEEN 0 AND 2),
    sync_hash TEXT,
    FOREIGN KEY (category_id) REFERENCES categories(id)
);

-- 创建 categories 表
CREATE TABLE IF NOT EXISTS categories (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL UNIQUE CHECK (length(name) BETWEEN 1 AND 50),
    color TEXT NOT NULL DEFAULT '#3B82F6' CHECK (color REGEXP '^#[0-9A-Fa-f]{6}$'),
    icon TEXT,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

-- 创建 tags 表
CREATE TABLE IF NOT EXISTS tags (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL UNIQUE CHECK (length(name) BETWEEN 1 AND 30),
    color TEXT NOT NULL DEFAULT '#10B981' CHECK (color REGEXP '^#[0-9A-Fa-f]{6}$'),
    created_at TEXT NOT NULL
);

-- 创建 todo_tags 关联表
CREATE TABLE IF NOT EXISTS todo_tags (
    todo_id TEXT NOT NULL,
    tag_id TEXT NOT NULL,
    created_at TEXT NOT NULL,
    PRIMARY KEY (todo_id, tag_id),
    FOREIGN KEY (todo_id) REFERENCES todos(id) ON DELETE CASCADE,
    FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
);

-- 创建 config 表
CREATE TABLE IF NOT EXISTS config (
    id INTEGER PRIMARY KEY DEFAULT 1 CHECK (id = 1),
    theme TEXT NOT NULL DEFAULT 'light' CHECK (theme IN ('light', 'dark', 'custom')),
    auto_sync INTEGER NOT NULL DEFAULT 1,
    sync_interval INTEGER NOT NULL DEFAULT 30 CHECK (sync_interval >= 5),
    github_token TEXT,
    github_repo TEXT,
    github_branch TEXT NOT NULL DEFAULT 'main',
    window_opacity REAL NOT NULL DEFAULT 0.9 CHECK (window_opacity BETWEEN 0.5 AND 1.0),
    always_on_top INTEGER NOT NULL DEFAULT 1,
    reminder_enabled INTEGER NOT NULL DEFAULT 1,
    reminder_advance_minutes INTEGER NOT NULL DEFAULT 60 CHECK (reminder_advance_minutes >= 0),
    window_x INTEGER,
    window_y INTEGER,
    window_width INTEGER NOT NULL DEFAULT 400,
    window_height INTEGER NOT NULL DEFAULT 600,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

-- 创建 sync_logs 表
CREATE TABLE IF NOT EXISTS sync_logs (
    id TEXT PRIMARY KEY,
    sync_type TEXT NOT NULL,
    direction TEXT NOT NULL,
    status TEXT NOT NULL,
    message TEXT,
    changes_count INTEGER NOT NULL DEFAULT 0,
    started_at TEXT NOT NULL,
    completed_at TEXT
);

-- 创建索引
CREATE INDEX IF NOT EXISTS idx_todos_status ON todos(status);
CREATE INDEX IF NOT EXISTS idx_todos_priority ON todos(priority);
CREATE INDEX IF NOT EXISTS idx_todos_due_date ON todos(due_date);
CREATE INDEX IF NOT EXISTS idx_todos_category_id ON todos(category_id);
CREATE INDEX IF NOT EXISTS idx_todos_created_at ON todos(created_at);
CREATE INDEX IF NOT EXISTS idx_todos_sync_status ON todos(sync_status);
CREATE INDEX IF NOT EXISTS idx_categories_name ON categories(name);
CREATE INDEX IF NOT EXISTS idx_tags_name ON tags(name);
CREATE INDEX IF NOT EXISTS idx_todo_tags_todo_id ON todo_tags(todo_id);
CREATE INDEX IF NOT EXISTS idx_todo_tags_tag_id ON todo_tags(tag_id);
CREATE INDEX IF NOT EXISTS idx_sync_logs_status ON sync_logs(status);
CREATE INDEX IF NOT EXISTS idx_sync_logs_started_at ON sync_logs(started_at);

-- 插入默认配置
INSERT OR IGNORE INTO config (id, created_at, updated_at) VALUES (1, datetime('now'), datetime('now'));
```

---

## 5. C++ 数据模型类

### 5.1 Todo.h

```cpp
#pragma once

#include <QDateTime>
#include <QString>
#include <QUuid>
#include <QVector>

enum class Priority : int {
    Low = 1,
    Medium = 2,
    High = 3,
    Urgent = 4
};

enum class TodoStatus : int {
    Pending = 1,
    InProgress = 2,
    Completed = 3,
    Cancelled = 4
};

enum class SyncStatus : int {
    NotSynced = 0,
    Synced = 1,
    Conflict = 2
};

class Todo {
public:
    Todo();
    
    QUuid id() const;
    void setId(const QUuid& id);
    
    QString title() const;
    void setTitle(const QString& title);
    
    QString description() const;
    void setDescription(const QString& description);
    
    Priority priority() const;
    void setPriority(Priority priority);
    
    QUuid categoryId() const;
    void setCategoryId(const QUuid& categoryId);
    
    TodoStatus status() const;
    void setStatus(TodoStatus status);
    
    QDateTime dueDate() const;
    void setDueDate(const QDateTime& dueDate);
    
    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& createdAt);
    
    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime& updatedAt);
    
    QDateTime completedAt() const;
    void setCompletedAt(const QDateTime& completedAt);
    
    SyncStatus syncStatus() const;
    void setSyncStatus(SyncStatus syncStatus);
    
    QString syncHash() const;
    void setSyncHash(const QString& syncHash);
    
    QVector<QString> tags() const;
    void setTags(const QVector<QString>& tags);
    void addTag(const QString& tag);
    void removeTag(const QString& tag);
    
    bool isValid() const;
    QString calculateHash() const;
    
private:
    QUuid m_id;
    QString m_title;
    QString m_description;
    Priority m_priority;
    QUuid m_categoryId;
    TodoStatus m_status;
    QDateTime m_dueDate;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    QDateTime m_completedAt;
    SyncStatus m_syncStatus;
    QString m_syncHash;
    QVector<QString> m_tags;
};
```

### 5.2 Category.h

```cpp
#pragma once

#include <QDateTime>
#include <QString>
#include <QUuid>

class Category {
public:
    Category();
    
    QUuid id() const;
    void setId(const QUuid& id);
    
    QString name() const;
    void setName(const QString& name);
    
    QString color() const;
    void setColor(const QString& color);
    
    QString icon() const;
    void setIcon(const QString& icon);
    
    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& createdAt);
    
    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime& updatedAt);
    
    bool isValid() const;
    
private:
    QUuid m_id;
    QString m_name;
    QString m_color;
    QString m_icon;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
};
```

### 5.3 Config.h

```cpp
#pragma once

#include <QDateTime>
#include <QString>

class Config {
public:
    Config();
    
    QString theme() const;
    void setTheme(const QString& theme);
    
    bool autoSync() const;
    void setAutoSync(bool autoSync);
    
    int syncInterval() const;
    void setSyncInterval(int syncInterval);
    
    QString githubToken() const;
    void setGithubToken(const QString& githubToken);
    
    QString githubRepo() const;
    void setGithubRepo(const QString& githubRepo);
    
    QString githubBranch() const;
    void setGithubBranch(const QString& githubBranch);
    
    float windowOpacity() const;
    void setWindowOpacity(float windowOpacity);
    
    bool alwaysOnTop() const;
    void setAlwaysOnTop(bool alwaysOnTop);
    
    bool reminderEnabled() const;
    void setReminderEnabled(bool reminderEnabled);
    
    int reminderAdvanceMinutes() const;
    void setReminderAdvanceMinutes(int minutes);
    
    int windowX() const;
    void setWindowX(int x);
    
    int windowY() const;
    void setWindowY(int y);
    
    int windowWidth() const;
    void setWindowWidth(int width);
    
    int windowHeight() const;
    void setWindowHeight(int height);
    
    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& createdAt);
    
    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime& updatedAt);
    
    bool isValid() const;
    
private:
    QString m_theme;
    bool m_autoSync;
    int m_syncInterval;
    QString m_githubToken;
    QString m_githubRepo;
    QString m_githubBranch;
    float m_windowOpacity;
    bool m_alwaysOnTop;
    bool m_reminderEnabled;
    int m_reminderAdvanceMinutes;
    int m_windowX;
    int m_windowY;
    int m_windowWidth;
    int m_windowHeight;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
};
```

<!-- cli_version: 0.1.41 -->
