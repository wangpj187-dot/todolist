import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import TodoApp 1.0
import "components"

ApplicationWindow {
    id: root
    width: configService.windowWidth
    height: configService.windowHeight
    minimumWidth: 300
    minimumHeight: 400
    title: "Todolist"
    color: "#FFFFFF"
    visible: true

    // Window opacity from config
    opacity: configService.windowOpacity

    // Apply alwaysOnTop from config
    flags: configService.alwaysOnTop ? (root.flags | Qt.WindowStaysOnTopHint) : root.flags

    // -------------------------------------------------------------------------
    // Window position/size persistence
    // -------------------------------------------------------------------------

    // Restore position on startup
    Component.onCompleted: {
        if (configService.windowX !== 0 || configService.windowY !== 0) {
            root.x = configService.windowX
            root.y = configService.windowY
        }
    }

    // Save position when window is moved
    onXChanged: {
        if (root.visible && root.visibility !== Window.Minimized) {
            configService.setWindowX(root.x)
        }
    }
    onYChanged: {
        if (root.visible && root.visibility !== Window.Minimized) {
            configService.setWindowY(root.y)
        }
    }

    // Save size when window is resized
    onWidthChanged: {
        if (root.visible && root.width >= root.minimumWidth) {
            configService.setWindowWidth(root.width)
        }
    }
    onHeightChanged: {
        if (root.visible && root.height >= root.minimumHeight) {
            configService.setWindowHeight(root.height)
        }
    }

    // Auto-save config periodically when changes occur
    Timer {
        id: configSaveTimer
        interval: 1000
        repeat: false
        onTriggered: configService.save()
    }

    function triggerConfigSave() {
        configSaveTimer.restart()
    }

    Connections {
        target: configService
        function onWindowPositionChanged() { root.triggerConfigSave() }
        function onWindowSizeChanged() { root.triggerConfigSave() }
        function onAlwaysOnTopChanged() { root.triggerConfigSave() }
        function onWindowOpacityChanged() { root.triggerConfigSave() }
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 12

        // ---------------------------------------------------------------------
        // Header
        // ---------------------------------------------------------------------
        RowLayout {
            id: header
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 16
            spacing: 8

            Text {
                text: "待办事项"
                font.pixelSize: 20
                font.bold: true
                color: "#111827"
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                id: addButton
                text: "+ 新建"
                font.pixelSize: 14
                padding: 8
                background: Rectangle {
                    color: "#3B82F6"
                    radius: 8
                }
                contentItem: Text {
                    text: addButton.text
                    color: "#FFFFFF"
                    font: addButton.font
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                onClicked: todoForm.openForCreate()
            }
        }

        // ---------------------------------------------------------------------
        // Filter Controls
        // ---------------------------------------------------------------------
        RowLayout {
            id: filterControls
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            spacing: 8

            ComboBox {
                id: statusComboBox
                Layout.preferredWidth: 100
                model: ["全部", "待处理", "进行中", "已完成", "已取消"]
                currentIndex: 0
                font.pixelSize: 12
                background: Rectangle {
                    color: "#F3F4F6"
                    radius: 8
                    border.color: "#E5E7EB"
                    border.width: 1
                }
                contentItem: Text {
                    text: statusComboBox.displayText
                    color: "#374151"
                    font: statusComboBox.font
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 8
                    rightPadding: statusComboBox.indicator.width + 8
                }
                onCurrentIndexChanged: {
                    todoService.filterStatus = currentIndex
                }
            }

            ComboBox {
                id: priorityComboBox
                Layout.preferredWidth: 110
                model: ["全部优先级", "低", "中", "高", "紧急"]
                currentIndex: 0
                font.pixelSize: 12
                background: Rectangle {
                    color: "#F3F4F6"
                    radius: 8
                    border.color: "#E5E7EB"
                    border.width: 1
                }
                contentItem: Text {
                    text: priorityComboBox.displayText
                    color: "#374151"
                    font: priorityComboBox.font
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 8
                    rightPadding: priorityComboBox.indicator.width + 8
                }
                onCurrentIndexChanged: {
                    todoService.filterPriority = currentIndex
                }
            }

            ComboBox {
                id: sortComboBox
                Layout.preferredWidth: 110
                model: ["按创建时间", "按优先级", "按截止日期", "按标题"]
                currentIndex: 0
                font.pixelSize: 12
                background: Rectangle {
                    color: "#F3F4F6"
                    radius: 8
                    border.color: "#E5E7EB"
                    border.width: 1
                }
                contentItem: Text {
                    text: sortComboBox.displayText
                    color: "#374151"
                    font: sortComboBox.font
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 8
                    rightPadding: sortComboBox.indicator.width + 8
                }
                onCurrentIndexChanged: {
                    switch (currentIndex) {
                    case 0: todoService.sortBy = "createdAt"; break
                    case 1: todoService.sortBy = "priority"; break
                    case 2: todoService.sortBy = "dueDate"; break
                    case 3: todoService.sortBy = "title"; break
                    }
                }
            }

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "搜索..."
                font.pixelSize: 12
                background: Rectangle {
                    color: "#F3F4F6"
                    radius: 8
                    border.color: "#E5E7EB"
                    border.width: 1
                }
                onTextChanged: {
                    todoService.searchQuery = text
                }
            }
        }

        // ---------------------------------------------------------------------
        // Todo List View
        // ---------------------------------------------------------------------
        ListView {
            id: todoListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16
            model: todoService.todos
            delegate: TodoItem {
                onEditRequested: function(todoId, title, description, priority, categoryId, dueDate, status) {
                    todoForm.openForEdit(todoId, title, description, priority, categoryId, dueDate, status)
                }
            }
            spacing: 8
            clip: true
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
        }

        // ---------------------------------------------------------------------
        // Empty State (overlay on list view)
        // ---------------------------------------------------------------------
        Column {
            id: emptyState
            visible: todoListView.count === 0
            anchors.centerIn: todoListView
            spacing: 8
            z: 1

            Text {
                text: "暂无待办事项"
                font.pixelSize: 16
                color: "#9CA3AF"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "点击右上角 + 新建按钮添加"
                font.pixelSize: 14
                color: "#D1D5DB"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    // -------------------------------------------------------------------------
    // Todo Form Dialog
    // -------------------------------------------------------------------------
    TodoForm {
        id: todoForm
        onSaved: {
            todoService.refresh()
        }
    }

    // -------------------------------------------------------------------------
    // Widget Window (desktop widget mode)
    // -------------------------------------------------------------------------
    WidgetWindow {
        id: widgetWindow
    }

    // -------------------------------------------------------------------------
    // Quick Add Dialog (for tray menu quick add)
    // -------------------------------------------------------------------------
    Dialog {
        id: quickAddDialog
        title: "快速添加待办"
        width: 400
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        ColumnLayout {
            spacing: 12
            anchors.fill: parent

            TextField {
                id: quickTitleField
                Layout.fillWidth: true
                placeholderText: "标题"
                focus: true
            }

            ComboBox {
                id: quickPriorityCombo
                Layout.fillWidth: true
                model: ["低", "中", "高", "紧急"]
                currentIndex: 1 // Medium by default
            }
        }

        onAccepted: {
            if (quickTitleField.text.trimmed().length > 0) {
                todoService.createTodo(
                    quickTitleField.text.trimmed(),
                    "",
                    quickPriorityCombo.currentIndex + 1, // Priority enum starts at 1
                    QUuid(), // No category
                    QDateTime() // No due date
                )
                todoService.refresh()
                quickTitleField.text = ""
                quickPriorityCombo.currentIndex = 1
            }
        }
    }

    // -------------------------------------------------------------------------
    // System Tray Manager Connections
    // -------------------------------------------------------------------------

    Connections {
        target: systemTrayManager

        function onShowMainWindowRequested() {
            root.show()
            root.raise()
            root.requestActivate()
            systemTrayManager.setMainWindowVisible(true)
        }

        function onHideMainWindowRequested() {
            root.hide()
            systemTrayManager.setMainWindowVisible(false)
        }

        function onToggleMainWindowRequested() {
            if (root.visible) {
                root.hide()
                systemTrayManager.setMainWindowVisible(false)
            } else {
                root.show()
                root.raise()
                root.requestActivate()
                systemTrayManager.setMainWindowVisible(true)
            }
        }

        function onShowWidgetRequested() {
            widgetWindow.show()
            widgetWindow.raise()
            systemTrayManager.setWidgetVisible(true)
        }

        function onHideWidgetRequested() {
            widgetWindow.hide()
            systemTrayManager.setWidgetVisible(false)
        }

        function onToggleWidgetRequested() {
            if (widgetWindow.visible) {
                widgetWindow.hide()
                systemTrayManager.setWidgetVisible(false)
            } else {
                widgetWindow.show()
                widgetWindow.raise()
                systemTrayManager.setWidgetVisible(true)
            }
        }

        function onQuickAddRequested() {
            root.show()
            root.raise()
            root.requestActivate()
            quickAddDialog.open()
        }

        function onSyncRequested() {
            syncService.syncNow()
        }

        function onOpenSettingsRequested() {
            root.show()
            root.raise()
            root.requestActivate()
            // Settings dialog will be implemented in later phase
            systemTrayManager.showNotification("设置", "设置功能开发中...")
        }

        function onQuitRequested() {
            // Save config before quitting
            configService.save()
        }
    }

    // -------------------------------------------------------------------------
    // Window visibility tracking for tray menu
    // -------------------------------------------------------------------------

    onVisibleChanged: {
        systemTrayManager.setMainWindowVisible(root.visible)
    }
}
