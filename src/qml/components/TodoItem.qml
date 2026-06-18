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
    opacity: (status === Todo.Completed) ? 0.7 : 1.0

    signal editRequested(var todoId, var title, var description, var priority, var categoryId, var dueDate, var status)

    property bool isOverdue: {
        if (status === Todo.Completed)
            return false;
        if (!dueDate || !dueDate.isValid || dueDate.toString().length === 0)
            return false;
        return dueDate < new Date();
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

    property string dueDateText: {
        if (model.dueDate && model.dueDate.isValid) {
            return model.dueDate.toLocaleDateString(Qt.locale(), "yyyy-MM-dd hh:mm")
        }
        return ""
    }

    // Priority color indicator (left border)
    Rectangle {
        id: priorityIndicator
        width: 4
        height: parent.height
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        color: priorityColor || "#3B82F6"
        radius: 8
    }

    RowLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 12
        anchors.topMargin: 12
        anchors.bottomMargin: 12
        spacing: 12

        // Checkbox
        CheckBox {
            id: checkBox
            checked: status === Todo.Completed
            onToggled: {
                if (checked)
                    todoService.completeTodo(model.id)
                else
                    todoService.uncompleteTodo(model.id)
            }
        }

        // Content column
        ColumnLayout {
            id: contentColumn
            Layout.fillWidth: true
            spacing: 4

            // Title
            Text {
                text: model.title || ""
                font.pixelSize: 15
                font.weight: (status === Todo.Completed) ? Font.Normal : Font.Medium
                color: (status === Todo.Completed) ? "#9CA3AF" : "#111827"
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                style: (status === Todo.Completed) ? Text.Strikethrough : Text.Normal
            }

            // Description (if not empty)
            Text {
                text: model.description || ""
                visible: text.length > 0
                font.pixelSize: 13
                color: "#6B7280"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            // Due date row
            Row {
                spacing: 4
                visible: model.dueDate && model.dueDate.isValid && model.dueDate.toString().length > 0

                Text {
                    text: "📅"
                    font.pixelSize: 12
                }

                Text {
                    text: dueDateText || ""
                    font.pixelSize: 12
                    color: root.isOverdue ? "#EF4444" : "#6B7280"
                }
            }
        }

        // Action buttons
        Row {
            spacing: 4

            Button {
                id: editBtn
                text: "编辑"
                flat: true
                font.pixelSize: 12
                padding: 6
                onClicked: {
                    editRequested(
                        model.id,
                        model.title,
                        model.description,
                        model.priority,
                        model.categoryId,
                        model.dueDate,
                        model.status
                    )
                }
            }

            Button {
                id: deleteBtn
                text: "删除"
                flat: true
                font.pixelSize: 12
                padding: 6
                onClicked: {
                    // Confirmation dialog - will be implemented later
                    todoService.deleteTodo(model.id)
                }
            }
        }
    }

    // Hover effect
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: root.border.color = "#3B82F6"
        onExited: root.border.color = "#E5E7EB"
        onClicked: checkBox.checked = !checkBox.checked
    }
}
