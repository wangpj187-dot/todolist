import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic 6.3 as Basic
import QtQuick.Layouts 1.15
import TodoApp 1.0

Dialog {
    id: root

    parent: Overlay.overlay
    readonly property real overlayWidth: parent && parent.width > 0 ? parent.width : 560
    readonly property real overlayHeight: parent && parent.height > 0 ? parent.height : 720
    readonly property real dialogMargin: overlayWidth < 340 ? 8 : 12
    readonly property bool compact: width < 470

    width: Math.max(260, Math.min(560, overlayWidth - dialogMargin * 2))
    height: Math.max(360, Math.min(720, overlayHeight - dialogMargin * 2))
    x: Math.round((overlayWidth - width) / 2)
    y: Math.max(dialogMargin, Math.round((overlayHeight - height) / 2))
    modal: true
    padding: 0
    closePolicy: Popup.CloseOnEscape

    property var selectedStartDate: new Date()
    property var selectedEndDate: new Date()
    property string calendarTarget: "start"
    property int calendarMonth: selectedStartDate.getMonth()
    property int calendarYear: selectedStartDate.getFullYear()
    property var dateItems: []
    readonly property string flagTag: "旗标"

    signal editRequested(var item)

    function openForDate(date) {
        const day = normalizeDate(date)
        setSelectedRange(day, day)
        open()
    }

    function isValidDate(value) {
        return value instanceof Date && !isNaN(value.getTime())
    }

    function normalizeDate(value) {
        const date = isValidDate(value) ? value : new Date()
        return new Date(date.getFullYear(), date.getMonth(), date.getDate(), 0, 0, 0)
    }

    function setCalendarDate(date) {
        const normalized = normalizeDate(date)
        calendarMonth = normalized.getMonth()
        calendarYear = normalized.getFullYear()
    }

    function setSelectedRange(startDate, endDate) {
        let start = normalizeDate(startDate)
        let end = normalizeDate(endDate)
        if (end < start) {
            const tmp = start
            start = end
            end = tmp
        }
        selectedStartDate = start
        selectedEndDate = end
        setCalendarDate(calendarTarget === "end" ? selectedEndDate : selectedStartDate)
        refresh()
    }

    function refresh() {
        dateItems = todoService.getTodosForDateRangeFromQml(selectedStartDate, selectedEndDate)
    }

    function pad2(value) {
        return ("0" + value).slice(-2)
    }

    function formatDate(date) {
        return date.getFullYear() + "-" + pad2(date.getMonth() + 1) + "-" + pad2(date.getDate())
    }

    function formatDisplayDate(date) {
        return date.getFullYear() + "." + pad2(date.getMonth() + 1) + "." + pad2(date.getDate())
    }

    function formatRange() {
        return formatDisplayDate(selectedStartDate) + "-" + formatDisplayDate(selectedEndDate)
    }

    function formatDateTime(value) {
        if (!(value instanceof Date) || isNaN(value.getTime()))
            return ""
        return formatDate(value) + " " + pad2(value.getHours()) + ":" + pad2(value.getMinutes())
    }

    function shiftCalendarMonth(delta) {
        const next = new Date(calendarYear, calendarMonth + delta, 1)
        calendarYear = next.getFullYear()
        calendarMonth = next.getMonth()
    }

    function selectedDayCount() {
        const msPerDay = 24 * 60 * 60 * 1000
        return Math.max(1, Math.round((selectedEndDate.getTime() - selectedStartDate.getTime()) / msPerDay) + 1)
    }

    function shiftSelectedRange(direction) {
        const days = selectedDayCount() * direction
        const start = new Date(selectedStartDate)
        const end = new Date(selectedEndDate)
        start.setDate(start.getDate() + days)
        end.setDate(end.getDate() + days)
        setSelectedRange(start, end)
    }

    function openCalendar(target) {
        calendarTarget = target
        setCalendarDate(target === "end" ? selectedEndDate : selectedStartDate)
        calendarPopup.open()
    }

    function setCalendarTargetDate(value) {
        const date = normalizeDate(value)
        if (calendarTarget === "start")
            setSelectedRange(date, selectedEndDate)
        else
            setSelectedRange(selectedStartDate, date)
    }

    function countByStatus(status) {
        let count = 0
        for (let i = 0; i < dateItems.length; i++) {
            if (dateItems[i].status === status)
                count++
        }
        return count
    }

    function countOpen() {
        let count = 0
        for (let i = 0; i < dateItems.length; i++) {
            if (dateItems[i].isOpen)
                count++
        }
        return count
    }

    function countOverdue() {
        let count = 0
        for (let i = 0; i < dateItems.length; i++) {
            if (dateItems[i].isOverdue)
                count++
        }
        return count
    }

    function relationText(item) {
        const labels = item.relationLabels || []
        const result = []
        for (let i = 0; i < labels.length; i++)
            result.push(labels[i])
        return result.join(" / ")
    }

    function visibleTags(item) {
        const tags = item.tags || []
        const result = []
        for (let i = 0; i < tags.length; i++) {
            const tag = ("" + tags[i]).trim()
            if (tag.length > 0 && tag !== flagTag)
                result.push("#" + tag)
        }
        return result.join(" ")
    }

    function priorityColor(priority) {
        switch (priority) {
        case Todo.Low: return "#16A34A"
        case Todo.Medium: return "#CA8A04"
        case Todo.High: return "#EA580C"
        case Todo.Urgent: return "#DC2626"
        default: return "#6B7280"
        }
    }

    function statusColor(status) {
        switch (status) {
        case Todo.Completed: return "#16A34A"
        case Todo.Cancelled: return "#6B7280"
        case Todo.InProgress: return "#007AFF"
        default: return "#CA8A04"
        }
    }

    Connections {
        target: todoService
        function onDataChanged() {
            if (root.opened)
                root.refresh()
        }
    }

    background: Rectangle {
        color: "#FFFFFF"
        radius: 14
        border.color: "#DADAE0"
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            color: "#F5F5F7"
            radius: 14
            clip: true

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 8
                color: parent.color
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 14
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: "按日期查看"
                        color: "#111827"
                        font.pixelSize: 19
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: root.formatRange() + " · " + root.dateItems.length + " 项"
                        color: "#6B7280"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Button {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    text: "×"
                    font.pixelSize: 16
                    onClicked: root.close()
                    background: Rectangle {
                        radius: 17
                        color: parent.hovered ? "#E5E5EA" : "transparent"
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#6B7280"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 14
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    id: previousDayButton
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    text: "<"
                    onClicked: root.shiftSelectedRange(-1)
                }

                Button {
                    id: rangeStartButton
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: 34
                    text: root.formatDisplayDate(root.selectedStartDate)
                    onClicked: root.openCalendar("start")
                    background: Rectangle {
                        radius: 9
                        color: calendarPopup.opened && root.calendarTarget === "start" ? "#E5F1FF" : "#FFFFFF"
                        border.color: calendarPopup.opened && root.calendarTarget === "start" ? "#007AFF" : "#DADAE0"
                        border.width: 1
                    }
                    contentItem: Text {
                        text: rangeStartButton.text
                        color: "#111827"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }

                Text {
                    Layout.preferredWidth: 14
                    text: "-"
                    color: "#6B7280"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                Button {
                    id: rangeEndButton
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: 34
                    text: root.formatDisplayDate(root.selectedEndDate)
                    onClicked: root.openCalendar("end")
                    background: Rectangle {
                        radius: 9
                        color: calendarPopup.opened && root.calendarTarget === "end" ? "#E5F1FF" : "#FFFFFF"
                        border.color: calendarPopup.opened && root.calendarTarget === "end" ? "#007AFF" : "#DADAE0"
                        border.width: 1
                    }
                    contentItem: Text {
                        text: rangeEndButton.text
                        color: "#111827"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }

                Button {
                    id: nextDayButton
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    text: ">"
                    onClicked: root.shiftSelectedRange(1)
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.compact ? 2 : 4
                columnSpacing: 8
                rowSpacing: 8

                Repeater {
                    model: [
                        { label: "相关", value: root.dateItems.length, color: "#007AFF" },
                        { label: "未完成", value: root.countOpen(), color: "#CA8A04" },
                        { label: "已完成", value: root.countByStatus(Todo.Completed), color: "#16A34A" },
                        { label: "逾期", value: root.countOverdue(), color: "#DC2626" }
                    ]

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 58
                        radius: 8
                        color: "#F9FAFB"
                        border.color: "#E5E7EB"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 1

                            Text {
                                text: modelData.value
                                color: modelData.color
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                Layout.fillWidth: true
                            }

                            Text {
                                text: modelData.label
                                color: "#6B7280"
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }

            ListView {
                id: dateList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 8
                model: root.dateItems
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: Rectangle {
                    width: dateList.width
                    implicitHeight: itemLayout.implicitHeight + 18
                    radius: 8
                    color: modelData.isCompleted ? "#FAFAFA" : "#FFFFFF"
                    border.color: modelData.isOverdue ? "#FCA5A5" : "#E5E7EB"
                    border.width: 1

                    ColumnLayout {
                        id: itemLayout
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 9
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Rectangle {
                                Layout.preferredWidth: 4
                                Layout.preferredHeight: 30
                                radius: 2
                                color: root.priorityColor(modelData.priority)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.title
                                    color: modelData.isCompleted ? "#9CA3AF" : "#111827"
                                    font.pixelSize: 14
                                    font.weight: modelData.isCompleted ? Font.Normal : Font.Medium
                                    font.strikeout: modelData.isCompleted
                                    elide: Text.ElideRight
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: root.relationText(modelData)
                                          + (modelData.dueDate instanceof Date && !isNaN(modelData.dueDate.getTime())
                                             ? " · 截止 " + root.formatDateTime(modelData.dueDate) : "")
                                    color: modelData.isOverdue ? "#DC2626" : "#6B7280"
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: statusLabel.implicitWidth + 18
                                Layout.preferredHeight: 24
                                radius: 12
                                color: "#F9FAFB"
                                border.color: root.statusColor(modelData.status)
                                border.width: 1

                                Text {
                                    id: statusLabel
                                    anchors.centerIn: parent
                                    text: modelData.statusLabel
                                    color: root.statusColor(modelData.status)
                                    font.pixelSize: 11
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: modelData.description.length > 0
                            text: modelData.description
                            color: "#4B5563"
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                Layout.fillWidth: true
                                text: (modelData.isFlagged ? "旗标 " : "") + root.visibleTags(modelData)
                                color: modelData.isFlagged ? "#C2410C" : "#1D4ED8"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }

                            Button {
                                Layout.preferredWidth: 58
                                Layout.preferredHeight: 28
                                text: modelData.status === Todo.Completed ? "重开" : "完成"
                                font.pixelSize: 12
                                onClicked: {
                                    todoService.setTodoStatusFromQml(modelData.id,
                                                                     modelData.status === Todo.Completed
                                                                     ? Todo.InProgress : Todo.Completed)
                                    root.refresh()
                                }
                            }

                            Button {
                                Layout.preferredWidth: 58
                                Layout.preferredHeight: 28
                                text: "编辑"
                                font.pixelSize: 12
                                onClicked: root.editRequested(modelData)
                            }
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: dateList.count === 0
                    text: root.selectedDayCount() === 1 ? "这一天暂无相关任务" : "这段时间暂无相关任务"
                    color: "#9CA3AF"
                    font.pixelSize: 14
                }
            }
        }
    }

    Popup {
        id: calendarPopup
        parent: root.contentItem
        readonly property var anchorButton: root.calendarTarget === "end" ? rangeEndButton : rangeStartButton
        readonly property real preferredX: anchorButton.mapToItem(root.contentItem, 0, 0).x
        readonly property real preferredY: anchorButton.mapToItem(root.contentItem, 0, anchorButton.height + 6).y
        x: Math.max(12, Math.min(root.width - width - 12, preferredX))
        y: Math.max(12, Math.min(root.height - implicitHeight - 12, preferredY))
        width: Math.max(216, Math.min(316, root.width - 24))
        modal: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 12
        z: 20

        background: Rectangle {
            color: "#FFFFFF"
            radius: 14
            border.color: "#DADAE0"
            border.width: 1
        }

        contentItem: ColumnLayout {
            spacing: 10

            RowLayout {
                Layout.fillWidth: true

                Button {
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 30
                    text: "<"
                    onClicked: root.shiftCalendarMonth(-1)
                }

                Text {
                    Layout.fillWidth: true
                    text: root.calendarYear + "年 " + (root.calendarMonth + 1) + "月"
                    color: "#111827"
                    font.pixelSize: 14
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Button {
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 30
                    text: ">"
                    onClicked: root.shiftCalendarMonth(1)
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
                id: monthGrid
                Layout.fillWidth: true
                month: root.calendarMonth
                year: root.calendarYear
                spacing: 4
                locale: Qt.locale("zh_CN")

                delegate: Rectangle {
                    required property var model

                    implicitWidth: Math.max(22, Math.floor((monthGrid.width - 24) / 7))
                    implicitHeight: 32
                    radius: 6
                    opacity: model.month === monthGrid.month ? 1 : 0.35

                    readonly property var cellDate: new Date(monthGrid.year, model.month, model.day, 0, 0, 0)
                    readonly property bool isStart: cellDate.getTime() === root.selectedStartDate.getTime()
                    readonly property bool isEnd: cellDate.getTime() === root.selectedEndDate.getTime()
                    readonly property bool isSelected: isStart || isEnd
                    readonly property bool isInRange: cellDate >= root.selectedStartDate && cellDate <= root.selectedEndDate
                    readonly property bool isToday: model.day === new Date().getDate()
                                                    && model.month === new Date().getMonth()
                                                    && monthGrid.year === new Date().getFullYear()

                    color: isSelected ? "#007AFF" : isInRange ? "#E5F1FF" : dayMouse.containsMouse ? "#EFF6FF" : "transparent"
                    border.color: !isSelected && isToday ? "#93C5FD" : "transparent"
                    border.width: !isSelected && isToday ? 1 : 0

                    Text {
                        anchors.centerIn: parent
                        text: model.day
                        color: parent.isSelected ? "#FFFFFF" : "#111827"
                        font.pixelSize: 13
                        font.bold: parent.isSelected || parent.isToday
                    }

                    MouseArea {
                        id: dayMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.setCalendarTargetDate(new Date(monthGrid.year, model.month, model.day, 0, 0, 0))
                            calendarPopup.close()
                        }
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
                    onClicked: {
                        root.setCalendarTargetDate(new Date())
                        calendarPopup.close()
                    }
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    text: "明天"
                    onClicked: {
                        const tomorrow = new Date()
                        tomorrow.setDate(tomorrow.getDate() + 1)
                        root.setCalendarTargetDate(tomorrow)
                        calendarPopup.close()
                    }
                }
            }
        }
    }
}
