import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic 6.3 as Basic
import QtQuick.Layouts 1.15
import TodoApp 1.0

Item {
    id: root

    property var viewDate: new Date()
    property var selectedDate: new Date()
    property var selectedStartDate: new Date()
    property var selectedEndDate: new Date()
    property var monthCells: []
    property var selectedItems: []
    property string calendarTarget: "start"
    property string rangeClickTarget: "start"
    property string highlightedRangeTarget: "start"
    property int calendarMonth: selectedStartDate.getMonth()
    property int calendarYear: selectedStartDate.getFullYear()
    readonly property bool compact: width < 760
    readonly property string calendarTitle: formatMonth(viewDate)

    signal editRequested(var item)
    signal createRequested(var date)

    function pad2(value) {
        return ("0" + value).slice(-2)
    }

    function isValidDate(value) {
        return value && typeof value.getTime === "function" && !isNaN(value.getTime())
    }

    function normalizeDate(value) {
        const date = isValidDate(value) ? value : new Date()
        return new Date(date.getFullYear(), date.getMonth(), date.getDate(), 0, 0, 0)
    }

    function sameDay(left, right) {
        const a = normalizeDate(left)
        const b = normalizeDate(right)
        return a.getFullYear() === b.getFullYear()
                && a.getMonth() === b.getMonth()
                && a.getDate() === b.getDate()
    }

    function formatMonth(date) {
        const value = normalizeDate(date)
        return value.getFullYear() + " 年 " + pad2(value.getMonth() + 1) + " 月"
    }

    function formatShortDate(date) {
        const value = normalizeDate(date)
        return pad2(value.getMonth() + 1) + "." + pad2(value.getDate())
    }

    function formatDateLabel(date) {
        const value = normalizeDate(date)
        return value.getFullYear() + "-" + pad2(value.getMonth() + 1) + "-" + pad2(value.getDate())
    }

    function formatDisplayDate(date) {
        const value = normalizeDate(date)
        return value.getFullYear() + "." + pad2(value.getMonth() + 1) + "." + pad2(value.getDate())
    }

    function selectedDayCount() {
        const msPerDay = 24 * 60 * 60 * 1000
        return Math.max(1, Math.round((normalizeDate(selectedEndDate).getTime()
                                      - normalizeDate(selectedStartDate).getTime()) / msPerDay) + 1)
    }

    function selectedRangeTitle() {
        if (selectedDayCount() === 1)
            return formatDateLabel(selectedStartDate)
        return formatDateLabel(selectedStartDate) + " - " + formatDateLabel(selectedEndDate)
    }

    function formatDateTime(value) {
        if (!isValidDate(value))
            return ""
        return formatDateLabel(value) + " " + pad2(value.getHours()) + ":" + pad2(value.getMinutes())
    }

    function formatRange(item) {
        const hasStart = isValidDate(item.startDate)
        const hasDue = isValidDate(item.dueDate)
        if (hasStart && hasDue) {
            const startText = formatShortDate(item.startDate)
            const endText = formatShortDate(item.dueDate)
            return startText === endText ? endText : startText + " - " + endText
        }
        if (hasDue)
            return formatDateTime(item.dueDate)
        if (hasStart)
            return formatShortDate(item.startDate)
        return ""
    }

    function visibleTags(item) {
        const tags = item.tags || []
        const result = []
        for (let i = 0; i < tags.length; i++) {
            const tag = ("" + tags[i]).trim()
            if (tag.length > 0 && tag !== "旗标")
                result.push("#" + tag)
        }
        return result.join(" ")
    }

    function relationText(item) {
        const labels = item.relationLabels || []
        const result = []
        for (let i = 0; i < labels.length; i++)
            result.push(labels[i])
        return result.join(" / ")
    }

    function taskColor(item) {
        if (item.isCompleted)
            return "#16A34A"
        if (item.isOverdue)
            return "#EF4444"
        if (item.status === Todo.InProgress)
            return "#007AFF"
        switch (item.priority) {
        case Todo.Urgent: return "#DC2626"
        case Todo.High: return "#F97316"
        case Todo.Medium: return "#CA8A04"
        default: return "#64748B"
        }
    }

    function taskFill(item) {
        if (item.isCompleted)
            return "#DCFCE7"
        if (item.isOverdue)
            return "#FEE2E2"
        if (item.status === Todo.InProgress)
            return "#DBEAFE"
        switch (item.priority) {
        case Todo.Urgent: return "#FEE2E2"
        case Todo.High: return "#FFEDD5"
        case Todo.Medium: return "#FEF9C3"
        default: return "#F1F5F9"
        }
    }

    function countWhere(items, predicateName) {
        let count = 0
        for (let i = 0; i < items.length; i++) {
            if (items[i][predicateName])
                count++
        }
        return count
    }

    function countStatus(items, status) {
        let count = 0
        for (let i = 0; i < items.length; i++) {
            if (items[i].status === status)
                count++
        }
        return count
    }

    function previewItems(items) {
        const result = []
        const maxCount = compact ? 2 : 3
        for (let i = 0; i < items.length && i < maxCount; i++)
            result.push(items[i])
        return result
    }

    function dateInSelectedRange(date) {
        const value = normalizeDate(date).getTime()
        return value >= normalizeDate(selectedStartDate).getTime()
                && value <= normalizeDate(selectedEndDate).getTime()
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
            const previousStart = start
            start = end
            end = previousStart
        }

        selectedStartDate = start
        selectedEndDate = end
        selectedDate = start

        if (start.getFullYear() !== viewDate.getFullYear() || start.getMonth() !== viewDate.getMonth())
            viewDate = new Date(start.getFullYear(), start.getMonth(), 1)

        setCalendarDate(calendarTarget === "end" ? selectedEndDate : selectedStartDate)
        buildMonth()
    }

    function selectDate(date) {
        const selected = normalizeDate(date)
        if (rangeClickTarget === "end") {
            calendarTarget = "end"
            setSelectedRange(selectedStartDate, selected)
            highlightedRangeTarget = "end"
            rangeClickTarget = "start"
        } else {
            calendarTarget = "start"
            setSelectedRange(selected, selected)
            highlightedRangeTarget = "start"
            rangeClickTarget = "end"
        }
    }

    function showToday() {
        const today = normalizeDate(new Date())
        viewDate = today
        setSelectedRange(today, today)
        highlightedRangeTarget = "start"
        rangeClickTarget = "start"
    }

    function showCurrentWeek() {
        const today = normalizeDate(new Date())
        const mondayOffset = today.getDay() === 0 ? -6 : 1 - today.getDay()
        const start = new Date(today)
        start.setDate(today.getDate() + mondayOffset)
        const end = new Date(start)
        end.setDate(start.getDate() + 6)
        setSelectedRange(start, end)
        highlightedRangeTarget = "start"
        rangeClickTarget = "start"
    }

    function showCurrentMonth() {
        const today = normalizeDate(new Date())
        const start = new Date(today.getFullYear(), today.getMonth(), 1)
        const end = new Date(today.getFullYear(), today.getMonth() + 1, 0)
        setSelectedRange(start, end)
        highlightedRangeTarget = "start"
        rangeClickTarget = "start"
    }

    function shiftSelectedRange(direction) {
        const days = selectedDayCount() * direction
        const start = normalizeDate(selectedStartDate)
        const end = normalizeDate(selectedEndDate)
        start.setDate(start.getDate() + days)
        end.setDate(end.getDate() + days)
        setSelectedRange(start, end)
        highlightedRangeTarget = "start"
        rangeClickTarget = "start"
    }

    function shiftMonth(delta) {
        const next = new Date(viewDate.getFullYear(), viewDate.getMonth() + delta, 1)
        viewDate = next
        buildMonth()
    }

    function shiftCalendarMonth(delta) {
        const next = new Date(calendarYear, calendarMonth + delta, 1)
        calendarMonth = next.getMonth()
        calendarYear = next.getFullYear()
    }

    function openCalendar(target) {
        calendarTarget = target
        rangeClickTarget = target
        highlightedRangeTarget = target
        setCalendarDate(target === "end" ? selectedEndDate : selectedStartDate)
        calendarPopup.open()
    }

    function setCalendarTargetDate(date) {
        const normalized = normalizeDate(date)
        if (calendarTarget === "end") {
            setSelectedRange(selectedStartDate, normalized)
            highlightedRangeTarget = "end"
            rangeClickTarget = "start"
        } else {
            setSelectedRange(normalized, selectedEndDate)
            highlightedRangeTarget = "start"
            rangeClickTarget = "end"
        }
    }

    function refreshSelectedItems() {
        selectedItems = todoService.getTodosForDateRangeFromQml(selectedStartDate, selectedEndDate)
    }

    function buildMonth() {
        const monthStart = new Date(viewDate.getFullYear(), viewDate.getMonth(), 1)
        const firstVisible = new Date(monthStart)
        firstVisible.setDate(monthStart.getDate() - monthStart.getDay())

        const cells = []
        for (let i = 0; i < 42; i++) {
            const date = new Date(firstVisible)
            date.setDate(firstVisible.getDate() + i)
            const items = todoService.getTodosForDateFromQml(date)
            cells.push({
                date: date,
                day: date.getDate(),
                inMonth: date.getMonth() === viewDate.getMonth(),
                isToday: sameDay(date, new Date()),
                isSelected: sameDay(date, selectedDate),
                isRangeEdge: sameDay(date, selectedStartDate) || sameDay(date, selectedEndDate),
                isInSelectedRange: dateInSelectedRange(date),
                items: items,
                openCount: countWhere(items, "isOpen"),
                completedCount: countWhere(items, "isCompleted"),
                overdueCount: countWhere(items, "isOverdue")
            })
        }
        monthCells = cells
        refreshSelectedItems()
    }

    Component.onCompleted: buildMonth()

    Connections {
        target: todoService
        function onDataChanged() {
            buildMonth()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 12
        spacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    id: todayButton
                    Layout.preferredWidth: 58
                    Layout.preferredHeight: 32
                    text: "今天"
                    font.pixelSize: 13
                    onClicked: root.showToday()
                    background: Rectangle {
                        radius: 8
                        color: todayButton.down ? "#D1E7FF" : todayButton.hovered ? "#E5F1FF" : "#FFFFFF"
                        border.color: todayButton.hovered || todayButton.down ? "#93C5FD" : "#DADAE0"
                        border.width: 1
                    }
                    contentItem: Text {
                        text: todayButton.text
                        color: "#374151"
                        font: todayButton.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    id: prevMonthButton
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    text: "<"
                    font.pixelSize: 16
                    onClicked: root.shiftMonth(-1)
                    background: Rectangle {
                        radius: 8
                        color: prevMonthButton.hovered ? "#FFFFFF" : "transparent"
                        border.color: prevMonthButton.hovered ? "#DADAE0" : "transparent"
                        border.width: 1
                    }
                    contentItem: Text {
                        text: prevMonthButton.text
                        color: "#374151"
                        font: prevMonthButton.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    id: nextMonthButton
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    text: ">"
                    font.pixelSize: 16
                    onClicked: root.shiftMonth(1)
                    background: Rectangle {
                        radius: 8
                        color: nextMonthButton.hovered ? "#FFFFFF" : "transparent"
                        border.color: nextMonthButton.hovered ? "#DADAE0" : "transparent"
                        border.width: 1
                    }
                    contentItem: Text {
                        text: nextMonthButton.text
                        color: "#374151"
                        font: nextMonthButton.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: root.calendarTitle
                    color: "#111827"
                    font.pixelSize: root.compact ? 16 : 18
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                Button {
                    id: addOnDayButton
                    Layout.preferredWidth: root.compact ? 34 : 82
                    Layout.preferredHeight: 32
                    text: root.compact ? "+" : "+ 当天"
                    font.pixelSize: 13
                    ToolTip.visible: hovered
                    ToolTip.text: "在选中日期新建任务"
                    onClicked: root.createRequested(root.selectedDate)
                    background: Rectangle {
                        radius: 8
                        color: addOnDayButton.down ? "#0062CC" : addOnDayButton.hovered ? "#0A84FF" : "#007AFF"
                    }
                    contentItem: Text {
                        text: addOnDayButton.text
                        color: "#FFFFFF"
                        font: addOnDayButton.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                columns: 7
                columnSpacing: 0
                rowSpacing: 0

                Repeater {
                    model: ["周日", "周一", "周二", "周三", "周四", "周五", "周六"]

                    Text {
                        Layout.fillWidth: true
                        text: modelData
                        color: "#6B7280"
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            GridLayout {
                id: monthGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: 7
                columnSpacing: 0
                rowSpacing: 0

                Repeater {
                    model: root.monthCells

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: root.compact ? 38 : 58
                        Layout.minimumHeight: root.compact ? 68 : 92
                        color: modelData.isRangeEdge ? "#E5F1FF"
                               : modelData.isInSelectedRange ? "#F3F8FF"
                               : modelData.inMonth ? "#FFFFFF" : "#F7F7FA"
                        border.color: modelData.isRangeEdge ? "#007AFF"
                                      : modelData.isInSelectedRange ? "#BFDBFE" : "#E5E7EB"
                        border.width: modelData.isRangeEdge ? 1.4 : 1

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: root.selectDate(modelData.date)
                            onDoubleClicked: root.createRequested(modelData.date)
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: root.compact ? 5 : 7
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Text {
                                    Layout.fillWidth: true
                                    visible: !modelData.isToday
                                    text: modelData.day
                                    color: modelData.isToday ? "#FFFFFF"
                                           : modelData.inMonth ? "#374151" : "#A1A1AA"
                                    font.pixelSize: 12
                                    font.weight: Font.DemiBold
                                    verticalAlignment: Text.AlignVCenter
                                }

                                Rectangle {
                                    visible: modelData.isToday
                                    Layout.preferredWidth: 22
                                    Layout.preferredHeight: 22
                                    radius: 11
                                    color: "#007AFF"
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.day
                                        color: "#FFFFFF"
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                    }
                                }

                                Text {
                                    visible: !modelData.isToday && modelData.items.length > 0
                                    text: modelData.items.length
                                    color: modelData.overdueCount > 0 ? "#DC2626" : "#007AFF"
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                }
                            }

                            Column {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 3
                                clip: true

                                Repeater {
                                    model: root.previewItems(modelData.items)

                                    delegate: Rectangle {
                                        width: parent.width
                                        height: root.compact ? 16 : 18
                                        radius: 4
                                        color: root.taskFill(modelData)
                                        border.color: root.taskColor(modelData)
                                        border.width: 1

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 5
                                            anchors.rightMargin: 5
                                            spacing: 4

                                            Rectangle {
                                                Layout.preferredWidth: 4
                                                Layout.preferredHeight: 10
                                                radius: 2
                                                color: root.taskColor(modelData)
                                            }

                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData.title || ""
                                                color: "#1F2937"
                                                font.pixelSize: 10
                                                elide: Text.ElideRight
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                        }
                                    }
                                }

                                Text {
                                    visible: modelData.items.length > root.previewItems(modelData.items).length
                                    text: "+" + (modelData.items.length - root.previewItems(modelData.items).length)
                                    color: "#6B7280"
                                    font.pixelSize: 10
                                    horizontalAlignment: Text.AlignRight
                                    width: parent.width
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: detailPane
            Layout.preferredWidth: root.compact ? 210 : 270
            Layout.fillHeight: true
            radius: 8
            color: "#FFFFFF"
            border.color: "#DADAE0"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            Layout.fillWidth: true
                            text: root.selectedRangeTitle()
                            color: "#111827"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: root.selectedDayCount() === 1
                                  ? root.selectedItems.length + " 项任务"
                                  : root.selectedDayCount() + " 天 · " + root.selectedItems.length + " 项任务"
                            color: "#6B7280"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }

                    Button {
                        id: sideAddButton
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 30
                        text: "+"
                        font.pixelSize: 18
                        ToolTip.visible: hovered
                        ToolTip.text: "在开始日期新建任务"
                        onClicked: root.createRequested(root.selectedDate)
                        background: Rectangle {
                            radius: 8
                            color: sideAddButton.down ? "#D1E7FF" : sideAddButton.hovered ? "#E5F1FF" : "#F5F5F7"
                            border.color: "#DADAE0"
                            border.width: 1
                        }
                        contentItem: Text {
                            text: sideAddButton.text
                            color: "#007AFF"
                            font: sideAddButton.font
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    Text {
                        Layout.fillWidth: true
                        text: "查看时间段"
                        color: "#6B7280"
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Button {
                            id: rangeStartButton
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.preferredHeight: 32
                            text: root.formatDisplayDate(root.selectedStartDate)
                            font.pixelSize: 12
                            onClicked: root.openCalendar("start")
                            background: Rectangle {
                                radius: 8
                                color: root.highlightedRangeTarget === "start" ? "#E5F1FF" : "#FFFFFF"
                                border.color: root.highlightedRangeTarget === "start" ? "#007AFF" : "#DADAE0"
                                border.width: 1
                            }
                            contentItem: Text {
                                text: rangeStartButton.text
                                color: "#111827"
                                font: rangeStartButton.font
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }

                        Text {
                            text: "-"
                            color: "#6B7280"
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        Button {
                            id: rangeEndButton
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.preferredHeight: 32
                            text: root.formatDisplayDate(root.selectedEndDate)
                            font.pixelSize: 12
                            onClicked: root.openCalendar("end")
                            background: Rectangle {
                                radius: 8
                                color: root.highlightedRangeTarget === "end" ? "#E5F1FF" : "#FFFFFF"
                                border.color: root.highlightedRangeTarget === "end" ? "#007AFF" : "#DADAE0"
                                border.width: 1
                            }
                            contentItem: Text {
                                text: rangeEndButton.text
                                color: "#111827"
                                font: rangeEndButton.font
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Button {
                            id: previousRangeButton
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 30
                            text: "<"
                            font.pixelSize: 14
                            onClicked: root.shiftSelectedRange(-1)
                        }

                        Button {
                            id: todayRangeButton
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            text: "今天"
                            font.pixelSize: 12
                            onClicked: root.showToday()
                        }

                        Button {
                            id: weekRangeButton
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            text: "本周"
                            font.pixelSize: 12
                            onClicked: root.showCurrentWeek()
                        }

                        Button {
                            id: monthRangeButton
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            text: "本月"
                            font.pixelSize: 12
                            onClicked: root.showCurrentMonth()
                        }

                        Button {
                            id: nextRangeButton
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 30
                            text: ">"
                            font.pixelSize: 14
                            onClicked: root.shiftSelectedRange(1)
                        }
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 1
                    rowSpacing: 7

                    Repeater {
                        model: [
                            { label: "待处理", value: root.countStatus(root.selectedItems, Todo.Pending), color: "#CA8A04" },
                            { label: "进行中", value: root.countStatus(root.selectedItems, Todo.InProgress), color: "#007AFF" },
                            { label: "已完成", value: root.countStatus(root.selectedItems, Todo.Completed), color: "#16A34A" }
                        ]

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Rectangle {
                                Layout.preferredWidth: 7
                                Layout.preferredHeight: 7
                                radius: 4
                                color: modelData.color
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.label
                                color: "#6B7280"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }

                            Text {
                                text: modelData.value
                                color: "#374151"
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#ECECF1"
                }

                ListView {
                    id: selectedTaskList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: root.selectedItems
                    spacing: 8
                    clip: true
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: Rectangle {
                        width: selectedTaskList.width
                        height: Math.max(taskContent.implicitHeight + 18, 70)
                        radius: 8
                        color: "#F9FAFB"
                        border.color: modelData.isOverdue ? "#FCA5A5" : "#E5E7EB"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 9
                            spacing: 8

                            Rectangle {
                                Layout.preferredWidth: 4
                                Layout.fillHeight: true
                                radius: 2
                                color: root.taskColor(modelData)
                            }

                            ColumnLayout {
                                id: taskContent
                                Layout.fillWidth: true
                                spacing: 4

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.title || ""
                                        color: modelData.isCompleted ? "#9CA3AF" : "#111827"
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                        font.strikeout: modelData.isCompleted
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        text: modelData.statusLabel || ""
                                        color: root.taskColor(modelData)
                                        font.pixelSize: 11
                                        font.weight: Font.DemiBold
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    visible: (modelData.description || "").length > 0
                                    text: modelData.description || ""
                                    color: "#6B7280"
                                    font.pixelSize: 11
                                    maximumLineCount: 2
                                    wrapMode: Text.Wrap
                                    elide: Text.ElideRight
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: [root.relationText(modelData), root.formatRange(modelData), root.visibleTags(modelData)]
                                          .filter(function (value) { return value && value.length > 0 }).join(" · ")
                                    color: "#8E8E93"
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }

                            Button {
                                id: editTaskButton
                                Layout.preferredWidth: 34
                                Layout.preferredHeight: 28
                                text: "编"
                                font.pixelSize: 12
                                onClicked: root.editRequested(modelData)
                                background: Rectangle {
                                    radius: 7
                                    color: editTaskButton.hovered ? "#EFF6FF" : "transparent"
                                }
                                contentItem: Text {
                                    text: editTaskButton.text
                                    color: editTaskButton.hovered ? "#2563EB" : "#6B7280"
                                    font: editTaskButton.font
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }

                    Column {
                        visible: selectedTaskList.count === 0
                        anchors.centerIn: parent
                        spacing: 8

	                        Text {
	                            text: root.selectedDayCount() === 1 ? "当天暂无任务" : "这段时间暂无任务"
	                            color: "#9CA3AF"
	                            font.pixelSize: 14
	                            anchors.horizontalCenter: parent.horizontalCenter
	                        }

                        Button {
                            id: emptyAddButton
                            text: "+ 新建"
                            font.pixelSize: 12
                            anchors.horizontalCenter: parent.horizontalCenter
                            onClicked: root.createRequested(root.selectedDate)
                            background: Rectangle {
                                radius: 8
                                color: emptyAddButton.down ? "#D1E7FF" : emptyAddButton.hovered ? "#E5F1FF" : "#FFFFFF"
                                border.color: "#DADAE0"
                                border.width: 1
                            }
                            contentItem: Text {
                                text: emptyAddButton.text
                                color: "#007AFF"
                                font: emptyAddButton.font
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: calendarPopup
        parent: root
        readonly property var anchorButton: root.calendarTarget === "end" ? rangeEndButton : rangeStartButton
        readonly property real preferredX: anchorButton.mapToItem(root, 0, 0).x
        readonly property real preferredY: anchorButton.mapToItem(root, 0, anchorButton.height + 6).y

        x: Math.max(12, Math.min(root.width - width - 12, preferredX))
        y: Math.max(12, Math.min(root.height - implicitHeight - 12, preferredY))
        width: Math.max(240, Math.min(320, root.width - 24))
        padding: 12
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
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
                    elide: Text.ElideRight
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
                id: rangeMonthGrid
                Layout.fillWidth: true
                month: root.calendarMonth
                year: root.calendarYear
                spacing: 4
                locale: Qt.locale("zh_CN")

                delegate: Rectangle {
                    required property var model

                    implicitWidth: Math.max(22, Math.floor((rangeMonthGrid.width - 24) / 7))
                    implicitHeight: 32
                    radius: 6
                    opacity: model.month === rangeMonthGrid.month ? 1 : 0.35

                    readonly property var cellDate: new Date(rangeMonthGrid.year, model.month, model.day, 0, 0, 0)
                    readonly property bool isStart: root.sameDay(cellDate, root.selectedStartDate)
                    readonly property bool isEnd: root.sameDay(cellDate, root.selectedEndDate)
                    readonly property bool isSelected: isStart || isEnd
                    readonly property bool isInRange: root.dateInSelectedRange(cellDate)
                    readonly property bool isToday: root.sameDay(cellDate, new Date())

                    color: isSelected ? "#007AFF"
                           : isInRange ? "#E5F1FF"
                           : dayMouse.containsMouse ? "#EFF6FF" : "transparent"
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
                            root.setCalendarTargetDate(new Date(rangeMonthGrid.year, model.month, model.day, 0, 0, 0))
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
                    text: "本月"
                    onClicked: {
                        root.showCurrentMonth()
                        calendarPopup.close()
                    }
                }
            }
        }
    }
}
