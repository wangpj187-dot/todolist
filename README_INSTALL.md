# Todolist 安装说明

## macOS 安装

1. 打开发布包 `dist/Todolist-1.0.0-macos-arm64.dmg`。
2. 将 `Todolist.app` 拖到 `Applications` 文件夹。
3. 在「应用程序」中打开 `Todolist`。

如果 macOS 提示应用来自未认证开发者：

1. 在「应用程序」中右键点击 `Todolist.app`。
2. 选择「打开」。
3. 在弹窗中再次点击「打开」。

这是因为当前安装包未使用 Apple Developer ID 签名，不影响本地测试使用。

## 卸载

1. 退出 `Todolist`。
2. 从「应用程序」中删除 `Todolist.app`。

## 从源码构建

需要安装 Qt 6.5+、CMake、Ninja、libgit2 和 nlohmann/json。

```bash
cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --target Todolist -j 4
```

macOS 打包需要 `macdeployqt`：

```bash
macdeployqt cmake-build-release/Todolist.app -qmldir=src/qml -dmg
```

## Windows 说明

Windows 安装包需要在 Windows 环境下构建。该项目的 CMake 配置已经预留了 NSIS 打包配置，但不能在 macOS 上直接生成 Windows `.exe` 安装包。
