# Interface Contract: QML/C++ Bridge

## 1. 概述

本文档定义了 QML 表现层与 C++ 业务逻辑层之间的接口契约。所有 QML/C++ 交互必须通过本文档定义的接口进行。

## 2. Context Properties 注册

### 2.1 注册时机

在 `main.cpp` 中，在加载 QML 之前注册所有 Context Properties。

```cpp
// main.cpp
QQmlApplicationEngine engine;

// 注册服务实例
engine.rootContext()->setContextProperty("todoService", &todoService);
engine.rootContext()->setContextProperty("syncService", &syncService);
engine.rootContext()->setContextProperty("reminderService", &reminderService);
engine.rootContext()->setContextProperty("statsService", &statsService);
engine.rootContext()->setContextProperty("themeService", &themeService);
engine.rootContext()->setContextProperty("configService", &configService);

// 注册枚举类型
qmlRegisterUncreatableType<Priority>("TodoApp", 1, 0, "Priority", "Cannot create Priority enum");
qmlRegisterUncreatableType<TodoStatus>("TodoApp", 1, 0, "TodoStatus", "Cannot create TodoStatus enum");
qmlRegisterUncreatableType<SyncStatus>("TodoApp", 1, 0, "SyncStatus", "Cannot create SyncStatus enum");

// 注册数据模型
qmlRegisterType<Todo>("TodoApp", 1, 0, "Todo");
qmlRegisterType<Category>("TodoApp", 1, 0, "Category");
qmlRegisterType<Config>("TodoApp", 1, 0, "Config");
```

### 2.2 命名规范

- Context Property 名称使用 lowerCamelCase
- 枚举类型使用 UpperCamelCase
- 注册的 QML 类型使用 UpperCamelCase
- 属性名称使用 lowerCamelCase
- 信号名称使用 lowerCamelCase
- 方法名称使用 lowerCamelCase

## 3. TodoService 接口

### 3.1 属性

| 属性名 | 类型 | 可读 | 可写 | 通知信号 | 描述 |
|---------|------|------|------|------------|------|
| `todos` | `QList<Todo*>` | 是 | 否 | `todosChanged` | 所有待办事项列表 |
| `categories` | `QList<Category*>` | 是 | 否 | `categoriesChanged` | 所有分类列表 |
| `filterStatus` | `TodoStatus` | 是 | 是 | `filterStatusChanged` | 当前过滤的状态 |
| `filterCategory` | `QUuid` | 是 | 是 | `filterCategoryChanged` | 当前过滤的分类 |
| `searchQuery` | `QString` | 是 | 是 | `searchQueryChanged` | 当前搜索关键词 |
| `sortBy` | `QString` | 是 | 是 | `sortByChanged` | 排序字段 |
| `sortOrder` | `Qt::SortOrder` | 是 | 是 | `sortOrderChanged` | 排序方向 |

### 3.2 方法

| 方法名 | 参数 | 返回值 | 描述 |
|---------|------|--------|------|
| `createTodo` | `title: QString`, `description: QString`, `priority: Priority`, `categoryId: QUuid`, `dueDate: QDateTime` | `QUuid` | 创建新待办事项，返回新创建的 ID |
| `updateTodo` | `todoId: QUuid`, `title: QString`, `description: QString`, `priority: Priority`, `categoryId: QUuid`, `dueDate: QDateTime`, `status: TodoStatus` | `bool` | 更新待办事项 |
| `deleteTodo` | `todoId: QUuid` | `bool` | 删除待办事项 |
| `completeTodo` | `todoId: QUuid` | `bool` | 标记待办事项为已完成 |
| `uncompleteTodo` | `todoId: QUuid` | `bool` | 标记待办事项为未完成 |
| `getTodo` | `todoId: QUuid` | `Todo*` | 根据 ID 获取待办事项 |
| `createCategory` | `name: QString`, `color: QString`, `icon: QString` | `QUuid` | 创建新分类 |
| `updateCategory` | `categoryId: QUuid`, `name: QString`, `color: QString`, `icon: QString` | `bool` | 更新分类 |
| `deleteCategory` | `categoryId: QUuid` | `bool` | 删除分类 |
| `addTagToTodo` | `todoId: QUuid`, `tag: QString` | `bool` | 为待办事项添加标签 |
| `removeTagFromTodo` | `todoId: QUuid`, `tag: QString` | `bool` | 从待办事项移除标签 |
| `getAllTags` | 无 | `QStringList` | 获取所有标签 |
| `refresh` | 无 | `void` | 刷新数据 |

