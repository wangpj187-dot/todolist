import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import TodoApp 1.0

Rectangle {
    id: root
    width: ListView.view ? ListView.view.width : parent.width
    height: Math.max(contentColumn.implicitHeight, actionRow.implicitHeight, checkBox.implicitHeight) + 22
    transformOrigin: Item.Center
    scale: 1.0
    radius: 10
    color: (status === Todo.Completed) ? "#FAFAFA" : "#FFFFFF"
    border.color: hoverArea.containsMouse ? "#D1D5DB" : "#E5E7EB"
    border.width: 1
    opacity: ((status === Todo.Completed) ? 0.7 : 1.0) * animationOpacity

    Behavior on color { ColorAnimation { duration: 150 } }
    Behavior on border.color { ColorAnimation { duration: 150 } }

    // Responsive: shrink text & condense action buttons on narrow cards
    readonly property bool compact: width < 360
    readonly property string flagTag: "旗标"
    property bool completing: false
    property real animationOpacity: 1.0
    property var todoId: model.id
    property int currentPriority: model.priority
    readonly property var priorityOptions: [
        { "value": Todo.Urgent, "label": "紧急" },
        { "value": Todo.High, "label": "高" },
        { "value": Todo.Medium, "label": "中" },
        { "value": Todo.Low, "label": "低" }
    ]

    signal editRequested(var todoId, var title, var description, var priority, var categoryId, var dueDate, var status, var tags)

    // Hover-only layer; completion is handled by the checkbox so row actions stay reliable.
    MouseArea {
        id: hoverArea
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
    }

    function hasValidDate(value) {
        return value && typeof value.getTime === "function" && !isNaN(value.getTime())
    }

    function tagsArray() {
        if (!model.tags)
            return []

        const result = []
        for (let i = 0; i < model.tags.length; i++) {
            const tag = ("" + model.tags[i]).trim()
            if (tag.length > 0)
                result.push(tag)
        }
        return result
    }

    property bool flagged: tagsArray().indexOf(flagTag) !== -1

    property bool isOverdue: {
        if (status === Todo.Completed)
            return false;
        if (!hasValidDate(dueDate))
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

    property string priorityLabel: {
        switch (model.priority) {
        case Todo.Urgent: return "紧急"
        case Todo.High: return "高"
        case Todo.Medium: return "中"
        case Todo.Low: return "低"
        default: return ""
        }
    }

    // Light background tint for the priority badge
    property string priorityBgColor: {
        switch (model.priority) {
        case Todo.Urgent: return "#FEE2E2"
        case Todo.High: return "#FFEDD5"
        case Todo.Medium: return "#FEF9C3"
        case Todo.Low: return "#DCFCE7"
        default: return "#F3F4F6"
        }
    }

    property string dueDateText: {
        if (hasValidDate(model.dueDate)) {
            return model.dueDate.toLocaleDateString(Qt.locale(), "yyyy-MM-dd hh:mm")
        }
        return ""
    }

    function priorityColorFor(value) {
        switch (value) {
        case Todo.Urgent: return "#DC2626"
        case Todo.High: return "#F97316"
        case Todo.Medium: return "#EAB308"
        case Todo.Low: return "#22C55E"
        default: return "#6B7280"
        }
    }

    function priorityBgColorFor(value) {
        switch (value) {
        case Todo.Urgent: return "#FEE2E2"
        case Todo.High: return "#FFEDD5"
        case Todo.Medium: return "#FEF9C3"
        case Todo.Low: return "#DCFCE7"
        default: return "#F3F4F6"
        }
    }

    function completeWithAnimation() {
        if (completing)
            return
        completing = true
        completeExitAnimation.restart()
    }

    SequentialAnimation {
        id: completeExitAnimation

        ParallelAnimation {
            NumberAnimation {
                target: root
                property: "scale"
                from: 1.0
                to: 0.12
                duration: 520
                easing.type: Easing.InBack
                easing.overshoot: 1.25
            }
            NumberAnimation {
                target: root
                property: "animationOpacity"
                from: 1.0
                to: 0.0
                duration: 480
                easing.type: Easing.InCubic
            }
        }

        ScriptAction {
            script: {
                if (!todoService.completeTodo(root.todoId)) {
                    root.scale = 1.0
                    root.animationOpacity = 1.0
                    root.completing = false
                }
            }
        }
    }

    // Priority color indicator (left border)
    Rectangle {
        id: priorityIndicator
        width: 6
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
        anchors.rightMargin: 10
        anchors.topMargin: 11
        anchors.bottomMargin: 11
        spacing: 12

        // Checkbox
        CheckBox {
            id: checkBox
            Layout.alignment: Qt.AlignTop
            enabled: !root.completing
            checked: status === Todo.Completed || root.completing
            onClicked: {
                if (checked)
                    root.completeWithAnimation()
                else
                    todoService.uncompleteTodo(model.id)
            }
        }

        // Content column
        ColumnLayout {
            id: contentColumn
            Layout.fillWidth: true
            spacing: 4

            // Title row with priority badge
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                // Priority badge
                Rectangle {
                    id: priorityBadge
                    Layout.alignment: Qt.AlignTop
                    visible: root.priorityLabel.length > 0
                    implicitWidth: priorityBadgeText.implicitWidth + 14
                    implicitHeight: 20
                    radius: 10
                    color: priorityBadgeMouse.containsMouse || priorityPopup.opened ? "#FFFFFF" : root.priorityBgColor
                    border.color: root.priorityColor
                    border.width: 1
                    z: priorityPopup.opened ? 3 : 1
                    ToolTip.visible: priorityBadgeMouse.containsMouse
                    ToolTip.text: "切换重要性"

                    Text {
                        id: priorityBadgeText
                        anchors.centerIn: parent
                        text: root.priorityLabel
                        font.pixelSize: 11
                        font.bold: true
                        color: root.priorityColor
                    }

                    MouseArea {
                        id: priorityBadgeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: priorityPopup.opened ? priorityPopup.close() : priorityPopup.open()
                    }
                }

                // Title
                Text {
                    text: model.title || ""
                    font.pixelSize: root.compact ? 14 : 15
                    font.weight: (status === Todo.Completed) ? Font.Normal : Font.Medium
                    color: (status === Todo.Completed) ? "#9CA3AF" : "#111827"
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    font.strikeout: status === Todo.Completed
                }
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
                visible: root.hasValidDate(model.dueDate)

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

            Flow {
                Layout.fillWidth: true
                spacing: 5
                visible: root.tagsArray().length > 0

                Repeater {
                    model: root.tagsArray()

                    delegate: Rectangle {
                        height: 20
                        width: tagText.implicitWidth + 14
                        radius: 10
                        color: modelData === root.flagTag ? "#FFF7ED" : "#EFF6FF"
                        border.color: modelData === root.flagTag ? "#FDBA74" : "#BFDBFE"
                        border.width: 1

                        Text {
                            id: tagText
                            anchors.centerIn: parent
                            text: modelData === root.flagTag ? "旗标" : "#" + modelData
                            color: modelData === root.flagTag ? "#C2410C" : "#1D4ED8"
                            font.pixelSize: 11
                            font.weight: Font.Medium
                        }
                    }
                }
            }
        }

        // Action buttons
        Row {
            id: actionRow
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: root.compact ? 76 : 104
            Layout.preferredHeight: 32
            spacing: 4
            z: 2

            Button {
                id: editBtn
                width: root.compact ? 36 : 50
                height: 30
                text: root.compact ? "编" : "编辑"
                font.pixelSize: 12
                background: Rectangle {
                    radius: 6
                    color: editBtn.down ? "#DBEAFE" : editBtn.hovered ? "#EFF6FF" : "transparent"
                    scale: editBtn.down ? 0.94 : 1.0
                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                }
                contentItem: Text {
                    text: editBtn.text
                    font: editBtn.font
                    color: editBtn.hovered ? "#2563EB" : "#6B7280"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                onClicked: {
                    editRequested(
                        model.id,
                        model.title,
                        model.description,
                        model.priority,
                        model.categoryId,
                        model.dueDate,
                        model.status,
                        model.tags
                    )
                }
            }

            Button {
                id: deleteBtn
                width: root.compact ? 36 : 50
                height: 30
                text: root.compact ? "删" : "删除"
                font.pixelSize: 12
                background: Rectangle {
                    radius: 6
                    color: deleteBtn.down ? "#FEE2E2" : deleteBtn.hovered ? "#FEF2F2" : "transparent"
                    scale: deleteBtn.down ? 0.94 : 1.0
                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                }
                contentItem: Text {
                    text: deleteBtn.text
                    font: deleteBtn.font
                    color: deleteBtn.hovered ? "#DC2626" : "#6B7280"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                onClicked: {
                    if (todoService.deleteTodo(model.id)) {
                        todoService.refresh()
                    }
                }
            }
        }
    }

    Popup {
        id: priorityPopup
        parent: root
        x: Math.max(0, Math.min(root.width - width, priorityBadge.mapToItem(root, 0, 0).x))
        y: priorityBadge.mapToItem(root, 0, priorityBadge.height + 5).y
        width: 92
        padding: 5
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        z: 20

        background: Rectangle {
            radius: 9
            color: "#FFFFFF"
            border.color: "#DADAE0"
            border.width: 1
        }

        contentItem: Column {
            id: priorityMenuColumn
            width: priorityPopup.width - priorityPopup.padding * 2
            spacing: 4

            Repeater {
                model: root.priorityOptions

                delegate: Button {
                    id: priorityOptionButton
                    width: priorityMenuColumn.width
                    height: 30
                    text: modelData.label
                    font.pixelSize: 12
                    property bool selected: root.currentPriority === modelData.value
                    onClicked: {
                        todoService.setTodoPriorityFromQml(root.todoId, modelData.value)
                        priorityPopup.close()
                    }

                    background: Rectangle {
                        radius: 7
                        color: priorityOptionButton.selected
                               ? root.priorityBgColorFor(modelData.value)
                               : priorityOptionButton.hovered ? "#F9FAFB" : "transparent"
                        border.color: priorityOptionButton.selected ? root.priorityColorFor(modelData.value) : "transparent"
                        border.width: priorityOptionButton.selected ? 1 : 0
                    }

                    contentItem: Item {
                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: 7
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 7

                            Rectangle {
                                width: 9
                                height: 9
                                radius: 5
                                color: root.priorityColorFor(modelData.value)
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                text: priorityOptionButton.text
                                color: priorityOptionButton.selected ? root.priorityColorFor(modelData.value) : "#374151"
                                font: priorityOptionButton.font
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }
        }
    }
}
