import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic 6.3 as Basic
import QtQuick.Layouts 1.15
import TodoApp 1.0

Dialog {
    id: root

    parent: Overlay.overlay
    readonly property real overlayWidth: parent && parent.width > 0 ? parent.width : 520
    readonly property real overlayHeight: parent && parent.height > 0 ? parent.height : 700
    readonly property real dialogMargin: overlayWidth < 340 ? 8 : 12
    readonly property bool compact: width < 460
    readonly property bool tiny: width < 340

    width: Math.max(240, Math.min(520, overlayWidth - dialogMargin * 2))
    height: Math.max(320, Math.min(700, overlayHeight - dialogMargin * 2))
    x: Math.round((overlayWidth - width) / 2)
    y: Math.max(dialogMargin, Math.round((overlayHeight - height) / 2))
    modal: true
    padding: 0
    closePolicy: Popup.CloseOnEscape

    // Smooth open / close transitions
    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 180
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "scale"
            from: 0.94
            to: 1.0
            duration: 180
            easing.type: Easing.OutCubic
        }
    }
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 140
            easing.type: Easing.InCubic
        }
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.96
            duration: 140
            easing.type: Easing.InCubic
        }
    }

    property bool isEdit: false
    property string editTodoId: ""
    property string editTitle: ""
    property string editDescription: ""
    property int editPriority: Todo.Medium
    property var editStartDate: null
    property var editDueDate: null
    property int editStatus: Todo.Pending
    property var editTags: []

    property bool submitted: false
    property string errorText: ""
    property bool dateRangeEnabled: false
    property var selectedDate: new Date()
    property var startDateValue: new Date()
    property var endDateValue: new Date()
    property string calendarTarget: "start"
    property var calendarAnchor: null
    property bool calendarRangeMode: false
    property bool waitingForRangeEnd: false
    property int calendarMonth: selectedDate.getMonth()
    property int calendarYear: selectedDate.getFullYear()
    readonly property string flagTag: "旗标"
    readonly property var baseTagPresets: ["工作", "学习", "娱乐", "消费", "羽毛球", "健身"]
    property var tagPresets: ["工作", "学习", "娱乐", "消费", "羽毛球", "健身"]

    signal saved(var todoId)

    function openForCreate() {
        refreshTagPresets();
        resetForm();
        isEdit = false;
        open();
        Qt.callLater(function () {
            titleField.forceActiveFocus();
        });
    }

    function openForCreateOnDate(date) {
        refreshTagPresets();
        resetForm();
        isEdit = false;
        dateRangeEnabled = true;
        setDateRangeValues(date, date);
        open();
        Qt.callLater(function () {
            titleField.forceActiveFocus();
        });
    }

    function openForEdit(todoId, title, description, priority, categoryId, dueDate, status, tags, startDate) {
        refreshTagPresets();
        resetForm();

        editTodoId = todoId;
        editTitle = title;
        editDescription = description;
        editPriority = priority;
        editStartDate = startDate;
        editDueDate = dueDate;
        editStatus = status || Todo.Pending;
        editTags = tagsToArray(tags);
        isEdit = true;
        tagPresets = mergeTagPresets(tagPresets.concat(editTags));

        titleField.text = editTitle;
        descField.text = editDescription;
        tagField.text = tagsToText(editTags);
        priorityRepeater.itemAt(Math.max(0, Math.min(3, editPriority - 1))).checked = true;
        statusEditCombo.currentIndex = Math.max(0, Math.min(3, editStatus - 1));

        if (isValidDate(editStartDate) || isValidDate(editDueDate)) {
            dateRangeEnabled = true;
            setDateRangeValues(editStartDate, editDueDate);
        }

        open();
        Qt.callLater(function () {
            titleField.forceActiveFocus();
        });
    }

    function resetForm() {
        submitted = false;
        errorText = "";
        titleField.text = "";
        descField.text = "";
        tagField.text = "";
        priorityRepeater.itemAt(Todo.Medium - 1).checked = true;
        statusEditCombo.currentIndex = Todo.Pending - 1;
        dateRangeEnabled = false;
        calendarAnchor = null;
        calendarRangeMode = false;
        waitingForRangeEnd = false;
        setDateRangeValues(new Date(), new Date());

        editTodoId = "";
        editTitle = "";
        editDescription = "";
        editPriority = Todo.Medium;
        editStartDate = null;
        editDueDate = null;
        editStatus = Todo.Pending;
        editTags = [];
    }

    function save() {
        submitted = true;
        errorText = "";

        if (titleField.text.trim().length === 0) {
            errorText = "请先填写标题";
            titleField.forceActiveFocus();
            return;
        }

        const title = titleField.text.trim();
        const description = descField.text.trim();
        const priority = priorityButtonGroup.checkedButton ? priorityButtonGroup.checkedButton.priorityValue : Todo.Medium;
        const categoryId = "";
        const statusValue = root.isEdit ? statusEditCombo.currentIndex + 1 : editStatus;
        const tags = parseTagsForSave();
        let startDate = dayStart(new Date());
        let dueDate = dayEnd(new Date());

        if (dateRangeEnabled) {
            startDate = dayStart(startDateValue);
            dueDate = dayEnd(endDateValue);
            if (dueDate < startDate) {
                errorText = "结束日期不能早于开始日期";
                return;
            }
        }

        if (isEdit) {
            if (!todoService.updateTodoFromQmlWithTagsAndRange(editTodoId, title, description, priority, categoryId, startDate, dueDate, statusValue, tags)) {
                errorText = "保存失败，请检查输入后重试";
                return;
            }
            saved(editTodoId);
        } else {
            const newId = todoService.createTodoFromQmlWithTagsAndRange(title, description, priority, categoryId, startDate, dueDate, tags);
            if (!isValidId(newId)) {
                errorText = "创建失败，请检查输入后重试";
                return;
            }
            saved(newId);
        }

        refreshTagPresets();
        root.close();
    }

    function isValidId(value) {
        const id = String(value || "");
        return id.length > 0 && id !== "{00000000-0000-0000-0000-000000000000}";
    }

    function pad2(value) {
        return ("0" + value).slice(-2);
    }

    function formatDate(date) {
        return date.getFullYear() + "-" + pad2(date.getMonth() + 1) + "-" + pad2(date.getDate());
    }

    function isValidDate(value) {
        return value && value instanceof Date && !isNaN(value.getTime());
    }

    function dayStart(value) {
        const date = isValidDate(value) ? value : new Date();
        return new Date(date.getFullYear(), date.getMonth(), date.getDate(), 0, 0, 0);
    }

    function dayEnd(value) {
        const date = isValidDate(value) ? value : new Date();
        return new Date(date.getFullYear(), date.getMonth(), date.getDate(), 23, 59, 59);
    }

    function setCalendarDate(value) {
        const date = isValidDate(value) ? value : new Date();
        selectedDate = date;
        calendarMonth = date.getMonth();
        calendarYear = date.getFullYear();
    }

    function setDateRangeValues(startValue, endValue) {
        const fallback = new Date();
        const start = isValidDate(startValue) ? startValue : isValidDate(endValue) ? endValue : fallback;
        const end = isValidDate(endValue) ? endValue : start;
        startDateValue = dayStart(start);
        endDateValue = dayEnd(end);
        if (endDateValue < startDateValue)
            endDateValue = dayEnd(startDateValue);
        setCalendarDate(startDateValue);
    }

    function openDatePicker(target, anchor) {
        dateRangeEnabled = true;
        calendarTarget = target;
        calendarAnchor = anchor || null;
        calendarRangeMode = false;
        waitingForRangeEnd = false;
        setCalendarDate(target === "start" ? startDateValue : endDateValue);
        calendarPopup.open();
    }

    function openRangePicker(anchor) {
        dateRangeEnabled = true;
        calendarTarget = "start";
        calendarAnchor = anchor || null;
        calendarRangeMode = true;
        waitingForRangeEnd = false;
        setCalendarDate(startDateValue);
        calendarPopup.open();
    }

    function setDatePickerValue(value) {
        if (calendarRangeMode) {
            if (!waitingForRangeEnd) {
                startDateValue = dayStart(value);
                endDateValue = dayEnd(value);
                waitingForRangeEnd = true;
                calendarTarget = "end";
                setCalendarDate(value);
                return false;
            }

            endDateValue = dayEnd(value);
            if (endDateValue < startDateValue) {
                const start = startDateValue;
                startDateValue = dayStart(endDateValue);
                endDateValue = dayEnd(start);
            }
            calendarRangeMode = false;
            waitingForRangeEnd = false;
            setCalendarDate(value);
            return true;
        }

        if (calendarTarget === "start")
            startDateValue = dayStart(value);
        else
            endDateValue = dayEnd(value);
        if (endDateValue < startDateValue)
            endDateValue = dayEnd(startDateValue);
        setCalendarDate(value);
        return true;
    }

    function shiftCalendarMonth(delta) {
        const next = new Date(calendarYear, calendarMonth + delta, 1);
        calendarYear = next.getFullYear();
        calendarMonth = next.getMonth();
    }

    function tagsToArray(value) {
        if (!value)
            return [];

        const result = [];
        for (let i = 0; i < value.length; i++) {
            const tag = ("" + value[i]).trim();
            if (tag.length > 0 && result.indexOf(tag) === -1)
                result.push(tag);
        }
        return result;
    }

    function tagsToText(tags) {
        const visibleTags = tagsToArray(tags).filter(function (tag) {
            return tag !== flagTag;
        });
        return visibleTags.map(function (tag) {
            return "#" + tag;
        }).join(" ");
    }

    function tagsFromText(text) {
        const result = [];
        const parts = (text || "").split(/[#\s,，、;；]+/);

        function addTag(value) {
            const tag = (value || "").trim();
            if (tag.length > 0 && tag !== flagTag && result.indexOf(tag) === -1)
                result.push(tag);
        }

        for (let i = 0; i < parts.length; i++)
            addTag(parts[i]);

        return result;
    }

    function parseTagsForSave() {
        return tagsFromText(tagField.text);
    }

    function mergeTagPresets(values) {
        const result = [];

        function addTag(value) {
            const tag = (value || "").trim();
            if (tag.length > 0 && tag !== flagTag && result.indexOf(tag) === -1)
                result.push(tag);
        }

        for (let i = 0; i < baseTagPresets.length; i++)
            addTag(baseTagPresets[i]);

        if (values) {
            for (let j = 0; j < values.length; j++)
                addTag(values[j]);
        }

        return result;
    }

    function refreshTagPresets() {
        let tags = [];
        if (typeof todoService !== "undefined" && todoService.getAllTags)
            tags = todoService.getAllTags();
        tagPresets = mergeTagPresets(tags);
    }

    function addCurrentTagsToPresetBar() {
        const typedTags = tagsFromText(tagField.text);
        if (typedTags.length === 0) {
            tagField.forceActiveFocus();
            return;
        }

        tagPresets = mergeTagPresets(tagPresets.concat(typedTags));
        tagField.text = tagsToText(typedTags);
        tagField.forceActiveFocus();
    }

    function hasTagInField(tag) {
        const tags = tagsFromText(tagField.text);
        const target = (tag || "").trim();
        for (let i = 0; i < tags.length; i++) {
            if (tags[i] === target)
                return true;
        }
        return false;
    }

    function toggleTagPreset(tag) {
        const target = (tag || "").trim();
        if (target.length === 0 || target === flagTag)
            return;
        const result = [];
        const tags = tagsFromText(tagField.text);
        let removed = false;

        for (let i = 0; i < tags.length; i++) {
            const value = tags[i];
            if (value.length === 0)
                continue;
            if (value === target) {
                removed = true;
                continue;
            }
            if (result.indexOf(value) === -1)
                result.push(value);
        }

        if (!removed)
            result.push(target);

        tagField.text = result.map(function (value) {
            return "#" + value;
        }).join(" ");
    }

    Component.onCompleted: refreshTagPresets()

    ButtonGroup {
        id: priorityButtonGroup
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
            Layout.preferredHeight: root.compact ? 66 : 72
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

                Rectangle {
                    Layout.preferredWidth: 4
                    Layout.preferredHeight: 32
                    radius: 2
                    color: "#007AFF"
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: root.isEdit ? "编辑待办" : "新建待办"
                        color: "#111827"
                        font.pixelSize: root.compact ? 17 : 19
                        font.weight: Font.DemiBold
                    }

                    Text {
                        text: root.isEdit ? "调整事项内容、优先级和提醒时间" : "记录一件清晰、可执行的待办事项"
                        color: "#6B7280"
                        font.pixelSize: 12
                        visible: !root.tiny
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

        ScrollView {
            id: formScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            padding: root.tiny ? 10 : root.compact ? 14 : 18
            contentWidth: availableWidth
            contentHeight: formBody.implicitHeight
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                id: formBody
                width: formScrollView.availableWidth
                implicitHeight: childrenRect.height
                spacing: root.compact ? 12 : 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: "标题"
                            color: "#374151"
                            font.pixelSize: 13
                            font.bold: true
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "必填"
                            color: "#9CA3AF"
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    TextField {
                        id: titleField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42
                        placeholderText: "请输入待办标题"
                        font.pixelSize: 14
                        selectByMouse: true
                        background: Rectangle {
                            color: "#FFFFFF"
                            radius: 6
                            border.width: 1
                            border.color: root.submitted && titleField.text.trim().length === 0 ? "#DC2626" : titleField.activeFocus ? "#007AFF" : "#E5E7EB"
                        }
                        padding: 10
                        onAccepted: root.save()
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: root.submitted && titleField.text.trim().length === 0
                        text: "标题不能为空"
                        color: "#DC2626"
                        font.pixelSize: 12
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: primaryDateLayout.implicitHeight + 16
                    radius: 8
                    color: "#F9FAFB"
                    border.color: root.dateRangeEnabled ? "#BFDBFE" : "#E5E7EB"
                    border.width: 1

                    RowLayout {
                        id: primaryDateLayout
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        Button {
                            id: primaryStartDateButton
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.preferredHeight: 38
                            text: formatDate(root.startDateValue)
                            font.pixelSize: 13
                            ToolTip.visible: hovered
                            ToolTip.text: "修改开始日期"
                            onClicked: root.openDatePicker("start", primaryStartDateButton)

                            background: Rectangle {
                                radius: 6
                                color: primaryStartDateButton.down ? "#E5F1FF" : primaryStartDateButton.hovered ? "#F3F8FF" : "#FFFFFF"
                                border.color: calendarPopup.opened && root.calendarTarget === "start" ? "#007AFF" : "#E5E7EB"
                                border.width: 1
                                Behavior on color {
                                    ColorAnimation {
                                        duration: 150
                                    }
                                }
                            }

                            contentItem: Text {
                                text: primaryStartDateButton.text
                                color: root.dateRangeEnabled ? "#111827" : "#9CA3AF"
                                font: primaryStartDateButton.font
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }

                        Text {
                            Layout.preferredWidth: 12
                            text: "-"
                            color: "#A1A1AA"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        Button {
                            id: primaryEndDateButton
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.preferredHeight: 38
                            text: formatDate(root.endDateValue)
                            font.pixelSize: 13
                            ToolTip.visible: hovered
                            ToolTip.text: "修改结束日期"
                            onClicked: root.openDatePicker("end", primaryEndDateButton)

                            background: Rectangle {
                                radius: 6
                                color: primaryEndDateButton.down ? "#E5F1FF" : primaryEndDateButton.hovered ? "#F3F8FF" : "#FFFFFF"
                                border.color: calendarPopup.opened && root.calendarTarget === "end" ? "#007AFF" : "#E5E7EB"
                                border.width: 1
                                Behavior on color {
                                    ColorAnimation {
                                        duration: 150
                                    }
                                }
                            }

                            contentItem: Text {
                                text: primaryEndDateButton.text
                                color: root.dateRangeEnabled ? "#111827" : "#9CA3AF"
                                font: primaryEndDateButton.font
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }

                        Button {
                            id: primaryCalendarButton
                            Layout.preferredWidth: 38
                            Layout.preferredHeight: 38
                            ToolTip.visible: hovered
                            ToolTip.text: "选择日期范围"
                            onClicked: root.openRangePicker(primaryCalendarButton)

                            background: Rectangle {
                                radius: 6
                                color: primaryCalendarButton.down ? "#D1E7FF" : primaryCalendarButton.hovered || (calendarPopup.opened && root.calendarRangeMode) ? "#E5F1FF" : "#FFFFFF"
                                border.color: calendarPopup.opened && root.calendarRangeMode ? "#007AFF" : "#E5E7EB"
                                border.width: 1
                                scale: primaryCalendarButton.down ? 0.96 : 1.0
                                Behavior on color {
                                    ColorAnimation {
                                        duration: 150
                                    }
                                }
                                Behavior on scale {
                                    NumberAnimation {
                                        duration: 120
                                        easing.type: Easing.OutCubic
                                    }
                                }
                            }

                            contentItem: Canvas {
                                id: primaryCalendarIcon
                                implicitWidth: 18
                                implicitHeight: 18
                                onPaint: {
                                    const ctx = getContext("2d");
                                    const stroke = primaryCalendarButton.down ? "#0062CC" : "#007AFF";
                                    const ox = Math.round((width - 18) / 2);
                                    const oy = Math.round((height - 18) / 2);
                                    ctx.clearRect(0, 0, width, height);
                                    ctx.save();
                                    ctx.translate(ox, oy);
                                    ctx.lineWidth = 1.7;
                                    ctx.strokeStyle = stroke;
                                    ctx.fillStyle = stroke;
                                    ctx.lineCap = "round";
                                    ctx.lineJoin = "round";
                                    ctx.beginPath();
                                    ctx.moveTo(4.5, 3.5);
                                    ctx.lineTo(13.5, 3.5);
                                    ctx.quadraticCurveTo(15.5, 3.5, 15.5, 5.5);
                                    ctx.lineTo(15.5, 13.5);
                                    ctx.quadraticCurveTo(15.5, 15.5, 13.5, 15.5);
                                    ctx.lineTo(4.5, 15.5);
                                    ctx.quadraticCurveTo(2.5, 15.5, 2.5, 13.5);
                                    ctx.lineTo(2.5, 5.5);
                                    ctx.quadraticCurveTo(2.5, 3.5, 4.5, 3.5);
                                    ctx.stroke();
                                    ctx.beginPath();
                                    ctx.moveTo(2.5, 7);
                                    ctx.lineTo(15.5, 7);
                                    ctx.stroke();
                                    ctx.beginPath();
                                    ctx.moveTo(6, 2);
                                    ctx.lineTo(6, 5);
                                    ctx.moveTo(12, 2);
                                    ctx.lineTo(12, 5);
                                    ctx.stroke();
                                    ctx.beginPath();
                                    ctx.arc(6.5, 10, 0.85, 0, Math.PI * 2);
                                    ctx.arc(10.5, 10, 0.85, 0, Math.PI * 2);
                                    ctx.arc(6.5, 13, 0.85, 0, Math.PI * 2);
                                    ctx.arc(10.5, 13, 0.85, 0, Math.PI * 2);
                                    ctx.fill();
                                    ctx.restore();
                                }
                            }
                            onDownChanged: primaryCalendarIcon.requestPaint()
                            onHoveredChanged: primaryCalendarIcon.requestPaint()
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "描述"
                        color: "#374151"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    TextArea {
                        id: descField
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.tiny ? 68 : 88
                        placeholderText: "补充更多上下文（可选）"
                        font.pixelSize: 14
                        wrapMode: Text.Wrap
                        selectByMouse: true
                        background: Rectangle {
                            color: "#FFFFFF"
                            radius: 6
                            border.color: descField.activeFocus ? "#007AFF" : "#E5E7EB"
                            border.width: 1
                        }
                        padding: 10
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 1
                    columnSpacing: 12
                    rowSpacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "优先级"
                            color: "#374151"
                            font.pixelSize: 13
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Repeater {
                                id: priorityRepeater
                                model: [
                                    {
                                        label: "低",
                                        value: Todo.Low,
                                        color: "#16A34A"
                                    },
                                    {
                                        label: "中",
                                        value: Todo.Medium,
                                        color: "#CA8A04"
                                    },
                                    {
                                        label: "高",
                                        value: Todo.High,
                                        color: "#EA580C"
                                    },
                                    {
                                        label: "紧急",
                                        value: Todo.Urgent,
                                        color: "#DC2626"
                                    }
                                ]

                                Button {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 38
                                    checkable: true
                                    checked: modelData.value === Todo.Medium
                                    ButtonGroup.group: priorityButtonGroup
                                    property int priorityValue: modelData.value
                                    text: modelData.label
                                    background: Rectangle {
                                        radius: 6
                                        color: parent.checked ? modelData.color : "#FFFFFF"
                                        border.color: parent.checked ? modelData.color : "#E5E7EB"
                                        border.width: 1
                                    }
                                    contentItem: Text {
                                        text: parent.text
                                        color: parent.checked ? "#FFFFFF" : "#374151"
                                        font.pixelSize: 13
                                        font.bold: parent.checked
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: statusLayout.implicitHeight + 18
                        radius: 8
                        color: root.isEdit ? "#F8FAFC" : "transparent"
                        border.color: root.isEdit ? "#E5E7EB" : "transparent"
                        border.width: root.isEdit ? 1 : 0
                        visible: root.isEdit

                        ColumnLayout {
                            id: statusLayout
                            anchors.fill: parent
                            anchors.margins: 9
                            spacing: 8

                            Text {
                                Layout.fillWidth: true
                                text: "任务状态"
                                color: "#374151"
                                font.pixelSize: 13
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            ComboBox {
                                id: statusEditCombo
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                model: ["待处理", "进行中", "已完成", "已取消"]
                                currentIndex: 0
                                font.pixelSize: 13
                                background: Rectangle {
                                    color: "#FFFFFF"
                                    radius: 6
                                    border.color: statusEditCombo.activeFocus ? "#007AFF" : "#E5E7EB"
                                    border.width: 1
                                }
                                contentItem: Text {
                                    text: statusEditCombo.displayText
                                    color: statusEditCombo.currentIndex === 2 ? "#16A34A" : statusEditCombo.currentIndex === 3 ? "#6B7280" : "#111827"
                                    font: statusEditCombo.font
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 10
                                    rightPadding: 28
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: tagLayout.implicitHeight + 18
                        radius: 8
                        color: tagField.text.trim().length > 0 ? "#F0FDF4" : "#F9FAFB"
                        border.color: tagField.text.trim().length > 0 ? "#BBF7D0" : "#E5E7EB"
                        border.width: 1

                        ColumnLayout {
                            id: tagLayout
                            anchors.fill: parent
                            anchors.margins: 9
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                TextField {
                                    id: tagField
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    Layout.preferredHeight: 38
                                    placeholderText: "添加标签，例如 #工作 #重要"
                                    font.pixelSize: 13
                                    selectByMouse: true
                                    background: Rectangle {
                                        color: "#FFFFFF"
                                        radius: 6
                                        border.color: tagField.activeFocus ? "#16A34A" : "#E5E7EB"
                                        border.width: 1
                                    }
                                    padding: 10
                                    onAccepted: root.addCurrentTagsToPresetBar()
                                }

                                Button {
                                    id: addTagPresetBtn
                                    Layout.preferredWidth: root.tiny ? 52 : root.compact ? 58 : 68
                                    Layout.preferredHeight: 38
                                    enabled: root.tagsFromText(tagField.text).length > 0
                                    text: root.compact ? "加入" : "加入栏"
                                    font.pixelSize: 12
                                    onClicked: root.addCurrentTagsToPresetBar()

                                    background: Rectangle {
                                        radius: 6
                                        color: !addTagPresetBtn.enabled ? "#F3F4F6" : addTagPresetBtn.down ? "#DCFCE7" : addTagPresetBtn.hovered ? "#ECFDF5" : "#FFFFFF"
                                        border.color: addTagPresetBtn.enabled ? "#86EFAC" : "#E5E7EB"
                                        border.width: 1
                                    }

                                    contentItem: Text {
                                        text: addTagPresetBtn.text
                                        color: addTagPresetBtn.enabled ? "#15803D" : "#9CA3AF"
                                        font: addTagPresetBtn.font
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            Flow {
                                id: defaultTagFlow
                                Layout.fillWidth: true
                                Layout.preferredHeight: implicitHeight
                                spacing: 6

                                Repeater {
                                    model: root.tagPresets

                                    delegate: Button {
                                        id: presetTagButton
                                        property bool selected: root.hasTagInField(modelData)

                                        text: modelData
                                        height: 30
                                        width: Math.max(56, presetTagLabel.implicitWidth + 24)
                                        font.pixelSize: 12

                                        background: Rectangle {
                                            radius: 15
                                            color: presetTagButton.selected ? "#16A34A" : presetTagButton.hovered ? "#ECFDF5" : "#FFFFFF"
                                            border.color: presetTagButton.selected ? "#16A34A" : "#BBF7D0"
                                            border.width: 1
                                        }

                                        contentItem: Text {
                                            id: presetTagLabel
                                            text: "#" + presetTagButton.text
                                            color: presetTagButton.selected ? "#FFFFFF" : "#15803D"
                                            font: presetTagButton.font
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            elide: Text.ElideRight
                                        }

                                        onClicked: root.toggleTagPreset(modelData)
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 4
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: "#FFFFFF"
            border.color: "#DADAE0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                spacing: 10

                Text {
                    Layout.fillWidth: true
                    text: root.errorText
                    color: "#DC2626"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                Button {
                    id: cancelBtn
                    Layout.preferredWidth: root.tiny ? 68 : 82
                    Layout.preferredHeight: 34
                    text: "取消"
                    onClicked: root.close()
                    background: Rectangle {
                        color: cancelBtn.down ? "#D1D5DB" : cancelBtn.hovered ? "#E5E5EA" : "#F5F5F7"
                        radius: 17
                        border.color: "#DADAE0"
                        border.width: 1
                        scale: cancelBtn.down ? 0.96 : 1.0
                        Behavior on color {
                            ColorAnimation {
                                duration: 150
                            }
                        }
                        Behavior on scale {
                            NumberAnimation {
                                duration: 120
                                easing.type: Easing.OutCubic
                            }
                        }
                    }
                    contentItem: Text {
                        text: cancelBtn.text
                        color: "#374151"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    id: submitBtn
                    Layout.preferredWidth: root.tiny ? (root.isEdit ? 82 : 68) : root.isEdit ? 96 : 82
                    Layout.preferredHeight: 34
                    text: root.isEdit ? "保存修改" : "创建"
                    onClicked: root.save()
                    background: Rectangle {
                        color: submitBtn.down ? "#0062CC" : submitBtn.hovered ? "#0A84FF" : "#007AFF"
                        radius: 17
                        scale: submitBtn.down ? 0.96 : 1.0
                        Behavior on color {
                            ColorAnimation {
                                duration: 150
                            }
                        }
                        Behavior on scale {
                            NumberAnimation {
                                duration: 120
                                easing.type: Easing.OutCubic
                            }
                        }
                    }
                    contentItem: Text {
                        text: submitBtn.text
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    Popup {
            id: calendarPopup
            parent: root.contentItem
            readonly property var anchorButton: root.calendarAnchor ? root.calendarAnchor : (root.calendarTarget === "start" ? primaryStartDateButton : primaryEndDateButton)
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
                        background: Rectangle {
                            color: parent.hovered ? "#F3F4F6" : "transparent"
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#374151"
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: (root.calendarRangeMode ? (root.waitingForRangeEnd ? "选择结束 · " : "选择开始 · ") : "") + root.calendarYear + "年 " + (root.calendarMonth + 1) + "月"
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
                        background: Rectangle {
                            color: parent.hovered ? "#F3F4F6" : "transparent"
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
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
                        readonly property bool isStart: cellDate.getTime() === root.dayStart(root.startDateValue).getTime()
                        readonly property bool isEnd: cellDate.getTime() === root.dayStart(root.endDateValue).getTime()
                        readonly property bool isCurrentTarget: model.day === root.selectedDate.getDate() && model.month === root.selectedDate.getMonth() && monthGrid.year === root.selectedDate.getFullYear()
                        readonly property bool isActiveStart: (root.calendarRangeMode ? !root.waitingForRangeEnd : root.calendarTarget === "start") && isStart
                        readonly property bool isActiveEnd: (root.calendarRangeMode ? root.waitingForRangeEnd : root.calendarTarget === "end") && isEnd
                        readonly property bool isActiveSelection: isActiveStart || isActiveEnd || !root.dateRangeEnabled && isCurrentTarget
                        readonly property bool isInactiveEndpoint: root.dateRangeEnabled && (isStart || isEnd) && !isActiveSelection
                        readonly property bool isInRange: root.dateRangeEnabled && cellDate.getTime() >= root.dayStart(root.startDateValue).getTime() && cellDate.getTime() <= root.dayStart(root.endDateValue).getTime()
                        readonly property bool isToday: model.day === new Date().getDate() && model.month === new Date().getMonth() && monthGrid.year === new Date().getFullYear()

                        color: isActiveSelection ? "#007AFF"
                               : isInactiveEndpoint ? "#D1E7FF"
                               : isInRange ? "#E5F1FF"
                               : dayMouse.containsMouse ? "#EFF6FF" : "transparent"
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
                            id: dayMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if (root.setDatePickerValue(new Date(monthGrid.year, model.month, model.day)))
                                    calendarPopup.close();
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
                            if (root.setDatePickerValue(new Date()))
                                calendarPopup.close();
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        text: "明天"
                        onClicked: {
                            const tomorrow = new Date();
                            tomorrow.setDate(tomorrow.getDate() + 1);
                            if (root.setDatePickerValue(tomorrow))
                                calendarPopup.close();
                        }
                    }
                }
            }
        }
}
