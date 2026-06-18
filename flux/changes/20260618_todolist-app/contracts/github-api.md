# Interface Contract: GitHub API Integration

## 1. 概述

本文档定义了与 GitHub API 交互的接口契约。所有 GitHub 相关操作必须遵循本文档定义的规范。

## 2. 认证方式

### 2.1 Personal Access Token (PAT)

使用 GitHub Personal Access Token 进行认证。Token 需要以下权限：

| 权限 | 描述 | 必需 |
|------|------|------|
| `repo` | 完整的仓库访问权限 | 是 |
| `repo:status` | 访问提交状态 | 否 |
| `repo_deployment` | 访问部署状态 | 否 |
| `public_repo` | 访问公开仓库 | 如果使用公开仓库 |

### 2.2 Token 存储

- Token 必须加密存储在本地数据库中
- 加密算法：AES-256-GCM
- 加密密钥：使用操作系统提供的密钥管理服务
  - Windows: DPAPI
  - macOS: Keychain
- 不得在日志、配置文件或错误消息中明文输出 Token

### 2.3 请求头

所有 API 请求必须包含以下请求头：

```http
Authorization: token <GITHUB_TOKEN>
Accept: application/vnd.github.v3+json
User-Agent: Todolist-Desktop-App/1.0
```

## 3. API 端点

### 3.1 获取仓库信息

**GET** `/repos/{owner}/{repo}`

**请求参数**: 无

**响应示例**:
```json
{
  "id": 123456789,
  "name": "todolist-backup",
  "full_name": "username/todolist-backup",
  "private": true,
  "html_url": "https://github.com/username/todolist-backup",
  "default_branch": "main",
  "permissions": {
    "admin": true,
    "push": true,
    "pull": true
  }
}
```

**错误码**:
- `401 Unauthorized`: Token 无效或已过期
- `403 Forbidden`: Token 权限不足
- `404 Not Found`: 仓库不存在或无访问权限

### 3.2 获取分支信息

**GET** `/repos/{owner}/{repo}/branches/{branch}`

**请求参数**: 无

**响应示例**:
```json
{
  "name": "main",
  "commit": {
    "sha": "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0",
    "url": "https://api.github.com/repos/username/todolist-backup/commits/a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0"
  },
  "protected": false
}
```

### 3.3 获取文件内容

**GET** `/repos/{owner}/{repo}/contents/{path}?ref={branch}`

**请求参数**:
- `ref`: 分支名称或 commit SHA

**响应示例**:
```json
{
  "name": "todos.json",
  "path": "todos.json",
  "sha": "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0",
  "size": 12345,
  "encoding": "base64",
  "content": "base64_encoded_content...",
  "html_url": "https://github.com/username/todolist-backup/blob/main/todos.json",
  "download_url": "https://raw.githubusercontent.com/username/todolist-backup/main/todos.json"
}
```

### 3.4 创建或更新文件

**PUT** `/repos/{owner}/{repo}/contents/{path}`

**请求体**:
```json
{
  "message": "Update todos.json",
  "content": "base64_encoded_content...",
  "sha": "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0",
  "branch": "main"
}
```

**字段说明**:
- `message`: Git commit message（必填）
- `content`: Base64 编码的文件内容（必填）
- `sha`: 文件当前的 SHA（更新时必填，创建时省略）
- `branch`: 分支名称（可选，默认使用默认分支）

**响应示例**:
```json
{
  "content": {
    "name": "todos.json",
    "path": "todos.json",
    "sha": "b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1"
  },
  "commit": {
    "sha": "c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2",
    "message": "Update todos.json",
    "author": {
      "name": "Todolist App",
      "email": "app@todolist.local"
    },
    "committer": {
      "name": "Todolist App",
      "email": "app@todolist.local"
    }
  }
}
```

**错误码**:
- `409 Conflict`: 文件已被修改（SHA 不匹配）
- `422 Unprocessable Entity`: 请求参数无效

### 3.5 删除文件

**DELETE** `/repos/{owner}/{repo}/contents/{path}`

**请求体**:
```json
{
  "message": "Delete todos.json",
  "sha": "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0",
  "branch": "main"
}
```

### 3.6 获取提交历史

**GET** `/repos/{owner}/{repo}/commits?path={path}&per_page={count}`

**请求参数**:
- `path`: 文件路径（可选）
- `per_page`: 每页数量，默认 30，最大 100
- `sha`: 分支或 commit SHA（可选）

**响应示例**:
```json
[
  {
    "sha": "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0",
    "commit": {
      "author": {
        "name": "Todolist App",
        "email": "app@todolist.local",
        "date": "2026-06-18T12:00:00Z"
      },
      "message": "Update todos.json"
    },
    "html_url": "https://github.com/username/todolist-backup/commit/a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0"
  }
]
```

### 3.7 验证 Token 权限

**GET** `/user`

用于验证 Token 是否有效以及获取用户信息。

**响应示例**:
```json
{
  "login": "username",
  "id": 12345,
  "name": "User Name",
  "email": "user@example.com",
  "html_url": "https://github.com/username"
}
```