### 3.3 信号

| 信号名 | 参数 | 描述 |
|---------|------|------|
| `todoCreated` | `todoId: QUuid` | 待办事项创建成功 |
| `todoUpdated` | `todoId: QUuid` | 待办事项更新成功 |
| `todoDeleted` | `todoId: QUuid` | 待办事项删除成功 |
| `todoCompleted` | `todoId: QUuid` | 待办事项标记为完成 |
| `errorOccurred` | `message: QString` | 发生错误 |

## 4. SyncService 接口

### 4.1 属性

| 属性名 | 类型 | 可读 | 可写 | 通知信号 | 描述 |
|---------|------|------|------|------------|------|
| `isSyncing` | `bool` | 是 | 否 | `isSyncingChanged` | 是否正在同步 |
| `lastSyncTime` | `QDateTime` | 是 | 否 | `lastSyncTimeChanged` | 上次同步时间 |
| `syncStatus` | `QString` | 是 | 否 | `syncStatusChanged` | 同步状态描述 |
| `hasPendingChanges` | `bool` | 是 | 否 | `hasPendingChangesChanged` | 是否有待同步的变更 |
| `isConfigured` | `bool` | 是 | 否 | `isConfiguredChanged` | GitHub 是否已配置 |

### 4.2 方法

| 方法名 | 参数 | 返回值 | 描述 |
|---------|------|--------|------|
| `syncNow` | 无 | `void` | 立即执行同步 |
| `configureGitHub` | `token: QString`, `repo: QString`, `branch: QString` | `bool` | 配置 GitHub |
| `testConnection` | 无 | `bool` | 测试 GitHub 连接 |
| `pushChanges` | 无 | `bool` | 推送本地变更到 GitHub |
| `pullChanges` | 无 | `bool` | 从 GitHub 拉取变更 |
| `resolveConflict` | `todoId: QUuid`, `useLocal: bool` | `bool` | 解决冲突 |
| `clearConfiguration` | 无 | `void` | 清除 GitHub 配置 |

### 4.3 信号

| 信号名 | 参数 | 描述 |
|---------|------|------|
| `syncStarted` | 无 | 同步开始 |
| `syncCompleted` | `success: bool`, `message: QString` | 同步完成 |
| `syncProgress` | `progress: int`, `message: QString` | 同步进度 |
| `conflictDetected` | `todoId: QUuid`, `localVersion: Todo*`, `remoteVersion: Todo*` | 检测到冲突 |
| `syncError` | `message: QString` | 同步错误 |

## 5. ReminderService 接口

### 5.1 属性

| 属性名 | 类型 | 可读 | 可写 | 通知信号 | 描述 |
|---------|------|------|------|------------|------|
| `isEnabled` | `bool` | 是 | 是 | `isEnabledChanged` | 是否启用提醒 |
| `upcomingReminders` | `QList<Todo*>` | 是 | 否 | `upcomingRemindersChanged` | 即将到期的待办事项 |

### 5.2 方法

| 方法名 | 参数 | 返回值 | 描述 |
|---------|------|--------|------|
| `dismissReminder` | `todoId: QUuid` | `void` | 关闭提醒 |
| `snoozeReminder` | `todoId: QUuid`, `minutes: int` | `void` | 延后提醒 |
| `checkUpcoming` | 无 | `void` | 检查即将到期的待办事项 |

### 5.3 信号

| 信号名 | 参数 | 描述 |
|---------|------|------|
| `reminderTriggered` | `todo: Todo*` | 触发提醒 |

## 6. StatsService 接口

### 6.1 属性

