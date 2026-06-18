# Interface Contract: Data Persistence

## 1. 概述

本文档定义了数据持久化层的接口契约。所有数据持久化操作必须遵循本文档定义的规范。

## 2. 数据库设计规范

### 2.1 表命名规范

- 使用小写字母和下划线
- 使用复数形式表示表名
- 前缀：无（单数据库场景）
- 示例：`todos`, `categories`, `todo_tags`

### 2.2 列命名规范

- 使用小写字母和下划线
- 布尔类型使用 `is_` 或 `has_` 前缀
- 时间类型使用 `_at` 后缀
- 外键使用 `_id` 后缀
- 示例：`title`, `is_completed`, `created_at`, `category_id`

### 2.3 主键规范

- 所有表使用 UUID 作为主键
- 主键列名统一为 `id`
- UUID 版本：v4（随机 UUID）
- 存储格式：TEXT（36 字符字符串）

### 2.4 索引规范

- 主键自动创建索引
- 外键必须创建索引
- 常用查询字段创建索引
- 唯一约束字段创建唯一索引
- 索引命名：`idx_<table>_<column>`

### 2.5 约束规范

- 非空约束：`NOT NULL` 适用于必填字段
- 唯一约束：`UNIQUE` 适用于需要唯一的字段
- 检查约束：`CHECK` 适用于需要范围限制的字段
- 外键约束：`FOREIGN KEY` 适用于关联字段
- 默认值：`DEFAULT` 适用于有默认值的字段

## 3. SQLite 特定规范

### 3.1 数据类型映射

| C++ 类型 | SQLite 类型 | 存储格式 |
|----------|------------|----------|
| `QUuid` | `TEXT` | UUID 字符串 |
| `QString` | `TEXT` | UTF-8 字符串 |
| `int` / `enum` | `INTEGER` | 64 位整数 |
| `float` / `double` | `REAL` | 64 位浮点数 |
| `bool` | `INTEGER` | 0 或 1 |
| `QDateTime` | `TEXT` | ISO 8601 格式 |
| `QByteArray` | `BLOB` | 二进制数据 |

### 3.2 日期时间存储

- 使用 ISO 8601 格式存储日期时间
- 格式：`yyyy-MM-ddTHH:mm:ssZ`
- 时区：UTC（避免时区问题）
- 示例：`2026-06-18T12:00:00Z`

### 3.3 布尔值存储

- 使用 INTEGER 类型存储布尔值
- `true` 存储为 `1`
- `false` 存储为 `0`
- 查询时使用 `WHERE is_enabled = 1`

### 3.4 枚举存储

- 使用 INTEGER 类型存储枚举值
- 枚举值从 1 开始（避免 0 与 NULL 混淆）
- 示例：`Priority::Low = 1`, `Priority::Medium = 2`

## 4. 数据库连接管理

### 4.1 连接参数

```cpp
// 数据库文件路径
QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
                 + "/todolist.db";

// 连接名称
QString connectionName = "todolist_connection";

// 连接选项
QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
db.setDatabaseName(dbPath);
db.setConnectOptions("QSQLITE_ENABLE_FK=ON;QSQLITE_JOURNAL_MODE=WAL");
```

### 4.2 连接池

- 单线程应用使用单一连接
- 多线程场景使用连接池
- 每个线程使用独立的连接
- 连接在使用完毕后正确关闭

### 4.3 事务管理

```cpp
// 开启事务
db.transaction();

try {
    // 执行数据库操作
    executeQuery1();
    executeQuery2();
    
    // 提交事务
    db.commit();
} catch (const DatabaseException& e) {
    // 回滚事务
    db.rollback();
    throw;
}
```

**事务使用场景**:
- 多个相关的写操作
- 需要原子性的操作
- 批量数据导入
- 冲突解决流程

## 5. 查询规范

### 5.1 参数化查询

**必须使用参数化查询，禁止字符串拼接 SQL**

✅ 正确：
```cpp
QSqlQuery query(db);
query.prepare("SELECT * FROM todos WHERE status = :status AND priority = :priority");
query.bindValue(":status", static_cast<int>(TodoStatus::Pending));
query.bindValue(":priority", static_cast<int>(Priority::High));
query.exec();
```

❌ 错误：
```cpp
// 禁止！存在 SQL 注入风险
QString sql = QString("SELECT * FROM todos WHERE status = %1").arg(status);
query.exec(sql);
```

### 5.2 查询性能优化

1. **使用索引**:
   - WHERE 子句中的字段应有索引
   - ORDER BY 字段应有索引
   - JOIN 字段应有索引

2. **限制返回行数**:
   ```cpp
   query.prepare("SELECT * FROM todos ORDER BY created_at DESC LIMIT :limit OFFSET :offset");
   ```

3. **只选择需要的列**:
   ```cpp
   // 好
   SELECT id, title, status FROM todos
   
   // 避免
   SELECT * FROM todos
   ```

4. **使用预编译语句**:
   - 重复执行的查询使用 `prepare()` 一次，多次 `exec()`

### 5.3 分页查询

```cpp
// 每页 20 条，获取第 3 页
int pageSize = 20;
int pageNumber = 3;
int offset = (pageNumber - 1) * pageSize;

query.prepare("SELECT * FROM todos ORDER BY created_at DESC LIMIT :limit OFFSET :offset");
query.bindValue(":limit", pageSize);
query.bindValue(":offset", offset);
```