## 4. Git 操作规范

### 4.1 Commit Message 格式

```
<type>(<scope>): <subject>

<body>
```

**Type**:
- `feat`: 新功能
- `fix`: 修复 bug
- `docs`: 文档变更
- `style`: 代码格式调整
- `refactor`: 重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建/工具链相关
- `backup`: 数据备份（本应用专用）

**示例**:
```
backup(todos): Update todos at 2026-06-18 12:00:00

42 todos backed up
- 5 created
- 3 updated
- 1 deleted
```

### 4.2 分支策略

- 默认使用 `main` 分支
- 支持用户自定义分支名称
- 不建议使用复杂的分支策略，保持简单

### 4.3 同步频率

- 手动同步：用户点击同步按钮时立即执行
- 自动同步：根据用户配置的间隔执行（默认 30 分钟）
- 变更后同步：待办事项变更后标记为"待同步"，在下次同步时推送

### 4.4 冲突检测与解决

**冲突检测**:
- 基于文件 SHA 进行冲突检测
- 拉取远程变更后比较本地和远程的文件哈希
- 如果哈希不匹配且本地有未提交的变更，则判定为冲突

**冲突解决策略**:
1. **使用本地版本**: 覆盖远程版本
2. **使用远程版本**: 丢弃本地变更
3. **手动合并**: 展示差异，由用户选择保留哪些内容

**冲突解决流程**:
```
检测到冲突
    │
    ▼
展示本地和远程版本差异
    │
    ▼
用户选择解决策略
    │
    ├─► 使用本地版本 → 推送本地版本
    │
    ├─► 使用远程版本 → 拉取远程版本覆盖本地
    │
    └─► 手动合并 → 用户编辑后提交新版本
```

## 5. 备份文件结构

### 5.1 仓库目录结构

```
todolist-backup/
├── todos.json          # 待办事项数据
├── categories.json     # 分类数据
├── tags.json           # 标签数据
├── config.json         # 配置数据（不含敏感信息）
├── metadata.json       # 备份元数据
└── README.md           # 仓库说明
```

### 5.2 README.md 内容

```markdown
# Todolist Backup

This repository contains backup data for the Todolist desktop application.

## ⚠️ Important

- **Do NOT edit files manually** unless you know what you're doing.
- Manual edits may cause sync conflicts or data corruption.
- Use the Todolist application to manage your todos.

## Structure

- `todos.json`: All todo items
- `categories.json`: Category definitions
- `tags.json`: Tag definitions
- `config.json`: Application configuration (excluding sensitive data)
- `metadata.json`: Backup metadata (version, timestamps, checksums)

## Restore

To restore your data:
1. Open the Todolist application
2. Go to Settings → Backup & Sync
3. Click "Restore from GitHub"
4. Select the backup point you want to restore

## License

This data is for personal use only.
```

## 6. 速率限制

GitHub API 有速率限制：

- 认证用户：5,000 请求/小时
- 未认证用户：60 请求/小时

**限流处理策略**:
1. 检查响应头中的速率限制信息：
   - `X-RateLimit-Limit`: 每小时最大请求数
   - `X-RateLimit-Remaining`: 剩余请求数
   - `X-RateLimit-Reset`: 重置时间戳（Unix 时间）
2. 如果剩余请求数 < 100，延迟后续请求
3. 如果达到限制，等待重置时间后再重试

## 7. 错误处理

### 7.1 重试策略

| 错误码 | 描述 | 重试策略 |
|--------|------|----------|
| `401` | 未授权 | 不重试，提示用户重新配置 Token |
| `403` | 禁止访问 | 不重试，检查权限 |
| `404` | 资源不存在 | 不重试，提示用户检查仓库地址 |
| `409` | 冲突 | 不重试，进入冲突解决流程 |
| `422` | 请求无效 | 不重试，检查请求参数 |
| `429` | 限流 | 指数退避重试，最多 3 次 |
| `5xx` | 服务器错误 | 指数退避重试，最多 3 次 |
| 网络错误 | 连接失败 | 指数退避重试，最多 3 次 |

### 7.2 指数退避算法

```
重试间隔 = min(2^attempt * 1000ms, 30000ms)
```

- 第 1 次重试：等待 1 秒
- 第 2 次重试：等待 2 秒
- 第 3 次重试：等待 4 秒
- 最大等待时间：30 秒

## 8. 安全考虑

1. **Token 安全**:
   - 绝不硬编码 Token
   - 使用操作系统安全存储
   - 定期提醒用户轮换 Token

2. **数据加密**:
   - 敏感配置在上传前加密
   - 可选：整个备份文件加密后再上传

3. **HTTPS**:
   - 所有 API 请求必须使用 HTTPS
   - 验证 SSL 证书

4. **日志安全**:
   - 日志中不得包含 Token、密码等敏感信息
   - 错误信息不得泄露内部实现细节

<!-- cli_version: 0.1.41 -->
