import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic 6.3 as Basic
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
    property string datePickerMode: "due"
    property var datePickerAnchor: null
    property var quickStartDate: new Date()
    property var quickEndDate: new Date()
    property var quickSelectedDate: new Date()
    property bool waitingForRangeEnd: false
    property int quickCalendarMonth: quickSelectedDate.getMonth()
    property int quickCalendarYear: quickSelectedDate.getFullYear()
    readonly property var priorityOptions: [
        { "value": Todo.Urgent, "label": "紧急" },
        { "value": Todo.High, "label": "高" },
        { "value": Todo.Medium, "label": "中" },
        { "value": Todo.Low, "label": "低" }
    ]

    signal editRequested(var todoId, var title, var description, var priority, var categoryId, var dueDate, var status, var tags, var startDate)

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
        if (hasValidDate(model.startDate) && hasValidDate(model.dueDate)) {
            const startText = model.startDate.toLocaleDateString(Qt.locale(), "yyyy-MM-dd")
            const endText = model.dueDate.toLocaleDateString(Qt.locale(), "yyyy-MM-dd")
            return startText === endText ? endText : startText + " - " + endText
        }
        if (hasValidDate(model.dueDate)) {
            return model.dueDate.toLocaleDateString(Qt.locale(), "yyyy-MM-dd hh:mm")
        }
        return ""
    }
    readonly property bool hasDateRange: hasValidDate(model.startDate)
                                         && hasValidDate(model.dueDate)
                                         && !sameDay(model.startDate, model.dueDate)
    readonly property string startDateText: hasValidDate(model.startDate)
                                            ? model.startDate.toLocaleDateString(Qt.locale(), "yyyy-MM-dd") : ""
    readonly property string endDateText: hasValidDate(model.dueDate)
                                          ? model.dueDate.toLocaleDateString(Qt.locale(), "yyyy-MM-dd") : ""

    function dayStart(value) {
        const date = hasValidDate(value) ? value : new Date()
        return new Date(date.getFullYear(), date.getMonth(), date.getDate(), 0, 0, 0)
    }

    function dayEnd(value) {
        const date = hasValidDate(value) ? value : new Date()
        return new Date(date.getFullYear(), date.getMonth(), date.getDate(), 23, 59, 59)
    }

    function currentStartDate() {
        if (hasValidDate(model.startDate))
            return model.startDate
        if (hasValidDate(model.dueDate))
            return model.dueDate
        return new Date()
    }

    function currentEndDate() {
        if (hasValidDate(model.dueDate))
            return model.dueDate
        if (hasValidDate(model.startDate))
            return model.startDate
        return new Date()
    }

    function sameDay(left, right) {
        return hasValidDate(left)
                && hasValidDate(right)
                && left.getFullYear() === right.getFullYear()
                && left.getMonth() === right.getMonth()
                && left.getDate() === right.getDate()
    }

    function setQuickCalendarDate(value) {
        const date = hasValidDate(value) ? value : new Date()
        quickSelectedDate = date
        quickCalendarMonth = date.getMonth()
        quickCalendarYear = date.getFullYear()
    }

    function resetQuickDates(anchor, mode) {
        datePickerAnchor = anchor || null
        datePickerMode = mode
        waitingForRangeEnd = false
        quickStartDate = dayStart(currentStartDate())
        quickEndDate = dayEnd(currentEndDate())
        if (quickEndDate < quickStartDate)
            quickStartDate = dayStart(quickEndDate)
        setQuickCalendarDate(mode === "start" || mode === "range" ? quickStartDate : quickEndDate)
    }

    function openStartDatePicker(anchor) {
        resetQuickDates(anchor, "start")
        quickDatePopup.open()
    }

    function openDueDatePicker(anchor) {
        resetQuickDates(anchor, "end")
        quickDatePopup.open()
    }

    function openRangeDatePicker(anchor) {
        resetQuickDates(anchor, "range")
        quickDatePopup.open()
    }

    function shiftQuickCalendarMonth(delta) {
        const next = new Date(quickCalendarYear, quickCalendarMonth + delta, 1)
        quickCalendarYear = next.getFullYear()
        quickCalendarMonth = next.getMonth()
    }

    function saveQuickDateRange(startValue, endValue) {
        const success = todoService.updateTodoFromQmlWithTagsAndRange(
            model.id,
            model.title || "",
            model.description || "",
            model.priority,
            model.categoryId,
            dayStart(startValue),
            dayEnd(endValue),
            model.status,
            tagsArray()
        )
        if (success)
            quickDatePopup.close()
        return success
    }

    function applyQuickDate(value) {
        if (datePickerMode === "range") {
            if (!waitingForRangeEnd) {
                quickStartDate = dayStart(value)
                quickEndDate = dayEnd(value)
                waitingForRangeEnd = true
                setQuickCalendarDate(value)
                return false
            }

            quickEndDate = dayEnd(value)
            if (quickEndDate < quickStartDate) {
                const start = quickStartDate
                quickStartDate = dayStart(quickEndDate)
                quickEndDate = dayEnd(start)
            }
            setQuickCalendarDate(value)
            return saveQuickDateRange(quickStartDate, quickEndDate)
        }

        if (datePickerMode === "start") {
            quickStartDate = dayStart(value)
            quickEndDate = dayEnd(currentEndDate())
            if (quickEndDate < quickStartDate)
                quickEndDate = dayEnd(value)
        } else {
            quickEndDate = dayEnd(value)
            quickStartDate = hasValidDate(model.startDate) ? dayStart(model.startDate) : dayStart(value)
            if (quickEndDate < quickStartDate)
                quickStartDate = dayStart(value)
        }
        setQuickCalendarDate(value)
        return saveQuickDateRange(quickStartDate, quickEndDate)
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

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                spacing: 5
                visible: root.hasValidDate(model.dueDate)

                Button {
                    id: dateIconButton
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    padding: 0
                    ToolTip.visible: hovered
                    ToolTip.text: "修改起始和截止日期"
                    onClicked: root.openRangeDatePicker(dateIconButton)

                    background: Rectangle {
                        radius: 6
                        color: dateIconButton.down ? "#DBEAFE" : dateIconButton.hovered || quickDatePopup.opened && root.datePickerMode === "range" ? "#EFF6FF" : "transparent"
                        scale: dateIconButton.down ? 0.94 : 1.0
                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                    }

                    contentItem: Canvas {
                        id: dateIconCanvas
                        implicitWidth: 16
                        implicitHeight: 16

                        onPaint: {
                            const ctx = getContext("2d")
                            const stroke = dateIconButton.down ? "#1D4ED8" : dateIconButton.hovered || quickDatePopup.opened && root.datePickerMode === "range" ? "#2563EB" : "#6B7280"
                            const ox = Math.round((width - 16) / 2)
                            const oy = Math.round((height - 16) / 2)
                            ctx.clearRect(0, 0, width, height)
                            ctx.save()
                            ctx.translate(ox, oy)
                            ctx.lineWidth = 1.6
                            ctx.strokeStyle = stroke
                            ctx.fillStyle = stroke
                            ctx.lineCap = "round"
                            ctx.lineJoin = "round"
                            ctx.beginPath()
                            ctx.moveTo(4, 3)
                            ctx.lineTo(12, 3)
                            ctx.quadraticCurveTo(14, 3, 14, 5)
                            ctx.lineTo(14, 12)
                            ctx.quadraticCurveTo(14, 14, 12, 14)
                            ctx.lineTo(4, 14)
                            ctx.quadraticCurveTo(2, 14, 2, 12)
                            ctx.lineTo(2, 5)
                            ctx.quadraticCurveTo(2, 3, 4, 3)
                            ctx.stroke()
                            ctx.beginPath()
                            ctx.moveTo(2, 6.5)
                            ctx.lineTo(14, 6.5)
                            ctx.stroke()
                            ctx.beginPath()
                            ctx.moveTo(5.2, 1.8)
                            ctx.lineTo(5.2, 4.2)
                            ctx.moveTo(10.8, 1.8)
                            ctx.lineTo(10.8, 4.2)
                            ctx.stroke()
                            ctx.beginPath()
                            ctx.arc(5.5, 9.3, 0.7, 0, Math.PI * 2)
                            ctx.arc(8.5, 9.3, 0.7, 0, Math.PI * 2)
                            ctx.arc(5.5, 12, 0.7, 0, Math.PI * 2)
                            ctx.arc(8.5, 12, 0.7, 0, Math.PI * 2)
                            ctx.fill()
                            ctx.restore()
                        }
                    }

                    onDownChanged: dateIconCanvas.requestPaint()
                    onHoveredChanged: dateIconCanvas.requestPaint()
                }

                RowLayout {
                    id: dateText
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        id: startDateTextItem
                        text: root.hasDateRange ? root.startDateText : root.dueDateText
                        font.pixelSize: 12
                        color: startDateMouse.containsMouse
                               || quickDatePopup.opened && (root.datePickerMode === "start" || root.datePickerMode === "range" && !root.waitingForRangeEnd)
                               ? "#2563EB"
                               : root.isOverdue ? "#EF4444" : "#6B7280"
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        Layout.fillWidth: !root.hasDateRange
                        Layout.minimumWidth: 0
                        ToolTip.visible: startDateMouse.containsMouse
                        ToolTip.text: root.hasDateRange ? "修改开始日期" : "修改日期"
                        Behavior on color { ColorAnimation { duration: 150 } }

                        MouseArea {
                            id: startDateMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.hasDateRange ? root.openStartDatePicker(startDateTextItem) : root.openDueDatePicker(startDateTextItem)
                        }
                    }

                    Text {
                        visible: root.hasDateRange
                        text: "-"
                        color: "#9CA3AF"
                        font.pixelSize: 12
                        verticalAlignment: Text.AlignVCenter
                    }

                    Text {
                        id: endDateTextItem
                        visible: root.hasDateRange
                        text: root.endDateText
                        font.pixelSize: 12
                        color: endDateMouse.containsMouse
                               || quickDatePopup.opened && (root.datePickerMode === "end" || root.datePickerMode === "range" && root.waitingForRangeEnd)
                               ? "#2563EB"
                               : root.isOverdue ? "#EF4444" : "#6B7280"
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        Layout.fillWidth: false
                        Layout.minimumWidth: 0
                        ToolTip.visible: endDateMouse.containsMouse
                        ToolTip.text: "修改截止日期"
                        Behavior on color { ColorAnimation { duration: 150 } }

                        MouseArea {
                            id: endDateMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.openDueDatePicker(endDateTextItem)
                        }
                    }
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
                        model.tags,
                        model.startDate
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

    Popup {
        id: quickDatePopup
        parent: Overlay.overlay
        readonly property var anchorItem: root.datePickerAnchor ? root.datePickerAnchor : dateText
        readonly property point anchorPoint: anchorItem && parent ? anchorItem.mapToItem(parent, 0, anchorItem.height + 6) : Qt.point(12, 12)
        x: parent ? Math.max(12, Math.min(parent.width - width - 12, anchorPoint.x)) : 0
        y: parent ? Math.max(12, Math.min(parent.height - implicitHeight - 12, anchorPoint.y)) : 0
        width: parent ? Math.max(236, Math.min(316, parent.width - 24)) : 280
        modal: false
        focus: true
        padding: 12
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        z: 30

        onOpened: dateIconCanvas.requestPaint()
        onClosed: dateIconCanvas.requestPaint()

        background: Rectangle {
            radius: 12
            color: "#FFFFFF"
            border.color: "#DADAE0"
            border.width: 1
        }

        contentItem: ColumnLayout {
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Button {
                    id: previousMonthButton
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 30
                    text: "<"
                    onClicked: root.shiftQuickCalendarMonth(-1)
                    background: Rectangle {
                        radius: 6
                        color: previousMonthButton.hovered ? "#F3F4F6" : "transparent"
                    }
                    contentItem: Text {
                        text: previousMonthButton.text
                        color: "#374151"
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: (root.datePickerMode === "range"
                           ? (root.waitingForRangeEnd ? "截止日期 · " : "起始日期 · ")
                           : root.datePickerMode === "start" ? "修改开始 · "
                           : root.datePickerMode === "end" ? "修改截止 · "
                           : "修改日期 · ")
                          + root.quickCalendarYear + "年 " + (root.quickCalendarMonth + 1) + "月"
                    color: "#111827"
                    font.pixelSize: 13
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                Button {
                    id: nextMonthButton
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 30
                    text: ">"
                    onClicked: root.shiftQuickCalendarMonth(1)
                    background: Rectangle {
                        radius: 6
                        color: nextMonthButton.hovered ? "#F3F4F6" : "transparent"
                    }
                    contentItem: Text {
                        text: nextMonthButton.text
                        color: "#374151"
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Repeater {
                    model: ["一", "二", "三", "四", "五", "六", "日"]

                    Text {
                        Layout.fillWidth: true
                        text: modelData
                        color: "#6B7280"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            Basic.MonthGrid {
                id: quickMonthGrid
                Layout.fillWidth: true
                month: root.quickCalendarMonth
                year: root.quickCalendarYear
                spacing: 4
                locale: Qt.locale("zh_CN")

                delegate: Rectangle {
                    required property var model

                    implicitWidth: Math.max(22, Math.floor((quickMonthGrid.width - 24) / 7))
                    implicitHeight: 32
                    radius: 6
                    opacity: model.month === quickMonthGrid.month ? 1 : 0.35

                    readonly property var cellDate: new Date(quickMonthGrid.year, model.month, model.day, 0, 0, 0)
                    readonly property bool isStart: root.sameDay(cellDate, root.quickStartDate)
                    readonly property bool isEnd: root.sameDay(cellDate, root.quickEndDate)
                    readonly property bool isDueSelection: root.datePickerMode === "end" && isEnd
                    readonly property bool isActiveRangeStart: (root.datePickerMode === "start" || root.datePickerMode === "range" && !root.waitingForRangeEnd) && isStart
                    readonly property bool isActiveRangeEnd: (root.datePickerMode === "end" || root.datePickerMode === "range" && root.waitingForRangeEnd) && isEnd
                    readonly property bool isActiveSelection: isDueSelection || isActiveRangeStart || isActiveRangeEnd
                    readonly property bool isInactiveEndpoint: (root.datePickerMode === "start" || root.datePickerMode === "end" || root.datePickerMode === "range") && (isStart || isEnd) && !isActiveSelection
                    readonly property bool isInRange: (root.datePickerMode === "start" || root.datePickerMode === "end" || root.datePickerMode === "range")
                                                      && cellDate.getTime() >= root.dayStart(root.quickStartDate).getTime()
                                                      && cellDate.getTime() <= root.dayStart(root.quickEndDate).getTime()
                    readonly property bool isToday: model.day === new Date().getDate()
                                                    && model.month === new Date().getMonth()
                                                    && quickMonthGrid.year === new Date().getFullYear()

                    color: isActiveSelection ? "#007AFF"
                           : isInactiveEndpoint ? "#D1E7FF"
                           : isInRange ? "#E5F1FF"
                           : quickDayMouse.containsMouse ? "#EFF6FF" : "transparent"
                    border.color: isInactiveEndpoint ? "#60A5FA" : !isActiveSelection && isToday ? "#93C5FD" : "transparent"
                    border.width: isInactiveEndpoint || !isActiveSelection && isToday ? 1 : 0

                    Text {
                        anchors.centerIn: parent
                        text: model.day
                        color: parent.isActiveSelection ? "#FFFFFF" : parent.isInactiveEndpoint ? "#1D4ED8" : "#111827"
                        font.pixelSize: 13
                        font.bold: parent.isActiveSelection || parent.isInactiveEndpoint || parent.isToday
                    }

                    MouseArea {
                        id: quickDayMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.applyQuickDate(new Date(quickMonthGrid.year, model.month, model.day))
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    text: "今天"
                    onClicked: root.applyQuickDate(new Date())
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    text: "明天"
                    onClicked: {
                        const tomorrow = new Date()
                        tomorrow.setDate(tomorrow.getDate() + 1)
                        root.applyQuickDate(tomorrow)
                    }
                }
            }
        }
    }
}
