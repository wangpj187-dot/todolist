import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import TodoApp 1.0

Rectangle {
    id: root
    width: parent.width
    height: contentColumn.implicitHeight + 24
    radius: 8
    color: "#FFFFFF"
    border.color: "#E5E7EB"
    border.width: 1
    visible: syncService.isConfigured

    // -------------------------------------------------------------------------
    // Properties
    // -------------------------------------------------------------------------
    property int currentProgress: 0
    property string progressMessage: ""
    property bool showToast: false
    property string toastMessage: ""

    // Status color mapping
    property string statusColor: {
        switch (syncService.syncStatus) {
        case "idle": return "#10B981"
        case "syncing": return "#3B82F6"
        case "conflict": return "#F59E0B"
        case "error": return "#EF4444"
        default: return "#9CA3AF"
        }
    }

    // Status text mapping
    property string statusText: {
        switch (syncService.syncStatus) {
        case "idle": return "已同步"
        case "syncing": return "同步中..."
        case "conflict": return "存在冲突"
        case "error": return "同步错误"
        default: return "未知状态"
        }
    }

    // Formatted last sync time
    property string lastSyncText: {
        if (!syncService.lastSyncTime || !syncService.lastSyncTime.isValid) {
            return "尚未同步"
        }
        var now = new Date()
        var last = syncService.lastSyncTime
        var diffMs = now.getTime() - last.getTime()
        var diffMins = Math.floor(diffMs / 60000)

        if (diffMins < 1) {
            return "最后同步: 刚刚"
        } else if (diffMins < 60) {
            return "最后同步: " + diffMins + " 分钟前"
        } else if (diffMins < 1440) {
            var diffHours = Math.floor(diffMins / 60)
            return "最后同步: " + diffHours + " 小时前"
        } else {
            var locale = Qt.locale("zh_CN")
            return "最后同步: " + locale.toString(last, "yyyy-MM-dd HH:mm")
        }
    }

    // -------------------------------------------------------------------------
    // Signal Handlers
    // -------------------------------------------------------------------------
    Connections {
        target: syncService

        function onSyncProgress(progress, message) {
            currentProgress = progress
            progressMessage = message
        }

        function onSyncCompleted(success, message) {
            currentProgress = 0
            progressMessage = ""
            showToastMessage(success ? "同步成功" : "同步失败: " + message, success)
        }

        function onConflictDetected(todoId, local, remote) {
            showToastMessage("检测到冲突，请解决后重试", false)
        }

        function onSyncError(message) {
            showToastMessage("同步错误: " + message, false)
        }
    }

    // -------------------------------------------------------------------------
    // Functions
    // -------------------------------------------------------------------------
    function showToastMessage(message, success) {
        toastMessage = message
        toast.color = success ? "#10B981" : "#EF4444"
        showToast = true
        toastTimer.restart()
    }

    // -------------------------------------------------------------------------
    // Content
    // -------------------------------------------------------------------------
    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 12
        anchors.bottomMargin: 12
        spacing: 10

        // Top row: Status indicator + sync button
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            // Status indicator dot
            Rectangle {
                id: statusDot
                width: 10
                height: 10
                radius: 5
                color: root.statusColor

                // Pulse animation when syncing
                PropertyAnimation on opacity {
                    running: syncService.isSyncing
                    from: 1.0
                    to: 0.4
                    duration: 800
                    loops: Animation.Infinite
                    easing.type: Easing.InOutQuad
                }
            }

            // Status text
            Text {
                text: root.statusText
                font.pixelSize: 14
                font.weight: Font.Medium
                color: "#111827"
            }

            // Last sync time
            Text {
                text: root.lastSyncText
                font.pixelSize: 12
                color: "#9CA3AF"
                Layout.fillWidth: true
            }

            // Pending changes badge
            Row {
                visible: syncService.hasPendingChanges
                spacing: 4
                Layout.rightMargin: 8

                Rectangle {
                    width: 20
                    height: 20
                    radius: 10
                    color: "#F59E0B"

                    Text {
                        text: "!"
                        anchors.centerIn: parent
                        font.pixelSize: 12
                        font.bold: true
                        color: "#FFFFFF"
                    }
                }

                Text {
                    text: "有未同步的更改"
                    font.pixelSize: 12
                    color: "#F59E0B"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // Sync button
            Button {
                id: syncButton
                text: syncService.isSyncing ? "同步中..." : "立即同步"
                font.pixelSize: 13
                padding: 8
                enabled: !syncService.isSyncing
                opacity: enabled ? 1.0 : 0.5

                background: Rectangle {
                    color: syncButton.enabled ? (syncButton.down ? "#2563EB" : "#3B82F6") : "#9CA3AF"
                    radius: 6
                }

                contentItem: Text {
                    text: syncButton.text
                    color: "#FFFFFF"
                    font: syncButton.font
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }

                onClicked: {
                    syncService.syncNow()
                }
            }
        }

        // Progress bar (visible only during sync)
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            visible: syncService.isSyncing

            // Progress message
            Text {
                text: progressMessage || "正在同步..."
                font.pixelSize: 12
                color: "#6B7280"
            }

            // Progress bar background
            Rectangle {
                Layout.fillWidth: true
                height: 6
                radius: 3
                color: "#E5E7EB"

                // Progress bar fill
                Rectangle {
                    id: progressFill
                    width: (currentProgress / 100) * parent.width
                    height: parent.height
                    radius: 3
                    color: "#3B82F6"

                    Behavior on width {
                        NumberAnimation { duration: 200 }
                    }
                }
            }

            // Progress percentage
            Text {
                text: currentProgress + "%"
                font.pixelSize: 11
                color: "#9CA3AF"
                Layout.alignment: Qt.AlignRight
            }
        }
    }

    // -------------------------------------------------------------------------
    // Toast Notification
    // -------------------------------------------------------------------------
    Rectangle {
        id: toast
        visible: showToast
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 16
        width: Math.min(toastText.implicitWidth + 24, parent.width - 32)
        height: toastText.implicitHeight + 12
        radius: 6
        color: "#10B981"
        opacity: showToast ? 1.0 : 0.0

        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }

        Text {
            id: toastText
            text: toastMessage
            anchors.centerIn: parent
            font.pixelSize: 13
            color: "#FFFFFF"
            wrapMode: Text.Wrap
            maximumLineCount: 2
        }
    }

    Timer {
        id: toastTimer
        interval: 3000
        onTriggered: {
            showToast = false
        }
    }
}