## 6. 错误处理

### 6.1 异常类

```cpp
class DatabaseException : public std::runtime_error {
public:
    enum class ErrorCode {
        ConnectionError,
        QueryError,
        ConstraintViolation,
        TransactionError,
        MigrationError,
        UnknownError
    };
    
    DatabaseException(ErrorCode code, const QString& message)
        : std::runtime_error(message.toStdString()), m_code(code) {}
    
    ErrorCode code() const { return m_code; }
    
private:
    ErrorCode m_code;
};
```

### 6.2 错误检查

```cpp
QSqlQuery query(db);
if (!query.prepare(sql)) {
    throw DatabaseException(
        DatabaseException::ErrorCode::QueryError,
        QString("Failed to prepare query: %1").arg(query.lastError().text())
    );
}

if (!query.exec()) {
    throw DatabaseException(
        DatabaseException::ErrorCode::QueryError,
        QString("Query execution failed: %1").arg(query.lastError().text())
    );
}
```

### 6.3 常见错误处理

| 错误类型 | 处理策略 |
|----------|----------|
| 数据库文件损坏 | 尝试从备份恢复，提示用户 |
| 磁盘空间不足 | 提示用户清理磁盘空间 |
| 权限不足 | 提示用户检查目录权限 |
| 锁超时 | 重试操作，最多 3 次 |
| 外键约束违反 | 提示用户存在关联数据 |
| 唯一约束违反 | 提示用户数据已存在 |

## 7. 数据库迁移

### 7.1 迁移版本管理

```sql
-- 迁移版本表
CREATE TABLE IF NOT EXISTS schema_migrations (
    version INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    applied_at TEXT NOT NULL
);
```

### 7.2 迁移脚本示例

**v1__initial_schema.sql**:
```sql
-- 初始数据库 schema
CREATE TABLE IF NOT EXISTS todos (...);
CREATE TABLE IF NOT EXISTS categories (...);
-- ... 其他表

INSERT INTO schema_migrations (version, name, applied_at)
VALUES (1, 'initial_schema', datetime('now'));
```

**v2__add_tags_table.sql**:
```sql
-- 添加标签功能
CREATE TABLE IF NOT EXISTS tags (...);
CREATE TABLE IF NOT EXISTS todo_tags (...);

INSERT INTO schema_migrations (version, name, applied_at)
VALUES (2, 'add_tags_table', datetime('now'));
```

### 7.3 迁移执行流程

```
应用启动
    │
    ▼
检查 schema_migrations 表
    │
    ├─► 不存在 → 执行所有迁移脚本
    │
    └─► 存在 → 检查已应用的版本
              │
              ▼
        比较迁移脚本版本
              │
              ├─► 有新版本 → 按顺序执行
              │
              └─► 无新版本 → 继续启动
```

## 8. 备份与恢复

### 8.1 数据库备份

```cpp
bool DatabaseManager::backup(const QString& backupPath) {
    QSqlDatabase db = QSqlDatabase::database();
    
    // 使用 SQLite .backup 命令
    QSqlQuery query(db);
    QString backupSql = QString("VACUUM INTO '%1'").arg(backupPath);
    
    if (!query.exec(backupSql)) {
        qCritical() << "Backup failed:" << query.lastError().text();
        return false;
    }
    
    return true;
}
```

### 8.2 数据库恢复

```cpp
bool DatabaseManager::restore(const QString& backupPath) {
    // 关闭当前连接
    QSqlDatabase::removeDatabase(connectionName);
    
    // 复制备份文件到数据库位置
    if (!QFile::copy(backupPath, dbPath)) {
        qCritical() << "Failed to copy backup file";
        return false;
    }
    
    // 重新打开连接
    return initialize();
}
```

### 8.3 自动备份策略

- 每日自动备份（凌晨 3 点）
- 保留最近 7 天的备份
- 备份文件命名：`todolist_YYYYMMDD.db`
- 备份位置：`AppDataLocation/backups/`

## 9. 性能优化

### 9.1 SQLite 性能调优

```sql
-- 启用 WAL 模式（写入性能提升）
PRAGMA journal_mode = WAL;

-- 设置缓存大小（20MB）
PRAGMA cache_size = -20000;

-- 设置同步模式（NORMAL 平衡性能和安全性）
PRAGMA synchronous = NORMAL;

-- 启用外键约束
PRAGMA foreign_keys = ON;

-- 设置自动清理 WAL 文件
PRAGMA wal_autocheckpoint = 1000;
```

### 9.2 查询性能监控

- 记录慢查询（执行时间 > 100ms）
- 使用 `EXPLAIN QUERY PLAN` 分析查询
- 定期使用 `ANALYZE` 更新统计信息

### 9.3 连接性能

- 应用启动时打开数据库连接
- 保持连接打开直到应用退出
- 避免频繁打开/关闭连接

## 10. 安全考虑

### 10.1 敏感数据加密

- GitHub Token 使用 AES-256-GCM 加密存储
- 加密密钥使用操作系统密钥管理服务
- 数据库文件可选加密（SQLCipher）

### 10.2 SQL 注入防护

- 始终使用参数化查询
- 禁止动态拼接 SQL 语句
- 输入验证和转义

### 10.3 数据完整性

- 使用外键约束保证引用完整性
- 使用事务保证操作原子性
- 定期检查数据库完整性（`PRAGMA integrity_check`）

<!-- cli_version: 0.1.41 -->
