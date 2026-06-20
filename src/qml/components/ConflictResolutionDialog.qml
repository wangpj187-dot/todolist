import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import TodoApp 1.0

Dialog {
    id: root
    title: "同步冲突"
    width: 700
    height: 550
    modal: true
    closePolicy: Popup.NoAutoClose

    // -------------------------------------------------------------------------
    // Properties
    // -------------------------------------------------------------------------
    property string conflictTodoId: ""
    property string localTitle: ""
    property string localDescription: ""
    property int localPriority: Todo.Medium
    property int localStatus: Todo.Pending
    property var localDueDate: null

    property string remoteTitle: ""
    property string remoteDescription: ""
    property int remotePriority: Todo.Medium
    property int remoteStatus: Todo.Pending
    property var remoteDueDate: null

    // -------------------------------------------------------------------------
    // Functions
    // -------------------------------------------------------------------------
    function openConflict(todoId, localVersion, remoteVersion) {
        conflictTodoId = todoId

        if (localVersion) {
            localTitle = localVersion.title || ""
            localDescription = localVersion.description || ""
            localPriority = localVersion.priority || Todo.Medium
            localStatus = localVersion.status || Todo.Pending
            localDueDate = localVersion.dueDate || null
        }

        if (remoteVersion) {
            remoteTitle = remoteVersion.title || ""
            remoteDescription = remoteVersion.description || ""
            remotePriority = remoteVersion.priority || Todo.Medium
            remoteStatus = remoteVersion.status || Todo.Pending
            remoteDueDate = remoteVersion.dueDate || null
        }

        open()
    }

    function priorityText(priority) {
        switch (priority) {
        case Todo.Low: return "低"
        case Todo.Medium: return "中"
        case Todo.High: return "高"
        case Todo.Urgent: return "紧急"
        default: return "未知"
        }
    }

    function priorityColor(priority) {
        switch (priority) {
        case Todo.Low: return "#10B981"
        case Todo.Medium: return "#3B82F6"
        case Todo.High: return "#F59E0B"
        case Todo.Urgent: return "#EF4444"
        default: return "#9CA3AF"
        }
    }

    function statusText(status) {
        switch (status) {
        case Todo.Pending: return "待处理"
        case Todo.InProgress: return "进行中"
        case Todo.Completed: return "已完成"
        case Todo.Cancelled: return "已取消"
        default: return "未知"
        }
    }

    function formatDate(date) {
        if (!date || typeof date.getTime !== "function" || isNaN(date.getTime())) {
            return "无"
        }
        return date.toLocaleString(Qt.locale("zh_CN"), Locale.ShortFormat)
    }

    function hasDifference(local, remote) {
        return local !== remote
    }

    // -------------------------------------------------------------------------
    // Content
    // -------------------------------------------------------------------------
    contentItem: ColumnLayout {
        spacing: 16
        anchors.margins: 20

        // Warning message
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                width: 24
                height: 24
                radius: 12
                color: "#F59E0B"

                Text {
                    text: "!"
                    color: "#FFFFFF"
                    font.bold: true
                    font.pixelSize: 16
                    anchors.centerIn: parent
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: "检测到同步冲突"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#111827"
                }

                Text {
                    text: "本地版本与云端版本存在差异，请选择要保留的版本"
                    font.pixelSize: 13
                    color: "#6B7280"
                }
            }
        }

        // Side-by-side comparison
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // Local version
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                border.color: "#E5E7EB"
                border.width: 1
                color: "#FFFFFF"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: "#3B82F6"
                        }

                        Text {
                            text: "本地版本"
                            font.pixelSize: 14
                            font.bold: true
                            color: "#111827"
                        }
                    }

                    // Title
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "标题"
                            font.pixelSize: 12
                            color: "#6B7280"
                        }

                        Text {
                            text: localTitle || "(空)"
                            font.pixelSize: 14
                            color: hasDifference(localTitle, remoteTitle) ? "#EF4444" : "#111827"
                            font.weight: hasDifference(localTitle, remoteTitle) ? Font.Bold : Font.Normal
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }

                    // Description
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "描述"
                            font.pixelSize: 12
                            color: "#6B7280"
                        }

                        Text {
                            text: localDescription || "(空)"
                            font.pixelSize: 13
                            color: hasDifference(localDescription, remoteDescription) ? "#EF4444" : "#374151"
                            font.weight: hasDifference(localDescription, remoteDescription) ? Font.Bold : Font.Normal
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                    }

                    // Priority
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "优先级"
                            font.pixelSize: 12
                            color: "#6B7280"
                        }

                        Row {
                            spacing: 6

                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                color: priorityColor(localPriority)
                            }

                            Text {
                                text: priorityText(localPriority)
                                font.pixelSize: 13
                                color: hasDifference(localPriority, remotePriority) ? "#EF4444" : "#374151"
                                font.weight: hasDifference(localPriority, remotePriority) ? Font.Bold : Font.Normal
                            }
                        }
                    }

                    // Status
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "状态"
                            font.pixelSize: 12
                            color: "#6B7280"
                        }

                        Text {
                            text: statusText(localStatus)
                            font.pixelSize: 13
                            color: hasDifference(localStatus, remoteStatus) ? "#EF4444" : "#374151"
                            font.weight: hasDifference(localStatus, remoteStatus) ? Font.Bold : Font.Normal
                        }
                    }

                    // Due date
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "截止日期"
                            font.pixelSize: 12
                            color: "#6B7280"
                        }

                        Text {
                            text: formatDate(localDueDate)
                            font.pixelSize: 13
                            color: hasDifference(formatDate(localDueDate), formatDate(remoteDueDate)) ? "#EF4444" : "#374151"
                            font.weight: hasDifference(formatDate(localDueDate), formatDate(remoteDueDate)) ? Font.Bold : Font.Normal
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    // Use Local button
                    Button {
                        Layout.fillWidth: true
                        text: "保留本地版本"
                        font.pixelSize: 14
                        padding: 12
                        background: Rectangle {
                            color: "#3B82F6"
                            radius: 8
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#FFFFFF"
                            font: parent.font
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                        onClicked: {
                            syncService.resolveConflict(conflictTodoId, true)
                            root.close()
                        }
                    }
                }
            }

            // Remote version
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                border.color: "#E5E7EB"
                border.width: 1
                color: "#FFFFFF"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: "#8B5CF6"
                        }

                        Text {
                            text: "云端版本"
                            font.pixelSize: 14
                            font.bold: true
                            color: "#111827"
                        }
                    }

                    // Title
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "标题"
                            font.pixelSize: 12
                            color: "#6B7280"
                        }

                        Text {
                            text: remoteTitle || "(空)"
                            font.pixelSize: 14
                            color: hasDifference(localTitle, remoteTitle) ? "#EF4444" : "#111827"
                            font.weight: hasDifference(localTitle, remoteTitle) ? Font.Bold : Font.Normal
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }

                    // Description
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "描述"
                            font.pixelSize: 12
                            color: "#6B7280"
                        }

                        Text {
                            text: remoteDescription || "(空)"
                            font.pixelSize: 13
                            color: hasDifference(localDescription, remoteDescription) ? "#EF4444" : "#374151"
                            font.weight: hasDifference(localDescription, remoteDescription) ? Font.Bold : Font.Normal
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                    }

                    // Priority
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "优先级"
                            font.pixelSize: 12
                            color: "#6B7280"
                        }

                        Row {
                            spacing: 6

                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                color: priorityColor(remotePriority)
                            }

                            Text {
                                text: priorityText(remotePriority)
                                font.pixelSize: 13
                                color: hasDifference(localPriority, remotePriority) ? "#EF4444" : "#374151"
                                font.weight: hasDifference(localPriority, remotePriority) ? Font.Bold : Font.Normal
                            }
                        }
                    }

                    // Status
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "状态"
                            font.pixelSize: 12
                            color: "#6B7280"
                        }

                        Text {
                            text: statusText(remoteStatus)
                            font.pixelSize: 13
                            color: hasDifference(localStatus, remoteStatus) ? "#EF4444" : "#374151"
                            font.weight: hasDifference(localStatus, remoteStatus) ? Font.Bold : Font.Normal
                        }
                    }

                    // Due date
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "截止日期"
                            font.pixelSize: 12
                            color: "#6B7280"
                        }

                        Text {
                            text: formatDate(remoteDueDate)
                            font.pixelSize: 13
                            color: hasDifference(formatDate(localDueDate), formatDate(remoteDueDate)) ? "#EF4444" : "#374151"
                            font.weight: hasDifference(formatDate(localDueDate), formatDate(remoteDueDate)) ? Font.Bold : Font.Normal
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    // Use Remote button
                    Button {
                        Layout.fillWidth: true
                        text: "保留云端版本"
                        font.pixelSize: 14
                        padding: 12
                        background: Rectangle {
                            color: "#8B5CF6"
                            radius: 8
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#FFFFFF"
                            font: parent.font
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                        onClicked: {
                            syncService.resolveConflict(conflictTodoId, false)
                            root.close()
                        }
                    }
                }
            }
        }

        // Cancel button
        Button {
            Layout.alignment: Qt.AlignCenter
            text: "稍后处理"
            font.pixelSize: 13
            padding: 10
            flat: true
            onClicked: root.close()
        }
    }
}