| 属性名 | 类型 | 可读 | 可写 | 通知信号 | 描述 |
|---------|------|------|------|------------|------|
| `totalTodos` | `int` | 是 | 否 | `totalTodosChanged` | 总待办事项数 |
| `completedTodos` | `int` | 是 | 否 | `completedTodosChanged` | 已完成待办事项数 |
| `completionRate` | `double` | 是 | 否 | `completionRateChanged` | 完成率 (0.0-1.0) |
| `pendingTodos` | `int` | 是 | 否 | `pendingTodosChanged` | 待处理待办事项数 |
| `overdueTodos` | `int` | 是 | 否 | `overdueTodosChanged` | 已过期待办事项数 |
| `categoryStats` | `QVariantMap` | 是 | 否 | `categoryStatsChanged` | 分类统计 |
| `dailyCompletionTrend` | `QVariantList` | 是 | 否 | `dailyCompletionTrendChanged` | 每日完成趋势（最近 7 天） |

### 6.2 方法

| 方法名 | 参数 | 返回值 | 描述 |
|---------|------|--------|------|
| `refreshStats` | 无 | `void` | 刷新统计数据 |
| `getStatsForDateRange` | `startDate: QDate`, `endDate: QDate` | `QVariantMap` | 获取指定日期范围的统计 |

## 7. ThemeService 接口

### 7.1 属性

| 属性名 | 类型 | 可读 | 可写 | 通知信号 | 描述 |
|---------|------|------|------|------------|------|
| `currentTheme` | `QString` | 是 | 是 | `currentThemeChanged` | 当前主题名称 |
| `availableThemes` | `QStringList` | 是 | 否 | `availableThemesChanged` | 可用主题列表 |
| `isDarkMode` | `bool` | 是 | 否 | `isDarkModeChanged` | 是否为深色模式 |
| `accentColor` | `QColor` | 是 | 是 | `accentColorChanged` | 强调色 |
| `backgroundColor` | `QColor` | 是 | 是 | `backgroundColorChanged` | 背景色 |
| `textColor` | `QColor` | 是 | 是 | `textColorChanged` | 文字颜色 |

### 7.2 方法

| 方法名 | 参数 | 返回值 | 描述 |
|---------|------|--------|------|
| `setTheme` | `themeName: QString` | `bool` | 设置主题 |
| `createCustomTheme` | `themeName: QString`, `colors: QVariantMap` | `bool` | 创建自定义主题 |
| `deleteCustomTheme` | `themeName: QString` | `bool` | 删除自定义主题 |
| `exportTheme` | `themeName: QString`, `filePath: QString` | `bool` | 导出主题 |
| `importTheme` | `filePath: QString` | `QString` | 导入主题，返回主题名称 |

### 7.3 信号

| 信号名 | 参数 | 描述 |
|---------|------|------|
| `themeChanged` | `themeName: QString` | 主题已更改 |

## 8. ConfigService 接口

### 8.1 属性

所有配置属性均可读可写，变更时发出对应信号。

### 8.2 方法

| 方法名 | 参数 | 返回值 | 描述 |
|---------|------|--------|------|
| `save` | 无 | `bool` | 保存配置 |
| `resetToDefaults` | 无 | `void` | 重置为默认配置 |

## 9. 数据类型约定

### 9.1 日期时间格式

- 所有日期时间使用 ISO 8601 格式：`yyyy-MM-ddTHH:mm:ssZ`
- QML 中使用 `Date` 类型
- C++ 中使用 `QDateTime` 类型

### 9.2 UUID 格式

- 所有 ID 使用 QUuid 类型
- 序列化为字符串时使用不带大驼峰格式：`{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}`

### 9.3 颜色格式

- 颜色使用 `#RRGGBB` 格式的字符串
- QML 中可以直接使用
- C++ 中使用 `QColor` 类型

## 10. 错误处理约定

### 10.1 错误码

| 错误码 | 描述 |
|---------|------|
| `NoError` | 无错误 |
| `InvalidInput` | 输入无效 |
| `NotFound` | 资源未找到 |
| `PermissionDenied` | 权限不足 |
| `NetworkError` | 网络错误 |
| `DatabaseError` | 数据库错误 |
| `Conflict` | 数据冲突 |
| `UnknownError` | 未知错误 |

### 10.2 错误通知

- 所有异步操作通过信号通知错误
- 错误信息使用用户友好的中文描述
- 详细错误信息记录到日志

<!-- cli_version: 0.1.41 -->
