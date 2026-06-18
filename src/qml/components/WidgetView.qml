import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import TodoApp 1.0

ColumnLayout {
    id: widgetView
    anchors.fill: parent
    anchors.margins: 8
    spacing: 6

    // -------------------------------------------------------------------------
    // Header with count
    // -------------------------------------------------------------------------

    RowLayout {
        Layout.fillWidth: true
        spacing: 4

        Text {
            text: "紧急待办"
            font.pixelSize: 12
            font.bold: true
            color: "#374151"
        }

        Rectangle {
            width: 18
            height: 18
            radius: 9
            color: "#EF4444"

            Text {
                anchors.centerIn: parent
                text: highPriorityList.count
                font.pixelSize: 10
                font.bold: true
                color: "#FFFFFF"
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            text: "刷新"
            font.pixelSize: 10
            padding: 4
            background: Rectangle {
                color: "#E5E7EB"
                radius: 4
            }
            contentItem: Text {
                text: parent.text
                color: "#374151"
                font: parent.font
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
            onClicked: todoService.refresh()
        }
    }

    // -------------------------------------------------------------------------
    // High priority todo list
    // -------------------------------------------------------------------------

    ListView {
        id: highPriorityList
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: todoService.getHighPriorityTodos()
        spacing: 4
        clip: true
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        delegate: Rectangle {
            id: widgetTodoItem
            width: parent.width
            height: contentRow.implicitHeight + 12
            radius: 6
            color: widgetTodoItemMouseArea.containsMouse ? "#EEF2FF" : "#FFFFFF"
            border.color: isOverdue ? "#FECACA" : "#E5E7EB"
            border.width: 1

            property bool isOverdue: {
                if (model.dueDate && model.dueDate.isValid && model.dueDate.toString().length > 0) {
                    return model.dueDate < new Date()
                }
                return false
            }

            property string priorityColor: {
                switch (model.priority) {
                case Todo.Urgent: return "#DC2626"
                case Todo.High: return "#F97316"
                case Todo.Medium: return "#EAB308"
                case Todo.Low: return "#22C55E"
                default: return "#6B7280"
                }
            }

            // Priority indicator
            Rectangle {
                width: 3
                height: parent.height
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                color: widgetTodoItem.priorityColor
                radius: 6
                topRightRadius: 0
                bottomRightRadius: 0
            }

            RowLayout {
                id: contentRow
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 8
                anchors.topMargin: 6
                anchors.bottomMargin: 6
                spacing: 6

                // Checkbox
                CheckBox {
                    id: widgetCheckBox
                    checked: model.status === Todo.Completed
                    onToggled: {
                        if (checked)
                            todoService.completeTodo(model.id)
                        else
                            todoService.uncompleteTodo(model.id)
                        todoService.refresh()
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    // Title
                    Text {
                        text: model.title || ""
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        color: (model.status === Todo.Completed) ? "#9CA3AF" : "#111827"
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        style: (model.status === Todo.Completed) ? Text.Strikethrough : Text.Normal
                    }

                    // Due date
                    Row {
                        spacing: 3
                        visible: model.dueDate && model.dueDate.isValid && model.dueDate.toString().length > 0

                        Text {
                            text: "📅"
                            font.pixelSize: 10
                        }

                        Text {
                            text: {
                                if (model.dueDate && model.dueDate.isValid) {
                                    return model.dueDate.toLocaleDateString(Qt.locale(), "MM-dd hh:mm")
                                }
                                return ""
                            }
                            font.pixelSize: 10
                            color: widgetTodoItem.isOverdue ? "#DC2626" : "#6B7280"
                        }
                    }
                }

                // Priority badge
                Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    color: widgetTodoItem.priorityColor
                    ToolTip.text: {
                        switch (model.priority) {
                        case Todo.Urgent: return "紧急"
                        case Todo.High: return "高"
                        case Todo.Medium: return "中"
                        case Todo.Low: return "低"
                        default: return ""
                        }
                    }
                    ToolTip.visible: widgetTodoItemMouseArea.containsMouse
                }
            }

            MouseArea {
                id: widgetTodoItemMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: widgetCheckBox.checked = !widgetCheckBox.checked
            }
        }
    }

    // -------------------------------------------------------------------------
    // Empty state
    // -------------------------------------------------------------------------

    Column {
        id: widgetEmptyState
        visible: highPriorityList.count === 0
        anchors.centerIn: parent
        spacing: 4

        Text {
            text: "🎉"
            font.pixelSize: 24
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "没有紧急待办"
            font.pixelSize: 12
            color: "#9CA3AF"
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
