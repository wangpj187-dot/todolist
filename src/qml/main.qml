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
    minimumHeight: 320
    title: "Todolist"
    color: "transparent"
    visible: true

    // Window opacity from config
    opacity: configService.windowOpacity

    // Frameless desktop widget (no system title bar / close / minimize).
    // Not forced always-on-top, so other apps can cover it normally.
    // The app is quit/hidden via the system tray menu.
    flags: Qt.Window | Qt.FramelessWindowHint

    Behavior on width {
        enabled: root.windowSizeAnimationActive
        NumberAnimation { duration: root.windowSizeAnimationDuration; easing.type: Easing.OutCubic }
    }

    Behavior on height {
        enabled: root.windowSizeAnimationActive
        NumberAnimation { duration: root.windowSizeAnimationDuration; easing.type: Easing.OutCubic }
    }

    // Responsive breakpoint: when the widget is narrow, collapse controls
    // (wrap filters to a new line, shrink button labels) like common web apps.
    readonly property bool compact: width < 560
    property int activeSmartList: 0
    property bool applyingSmartList: false
    property var availableTags: []
    property var todayCompletedTodos: []
    property bool completedDrawerOpen: false
    property bool calendarViewVisible: false
    property var listWindowGeometry: ({ "valid": false, "x": 0, "y": 0, "width": 0, "height": 0 })
    property bool windowSizeAnimationActive: false
    property string windowSizeAnimationMode: ""
    property var todayDate: new Date()
    property string summaryNotice: ""
    readonly property int listDefaultWidth: 490
    readonly property int listDefaultHeight: 340
    readonly property int listMinimumWidth: 300
    readonly property int listMinimumHeight: 320
    readonly property int calendarMinimumWidth: 980
    readonly property int calendarMinimumHeight: 680
    readonly property int windowSizeAnimationDuration: 260
    readonly property var smartLists: [
        { "label": "今天", "accent": "#007AFF" },
        { "label": "已计划", "accent": "#5856D6" },
        { "label": "高优先级", "accent": "#FF9500" },
        { "label": "有旗标", "accent": "#AF52DE" },
        { "label": "逾期", "accent": "#FF3B30" }
    ]

    function applySmartList(index) {
        applyingSmartList = true
        activeSmartList = index

        switch (index) {
        case 0:
            statusComboBox.currentIndex = 0
            priorityComboBox.currentIndex = 0
            sortComboBox.currentIndex = 2
            tagFilterCombo.currentIndex = 0
            todoService.smartFilterMode = 0
            break
        case 1:
            statusComboBox.currentIndex = 0
            priorityComboBox.currentIndex = 0
            sortComboBox.currentIndex = 2
            tagFilterCombo.currentIndex = 0
            todoService.smartFilterMode = 2
            break
        case 2:
            statusComboBox.currentIndex = 0
            priorityComboBox.currentIndex = 0
            sortComboBox.currentIndex = 1
            tagFilterCombo.currentIndex = 0
            todoService.smartFilterMode = 3
            break
        case 3:
            statusComboBox.currentIndex = 0
            priorityComboBox.currentIndex = 0
            sortComboBox.currentIndex = 0
            tagFilterCombo.currentIndex = 0
            todoService.smartFilterMode = 4
            break
        case 4:
            statusComboBox.currentIndex = 0
            priorityComboBox.currentIndex = 0
            sortComboBox.currentIndex = 2
            tagFilterCombo.currentIndex = 0
            todoService.smartFilterMode = 5
            break
        default:
            todoService.smartFilterMode = 0
            break
        }

        applyingSmartList = false
    }

    function noteManualFilterChanged() {
        if (!applyingSmartList) {
            activeSmartList = 0
            todoService.smartFilterMode = 0
        }
    }

    function refreshTagChoices() {
        const tags = todoService.getAllTags()
        const result = []
        for (let i = 0; i < tags.length; i++) {
            const tag = ("" + tags[i]).trim()
            if (tag.length > 0 && result.indexOf(tag) === -1)
                result.push(tag)
        }
        availableTags = result
    }

    function formatShortDate(date) {
        return (date.getMonth() + 1) + "月" + date.getDate() + "日"
    }

    function formatShortTime(value) {
        if (value && typeof value.getTime === "function" && !isNaN(value.getTime()))
            return value.toLocaleTimeString(Qt.locale(), "HH:mm")
        return ""
    }

    function refreshTodayCompletedTodos() {
        const items = todoService.getTodosForDateFromQml(todayDate)
        const result = []
        for (let i = 0; i < items.length; i++) {
            if (items[i].isCompleted && items[i].completedOnDate)
                result.push(items[i])
        }
        todayCompletedTodos = result
        if (todayCompletedTodos.length === 0)
            completedDrawerOpen = false
    }

    function millisecondsUntilNextDay() {
        const now = new Date()
        const nextDay = new Date(now.getFullYear(), now.getMonth(), now.getDate() + 1, 0, 0, 2)
        return Math.max(1000, nextDay.getTime() - now.getTime())
    }

    function restartDailyUiRefreshTimer() {
        dailyUiRefreshTimer.interval = millisecondsUntilNextDay()
        dailyUiRefreshTimer.restart()
    }

    function refreshTodayUi() {
        todayDate = new Date()
        todoService.refresh()
        refreshTodayCompletedTodos()
        restartDailyUiRefreshTimer()
    }

    function dateWithDayOffset(offset) {
        const date = new Date()
        date.setHours(0, 0, 0, 0)
        date.setDate(date.getDate() + offset)
        return date
    }

    function openTomorrowTodoForm() {
        functionPopup.close()
        todoForm.openForCreateOnDate(dateWithDayOffset(1))
    }

    function persistWindowSize() {
        if (root.visible && root.width >= root.minimumWidth)
            configService.setWindowWidth(Math.round(root.width))
        if (root.visible && root.height >= root.minimumHeight)
            configService.setWindowHeight(Math.round(root.height))
    }

    function beginWindowSizeAnimation(mode) {
        windowSizeAnimationMode = mode
        windowSizeAnimationActive = true
        windowSizeAnimationTimer.restart()
    }

    function finishWindowSizeAnimation() {
        const mode = windowSizeAnimationMode
        windowSizeAnimationActive = false

        if (mode === "calendar" && calendarViewVisible) {
            minimumWidth = calendarMinimumWidth
            minimumHeight = calendarMinimumHeight
            width = Math.max(root.width, calendarMinimumWidth)
            height = Math.max(root.height, calendarMinimumHeight)
        } else {
            minimumWidth = listMinimumWidth
            minimumHeight = listMinimumHeight
        }

        persistWindowSize()
        windowSizeAnimationMode = ""
    }

    function enterCalendarView() {
        if (calendarViewVisible)
            return

        listWindowGeometry = {
            "valid": true,
            "x": root.x,
            "y": root.y,
            "width": root.width,
            "height": root.height
        }
        completedDrawerOpen = false
        minimumWidth = listMinimumWidth
        minimumHeight = listMinimumHeight
        beginWindowSizeAnimation("calendar")
        calendarViewVisible = true
        width = Math.max(root.width, calendarMinimumWidth)
        height = Math.max(root.height, calendarMinimumHeight)
    }

    function leaveCalendarView() {
        if (!calendarViewVisible)
            return

        calendarViewVisible = false
        minimumWidth = listMinimumWidth
        minimumHeight = listMinimumHeight
        beginWindowSizeAnimation("list")

        if (listWindowGeometry.valid) {
            x = listWindowGeometry.x
            y = listWindowGeometry.y
            width = Math.max(listMinimumWidth, listWindowGeometry.width)
            height = Math.max(listMinimumHeight, listWindowGeometry.height)
            listWindowGeometry = { "valid": false, "x": 0, "y": 0, "width": 0, "height": 0 }
        } else {
            width = listDefaultWidth
            height = listDefaultHeight
        }
    }

    function toggleCalendarView() {
        if (calendarViewVisible)
            leaveCalendarView()
        else
            enterCalendarView()
    }

    // -------------------------------------------------------------------------
    // Window position/size persistence
    // -------------------------------------------------------------------------

    // Restore position on startup
    Component.onCompleted: {
        if (configService.windowX !== 0 || configService.windowY !== 0) {
            root.x = configService.windowX
            root.y = configService.windowY
        }
        if (configService.windowWidth === 400 && configService.windowHeight === 600) {
            root.width = listDefaultWidth
            root.height = listDefaultHeight
            persistWindowSize()
        }
        root.refreshTagChoices()
        root.applySmartList(0)
        root.refreshTodayCompletedTodos()
        root.restartDailyUiRefreshTimer()
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
        if (!root.windowSizeAnimationActive)
            persistWindowSize()
    }
    onHeightChanged: {
        if (!root.windowSizeAnimationActive)
            persistWindowSize()
    }

    Timer {
        id: windowSizeAnimationTimer
        interval: root.windowSizeAnimationDuration + 40
        repeat: false
        onTriggered: root.finishWindowSizeAnimation()
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

    Connections {
        target: todoService
        function onDataChanged() {
            root.refreshTagChoices()
            root.refreshTodayCompletedTodos()
        }
        function onSummaryExported(filePath) { root.summaryNotice = "已保存：" + filePath }
        function onSummaryLastExportErrorChanged() {
            if (todoService.summaryLastExportError.length > 0)
                root.summaryNotice = todoService.summaryLastExportError
        }
    }

    Shortcut {
        sequences: [StandardKey.New]
        onActivated: todoForm.openForCreate()
    }

    Timer {
        id: dailyUiRefreshTimer
        repeat: false
        onTriggered: root.refreshTodayUi()
    }

    Shortcut {
        sequences: [StandardKey.Find]
        onActivated: {
            functionPopup.open()
            searchField.forceActiveFocus()
        }
    }

    // Rounded widget surface (since the window itself is frameless & transparent)
    Rectangle {
        id: surface
        anchors.fill: parent
        anchors.margins: 1
        radius: 16
        color: "#F5F5F7"
        border.color: "#DADAE0"
        border.width: 1

        // Subtle pop-in animation when the widget becomes visible
        scale: root.visible ? 1.0 : 0.96
        Behavior on scale {
            NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
        }

        ColumnLayout {
            id: mainLayout
            anchors.fill: parent
            spacing: 10

            // ---------------------------------------------------------------------
            // Header (also acts as the window drag handle)
            // ---------------------------------------------------------------------
            RowLayout {
                id: header
                Layout.fillWidth: true
                Layout.leftMargin: 18
                Layout.rightMargin: 14
                Layout.topMargin: 16
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

	                    Text {
	                        text: root.calendarViewVisible ? "日历" : "提醒事项"
	                        font.pixelSize: root.compact ? 19 : 23
	                        font.weight: Font.DemiBold
	                        color: "#111827"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

	                    Text {
	                        text: root.calendarViewVisible
	                              ? calendarTaskView.calendarTitle + " · " + calendarTaskView.selectedItems.length + " 项"
	                              : root.formatShortDate(root.todayDate) + " · " + todoListView.count + " 项"
	                        font.pixelSize: 12
	                        color: "#8E8E93"
                        visible: !root.compact
                    }
                }

                Button {
                    id: addButton
                    text: root.compact ? "+" : "+ 新建"
                    font.pixelSize: 14
                    Layout.preferredWidth: root.compact ? 34 : 74
                    Layout.preferredHeight: 34
                    background: Rectangle {
                        radius: 17
                        color: addButton.down ? "#0062CC" : addButton.hovered ? "#0A84FF" : "#007AFF"
                        scale: addButton.down ? 0.96 : 1.0
                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
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

                Button {
                    id: functionButton
                    text: root.compact ? "⋯" : "功能"
                    font.pixelSize: root.compact ? 18 : 13
                    Layout.preferredWidth: root.compact ? 34 : 58
                    Layout.preferredHeight: 34
                    background: Rectangle {
                        radius: 17
                        color: functionPopup.opened || functionButton.down
                               ? "#D1E7FF"
                               : functionButton.hovered ? "#E5F1FF" : "#FFFFFF"
                        border.color: functionPopup.opened ? "#007AFF" : "#DADAE0"
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                    contentItem: Text {
                        text: functionButton.text
                        color: functionPopup.opened ? "#007AFF" : "#374151"
                        font: functionButton.font
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                    onClicked: functionPopup.opened ? functionPopup.close() : functionPopup.open()
                }

                Button {
                    id: tomorrowButton
                    text: "明日"
                    font.pixelSize: 13
                    Layout.preferredWidth: root.compact ? 42 : 50
                    Layout.preferredHeight: 34
                    ToolTip.visible: hovered
                    ToolTip.text: "添加明天的待办"
                    onClicked: root.openTomorrowTodoForm()
                    background: Rectangle {
                        radius: 17
                        color: tomorrowButton.down
                               ? "#D1E7FF"
                               : tomorrowButton.hovered ? "#E5F1FF" : "#FFFFFF"
                        border.color: "#DADAE0"
                        border.width: 1
                        scale: tomorrowButton.down ? 0.96 : 1.0
                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                    }
                    contentItem: Text {
                        text: tomorrowButton.text
                        color: tomorrowButton.hovered || tomorrowButton.down ? "#007AFF" : "#374151"
                        font: tomorrowButton.font
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }
                }

                Button {
                    id: dateHeaderButton
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    ToolTip.visible: hovered
                    ToolTip.text: root.calendarViewVisible ? "返回列表" : "按日期查看"
	                    background: Rectangle {
	                        radius: 17
	                        color: root.calendarViewVisible
	                               ? "#D1E7FF"
	                               : dateHeaderButton.down
	                               ? "#D1E7FF"
	                               : dateHeaderButton.hovered ? "#E5F1FF" : "transparent"
	                        border.color: root.calendarViewVisible ? "#007AFF" : "transparent"
	                        border.width: root.calendarViewVisible ? 1 : 0
	                        Behavior on color { ColorAnimation { duration: 150 } }
	                    }
                    contentItem: Canvas {
                        id: dateHeaderIcon
                        implicitWidth: 18
                        implicitHeight: 18
                        onPaint: {
                            const ctx = getContext("2d")
                            const stroke = dateHeaderButton.down ? "#0062CC" : "#007AFF"
                            const ox = Math.round((width - 18) / 2)
                            const oy = Math.round((height - 18) / 2)
                            ctx.clearRect(0, 0, width, height)
                            ctx.save()
                            ctx.translate(ox, oy)
                            ctx.lineWidth = 1.8
                            ctx.strokeStyle = stroke
                            ctx.fillStyle = stroke
                            ctx.lineCap = "round"
                            ctx.lineJoin = "round"

                            ctx.beginPath()
                            ctx.moveTo(4.5, 3.5)
                            ctx.lineTo(13.5, 3.5)
                            ctx.quadraticCurveTo(15.5, 3.5, 15.5, 5.5)
                            ctx.lineTo(15.5, 13.5)
                            ctx.quadraticCurveTo(15.5, 15.5, 13.5, 15.5)
                            ctx.lineTo(4.5, 15.5)
                            ctx.quadraticCurveTo(2.5, 15.5, 2.5, 13.5)
                            ctx.lineTo(2.5, 5.5)
                            ctx.quadraticCurveTo(2.5, 3.5, 4.5, 3.5)
                            ctx.stroke()

                            ctx.beginPath()
                            ctx.moveTo(2.5, 7)
                            ctx.lineTo(15.5, 7)
                            ctx.stroke()

                            ctx.beginPath()
                            ctx.moveTo(6, 2)
                            ctx.lineTo(6, 5)
                            ctx.moveTo(12, 2)
                            ctx.lineTo(12, 5)
                            ctx.stroke()

                            ctx.beginPath()
                            ctx.arc(6.5, 10, 0.85, 0, Math.PI * 2)
                            ctx.arc(10.5, 10, 0.85, 0, Math.PI * 2)
                            ctx.arc(6.5, 13, 0.85, 0, Math.PI * 2)
                            ctx.arc(10.5, 13, 0.85, 0, Math.PI * 2)
                            ctx.fill()
                            ctx.restore()
                        }
                    }
                    onDownChanged: dateHeaderIcon.requestPaint()
                    onHoveredChanged: dateHeaderIcon.requestPaint()
	                    onClicked: {
	                        functionPopup.close()
	                        root.toggleCalendarView()
	                    }
	                }
	            }

        // ---------------------------------------------------------------------
        // Todo List View
        // ---------------------------------------------------------------------
            Item {
                id: viewSwitcher
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ListView {
                    id: todoListView
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    enabled: !root.calendarViewVisible
                    visible: !root.calendarViewVisible || opacity > 0.01
                    opacity: root.calendarViewVisible ? 0 : 1
                    model: todoService.todos
                    delegate: TodoItem {
                        onEditRequested: function(todoId, title, description, priority, categoryId, dueDate, status, tags, startDate) {
                            todoForm.openForEdit(todoId, title, description, priority, categoryId, dueDate, status, tags, startDate)
                        }
                    }
                    spacing: 8
                    clip: true
                    transform: Translate {
                        id: todoListSlide
                        x: root.calendarViewVisible ? -22 : 0
                        Behavior on x {
                            NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                        }
                    }
                    Behavior on opacity {
                        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
                    }
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    // Smooth list transitions for add / remove / reorder
                    add: Transition {
                        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 430; easing.type: Easing.OutCubic }
                        NumberAnimation { property: "scale"; from: 0.18; to: 1.0; duration: 560; easing.type: Easing.OutBack; easing.overshoot: 1.35 }
                    }
                    remove: Transition {
                        NumberAnimation { property: "opacity"; to: 0; duration: 320; easing.type: Easing.InCubic }
                        NumberAnimation { property: "scale"; to: 0.18; duration: 360; easing.type: Easing.InBack; easing.overshoot: 1.25 }
                    }
                    displaced: Transition {
                        NumberAnimation { properties: "x,y"; duration: 360; easing.type: Easing.OutCubic }
                    }

                    // Empty State (overlay on list view)
                    Column {
                        id: emptyState
                        visible: todoListView.count === 0
                        anchors.centerIn: parent
                        spacing: 8
                        z: 1

                        Text {
                            text: "今天暂无待办事项"
                            font.pixelSize: 16
                            color: "#9CA3AF"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Text {
                            text: "点击右上角 + 新建今天的事项"
                            font.pixelSize: 14
                            color: "#D1D5DB"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }

                CalendarTaskView {
                    id: calendarTaskView
                    anchors.fill: parent
                    enabled: root.calendarViewVisible
                    visible: root.calendarViewVisible || opacity > 0.01
                    opacity: root.calendarViewVisible ? 1 : 0
                    transform: Translate {
                        id: calendarViewSlide
                        x: root.calendarViewVisible ? 0 : 22
                        Behavior on x {
                            NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                        }
                    }
                    Behavior on opacity {
                        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
                    }
                    onCreateRequested: function(date) {
                        todoForm.openForCreateOnDate(date)
                    }
                    onEditRequested: function(item) {
                        todoForm.openForEdit(item.id,
                                             item.title,
                                             item.description,
                                             item.priority,
                                             item.categoryId,
                                             item.dueDate,
                                             item.status,
                                             item.tags,
                                             item.startDate)
                    }
                }
            }

	            Rectangle {
                    visible: !root.calendarViewVisible
	                Layout.fillWidth: true
	                Layout.preferredHeight: 44
                color: "transparent"

                Button {
                    id: completedToggleButton
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 32, 190)
                    height: 32
                    enabled: root.todayCompletedTodos.length > 0
                    text: (root.completedDrawerOpen ? "↓" : "↑") + " 已完成 " + root.todayCompletedTodos.length
                    font.pixelSize: 12
                    ToolTip.visible: hovered && root.todayCompletedTodos.length === 0
                    ToolTip.text: "今天还没有已完成任务"
                    onClicked: root.completedDrawerOpen = !root.completedDrawerOpen

                    background: Rectangle {
                        radius: 16
                        color: !completedToggleButton.enabled ? "#ECECF1"
                               : completedToggleButton.down ? "#D1E7FF"
                               : completedToggleButton.hovered || root.completedDrawerOpen ? "#E5F1FF" : "#FFFFFF"
                        border.color: root.completedDrawerOpen ? "#007AFF" : "#DADAE0"
                        border.width: 1
                    }

                    contentItem: Text {
                        text: completedToggleButton.text
                        color: completedToggleButton.enabled ? "#007AFF" : "#9CA3AF"
                        font: completedToggleButton.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
            }
        }

	        Rectangle {
	            id: completedDrawer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
	            height: !root.calendarViewVisible && root.completedDrawerOpen
	                    ? Math.min(root.height * 0.55, Math.max(190, root.todayCompletedTodos.length * 66 + 86))
                    : 0
            z: 10
            visible: height > 1
            clip: true
            radius: 16
            color: "#FFFFFF"
            border.color: "#DADAE0"
            border.width: 1

            Behavior on height {
                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 12
                radius: 16
                color: parent.color
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                anchors.topMargin: 12
                anchors.bottomMargin: 14
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        id: closeCompletedDrawerButton
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 28
                        text: "↓"
                        font.pixelSize: 14
                        onClicked: root.completedDrawerOpen = false
                        background: Rectangle {
                            radius: 14
                            color: closeCompletedDrawerButton.hovered ? "#F3F4F6" : "transparent"
                        }
                        contentItem: Text {
                            text: closeCompletedDrawerButton.text
                            color: "#6B7280"
                            font: closeCompletedDrawerButton.font
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Text {
                            Layout.fillWidth: true
                            text: "今天已完成"
                            color: "#111827"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: root.formatShortDate(root.todayDate) + " · " + root.todayCompletedTodos.length + " 项"
                            color: "#8E8E93"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }
                }

                ListView {
                    id: completedTodayListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: root.todayCompletedTodos
                    spacing: 6
                    clip: true
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: Rectangle {
                        width: completedTodayListView.width
                        height: Math.max(completedContent.implicitHeight + 16, 58)
                        radius: 8
                        color: "#F9FAFB"
                        border.color: "#E5E7EB"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 8
                            anchors.topMargin: 8
                            anchors.bottomMargin: 8
                            spacing: 8

                            CheckBox {
                                Layout.alignment: Qt.AlignTop
                                checked: true
                                onClicked: todoService.uncompleteTodo(modelData.id)
                            }

                            ColumnLayout {
                                id: completedContent
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.title || ""
                                    color: "#6B7280"
                                    font.pixelSize: 13
                                    font.strikeout: true
                                    elide: Text.ElideRight
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: root.formatShortTime(modelData.completedAt)
                                    visible: text.length > 0
                                    color: "#9CA3AF"
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }

                            Button {
                                id: editCompletedButton
                                Layout.preferredWidth: 44
                                Layout.preferredHeight: 30
                                text: "编辑"
                                font.pixelSize: 12
                                onClicked: {
                                    root.completedDrawerOpen = false
                                    todoForm.openForEdit(modelData.id,
                                                         modelData.title,
                                                         modelData.description,
                                                         modelData.priority,
                                                         modelData.categoryId,
	                                                         modelData.dueDate,
	                                                         modelData.status,
	                                                         modelData.tags,
	                                                         modelData.startDate)
                                }
                                background: Rectangle {
                                    radius: 7
                                    color: editCompletedButton.hovered ? "#EFF6FF" : "transparent"
                                }
                                contentItem: Text {
                                    text: editCompletedButton.text
                                    color: editCompletedButton.hovered ? "#2563EB" : "#6B7280"
                                    font: editCompletedButton.font
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
            id: functionPopup
            width: Math.min(root.width - 24, 430)
            x: Math.max(12, Math.min(surface.width - width - 12,
                                     functionButton.mapToItem(surface, 0, 0).x + functionButton.width - width))
            y: functionButton.mapToItem(surface, 0, 0).y + functionButton.height + 8
            padding: 12
            height: Math.max(260, Math.min(root.height - y - 14, functionPanel.implicitHeight + padding * 2))
            focus: true
            modal: false
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

            background: Rectangle {
                radius: 14
                color: "#FFFFFF"
                border.color: "#DADAE0"
                border.width: 1
            }

            contentItem: ScrollView {
                id: functionPanelScroll
                clip: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                ColumnLayout {
                    id: functionPanel
                    width: functionPanelScroll.availableWidth
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: "功能"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: "#111827"
                            Layout.fillWidth: true
                        }

                        Text {
                            text: root.smartLists[root.activeSmartList].label
                            font.pixelSize: 12
                            color: "#8E8E93"
                        }
                    }

                    Flow {
                        id: smartListBar
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        spacing: 8

                        Repeater {
                            model: root.smartLists

                            delegate: Button {
                                id: smartButton
                                property bool selected: root.activeSmartList === index

                                height: 34
                                width: Math.max(64, smartLabel.implicitWidth + 26)
                                text: modelData.label
                                font.pixelSize: 12

                                background: Rectangle {
                                    radius: 17
                                    color: smartButton.selected
                                           ? modelData.accent
                                           : smartButton.hovered ? "#FFFFFF" : "#ECECF1"
                                    border.color: smartButton.selected ? modelData.accent : "#DADAE0"
                                    border.width: 1
                                    Behavior on color { ColorAnimation { duration: 140 } }
                                }

                                contentItem: Text {
                                    id: smartLabel
                                    text: smartButton.text
                                    font: smartButton.font
                                    color: smartButton.selected ? "#FFFFFF" : "#1F2937"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }

                                onClicked: root.applySmartList(index)
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#ECECF1"
                    }

                    Flow {
                        id: filterControls
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        spacing: 8

                    readonly property int compactControlWidth: Math.max(120, Math.floor((width - 8) / 2))
                    readonly property int normalComboWidth: 120
                    readonly property int normalSearchWidth: Math.max(148, width - normalComboWidth * 4 - 32)

                    TextField {
                        id: searchField
                        width: root.compact ? filterControls.width : filterControls.normalSearchWidth
                        height: 34
                        placeholderText: "搜索"
                        font.pixelSize: 12
                        background: Rectangle {
                            color: "#FFFFFF"
                            radius: 9
                            border.color: searchField.activeFocus ? "#007AFF" : "#DADAE0"
                            border.width: 1
                        }
                        onTextChanged: {
                            todoService.searchQuery = text
                        }
                    }

                    ComboBox {
                        id: statusComboBox
                        width: root.compact ? filterControls.compactControlWidth : filterControls.normalComboWidth
                        height: 34
	                        model: ["全部", "待处理", "进行中"]
                        currentIndex: 0
                        font.pixelSize: 12
                        background: Rectangle {
                            color: "#FFFFFF"
                            radius: 9
                            border.color: "#DADAE0"
                            border.width: 1
                        }
                        contentItem: Text {
                            text: statusComboBox.displayText
                            color: "#374151"
                            font: statusComboBox.font
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 9
                            rightPadding: 24
                            elide: Text.ElideRight
                        }
                        onCurrentIndexChanged: {
                            root.noteManualFilterChanged()
                            todoService.filterStatus = currentIndex
                        }
                    }

                    ComboBox {
                        id: priorityComboBox
                        width: root.compact ? filterControls.compactControlWidth : filterControls.normalComboWidth
                        height: 34
                        model: ["全部优先级", "低", "中", "高", "紧急"]
                        currentIndex: 0
                        font.pixelSize: 12
                        background: Rectangle {
                            color: "#FFFFFF"
                            radius: 9
                            border.color: "#DADAE0"
                            border.width: 1
                        }
                        contentItem: Text {
                            text: priorityComboBox.displayText
                            color: "#374151"
                            font: priorityComboBox.font
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 9
                            rightPadding: 24
                            elide: Text.ElideRight
                        }
                        onCurrentIndexChanged: {
                            root.noteManualFilterChanged()
                            todoService.filterPriority = currentIndex
                        }
                    }

                    ComboBox {
                        id: sortComboBox
                        width: root.compact ? filterControls.width : filterControls.normalComboWidth
                        height: 34
                        model: ["按创建时间", "按优先级", "按截止日期", "按标题"]
                        currentIndex: 0
                        font.pixelSize: 12
                        background: Rectangle {
                            color: "#FFFFFF"
                            radius: 9
                            border.color: "#DADAE0"
                            border.width: 1
                        }
                        contentItem: Text {
                            text: sortComboBox.displayText
                            color: "#374151"
                            font: sortComboBox.font
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 9
                            rightPadding: 24
                            elide: Text.ElideRight
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

                    ComboBox {
                        id: tagFilterCombo
                        width: root.compact ? filterControls.width : filterControls.normalComboWidth
                        height: 34
                        model: ["全部标签"].concat(root.availableTags)
                        currentIndex: 0
                        font.pixelSize: 12
                        background: Rectangle {
                            color: "#FFFFFF"
                            radius: 9
                            border.color: "#DADAE0"
                            border.width: 1
                        }
                        contentItem: Text {
                            text: tagFilterCombo.displayText
                            color: tagFilterCombo.currentIndex > 0 ? "#1D4ED8" : "#374151"
                            font: tagFilterCombo.font
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 9
                            rightPadding: 24
                            elide: Text.ElideRight
                        }
                        onCurrentIndexChanged: {
                            root.noteManualFilterChanged()
                            todoService.filterTag = currentIndex > 0 ? model[currentIndex] : ""
                        }
                    }
                }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#ECECF1"
                    }

                    ColumnLayout {
                        id: dailySummaryPanel
                        Layout.fillWidth: true
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: "整理所有提醒"
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                color: "#111827"
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Switch {
                                id: summaryAutoSwitch
                                checked: todoService.summaryAutoRefresh
                                text: "自动"
                                font.pixelSize: 12
                                onToggled: todoService.summaryAutoRefresh = checked
                            }
                        }

                        TextField {
                            id: summaryPathField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            placeholderText: displayFolderPath(todoService.defaultSummaryExportPath())
                            placeholderTextColor: "#9CA3AF"
                            font.pixelSize: 12
                            selectByMouse: true

                            function displayFolderPath(filePath) {
                                const value = String(filePath || "")
                                const suffix = "/todolist-summary.json"
                                const nativeSuffix = "\\todolist-summary.json"
                                if (value.endsWith(suffix))
                                    return value.slice(0, value.length - suffix.length)
                                if (value.endsWith(nativeSuffix))
                                    return value.slice(0, value.length - nativeSuffix.length)
                                return value
                            }

                            function displayInputPath(filePath) {
                                const value = String(filePath || "")
                                if (value === String(todoService.defaultSummaryExportPath()))
                                    return ""
                                return displayFolderPath(value)
                            }

                            function refreshFromService() {
                                text = displayInputPath(todoService.summaryExportPath)
                            }

                            function commitPath() {
                                todoService.summaryExportPath = text.trim()
                                if (text.trim().length === 0)
                                    root.summaryNotice = ""
                                else
                                    root.summaryNotice = "保存路径已更新"
                                refreshFromService()
                            }

                            background: Rectangle {
                                color: "#FFFFFF"
                                radius: 9
                                border.color: summaryPathField.activeFocus ? "#007AFF" : "#DADAE0"
                                border.width: 1
                            }
                            Component.onCompleted: refreshFromService()
                            onAccepted: commitPath()
                            onEditingFinished: commitPath()

                            Connections {
                                target: todoService
                                function onSummaryExportPathChanged() {
                                    if (!summaryPathField.activeFocus)
                                        summaryPathField.refreshFromService()
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Button {
                                id: summaryExportButton
                                text: "立即整理"
                                font.pixelSize: 12
                                Layout.preferredWidth: 82
                                Layout.preferredHeight: 32
                                background: Rectangle {
                                    radius: 9
                                    color: summaryExportButton.down
                                           ? "#0062CC"
                                           : summaryExportButton.hovered ? "#0A84FF" : "#007AFF"
                                }
                                contentItem: Text {
                                    text: summaryExportButton.text
                                    color: "#FFFFFF"
                                    font: summaryExportButton.font
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                }
                                onClicked: {
                                    summaryPathField.commitPath()
                                    if (todoService.exportTaskSummaryNow())
                                        root.summaryNotice = "已保存：" + todoService.summaryExportPath
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: root.summaryNotice.length > 0 ? root.summaryNotice : "保存到：" + todoService.summaryExportPath
                                font.pixelSize: 11
                                color: todoService.summaryLastExportError.length > 0 ? "#C2410C" : "#8E8E93"
                                elide: Text.ElideMiddle
                                maximumLineCount: 1
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#ECECF1"
                    }

                    Button {
                        id: quitButton
                        text: "退出应用"
                        font.pixelSize: 12
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        background: Rectangle {
                            radius: 9
                            color: quitButton.down
                                   ? "#FEE2E2"
                                   : quitButton.hovered ? "#FEF2F2" : "#FFFFFF"
                            border.color: quitButton.hovered || quitButton.down ? "#FCA5A5" : "#DADAE0"
                            border.width: 1
                        }
                        contentItem: Text {
                            text: quitButton.text
                            color: "#B91C1C"
                            font: quitButton.font
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                        onClicked: {
                            functionPopup.close()
                            configService.save()
                            Qt.quit()
                        }
                    }
                }
            }
        }

        // Drag handle covering the header strip: moves the frameless window.
        // Anchored to `surface` (its real parent) instead of the layout-managed
        // `header`, so the geometry is valid; z:-1 keeps the header buttons clickable.
        MouseArea {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 64
            z: -1
            onPressed: function(mouse) { root.startSystemMove() }
        }
    }

    // -------------------------------------------------------------------------
    // Todo Form Dialog
    // -------------------------------------------------------------------------
    TodoForm {
        id: todoForm
        onSaved: {
            todoService.refresh()
            if (dateReviewDialog.opened)
                dateReviewDialog.refresh()
        }
    }

    DateReviewDialog {
        id: dateReviewDialog
        onEditRequested: function(item) {
            todoForm.openForEdit(item.id,
                                 item.title,
                                 item.description,
                                 item.priority,
                                 item.categoryId,
	                                 item.dueDate,
	                                 item.status,
	                                 item.tags,
	                                 item.startDate)
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
            if (quickTitleField.text.trim().length > 0) {
                todoService.createTodoFromQml(
                    quickTitleField.text.trim(),
                    "",
                    quickPriorityCombo.currentIndex + 1, // Priority enum starts at 1
                    "", // No category
                    null // No due date
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
