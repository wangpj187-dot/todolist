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
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 180; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.94; to: 1.0; duration: 180; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 140; easing.type: Easing.InCubic }
        NumberAnimation { property: "scale"; from: 1.0; to: 0.96; duration: 140; easing.type: Easing.InCubic }
    }

    property bool isEdit: false
    property string editTodoId: ""
    property string editTitle: ""
    property string editDescription: ""
    property int editPriority: Todo.Medium
    property var editDueDate: null
    property int editStatus: Todo.Pending
    property var editTags: []

    property bool submitted: false
    property string errorText: ""
    property var selectedDate: new Date()
    property int calendarMonth: selectedDate.getMonth()
    property int calendarYear: selectedDate.getFullYear()
    readonly property string flagTag: "旗标"
    readonly property var baseTagPresets: ["工作", "学习", "娱乐", "消费", "羽毛球", "健身"]
    property var tagPresets: ["工作", "学习", "娱乐", "消费", "羽毛球", "健身"]

    signal saved(var todoId)

    function openForCreate() {
        refreshTagPresets()
        resetForm()
        isEdit = false
        open()
        Qt.callLater(function() { titleField.forceActiveFocus() })
    }

    function openForEdit(todoId, title, description, priority, categoryId, dueDate, status, tags) {
        refreshTagPresets()
        resetForm()

        editTodoId = todoId
        editTitle = title
        editDescription = description
        editPriority = priority
        editDueDate = dueDate
        editStatus = status || Todo.Pending
        editTags = tagsToArray(tags)
        isEdit = true
        tagPresets = mergeTagPresets(tagPresets.concat(editTags))

        titleField.text = editTitle
        descField.text = editDescription
        tagField.text = tagsToText(editTags)
        priorityRepeater.itemAt(Math.max(0, Math.min(3, editPriority - 1))).checked = true
        statusEditCombo.currentIndex = Math.max(0, Math.min(3, editStatus - 1))

        if (editDueDate && editDueDate instanceof Date && !isNaN(editDueDate.getTime())) {
            hasDueDate.checked = true
            setDueDateValue(editDueDate)
        }

        open()
        Qt.callLater(function() { titleField.forceActiveFocus() })
    }

    function resetForm() {
        submitted = false
        errorText = ""
        titleField.text = ""
        descField.text = ""
        tagField.text = ""
        priorityRepeater.itemAt(Todo.Medium - 1).checked = true
        statusEditCombo.currentIndex = Todo.Pending - 1
        hasDueDate.checked = false
        setDueDateValue(new Date())

        editTodoId = ""
        editTitle = ""
        editDescription = ""
        editPriority = Todo.Medium
        editDueDate = null
        editStatus = Todo.Pending
        editTags = []
    }

    function save() {
        submitted = true
        errorText = ""

        if (titleField.text.trim().length === 0) {
            errorText = "请先填写标题"
            titleField.forceActiveFocus()
            return
        }

        const title = titleField.text.trim()
        const description = descField.text.trim()
        const priority = priorityButtonGroup.checkedButton ? priorityButtonGroup.checkedButton.priorityValue : Todo.Medium
        const categoryId = ""
        const statusValue = root.isEdit ? statusEditCombo.currentIndex + 1 : editStatus
        const tags = parseTagsForSave()
        let dueDate = null

        if (hasDueDate.checked) {
            dueDate = new Date(selectedDate.getFullYear(), selectedDate.getMonth(), selectedDate.getDate(),
                               dueHourSpin.value, dueMinuteSpin.value, 0)
        }

        if (isEdit) {
            if (!todoService.updateTodoFromQmlWithTags(editTodoId, title, description, priority, categoryId, dueDate, statusValue, tags)) {
                errorText = "保存失败，请检查输入后重试"
                return
            }
            saved(editTodoId)
        } else {
            const newId = todoService.createTodoFromQmlWithTags(title, description, priority, categoryId, dueDate, tags)
            if (!isValidId(newId)) {
                errorText = "创建失败，请检查输入后重试"
                return
            }
            saved(newId)
        }

        refreshTagPresets()
        root.close()
    }

    function isValidId(value) {
        const id = String(value || "")
        return id.length > 0 && id !== "{00000000-0000-0000-0000-000000000000}"
    }

    function pad2(value) {
        return ("0" + value).slice(-2)
    }

    function formatDate(date) {
        return date.getFullYear() + "-" + pad2(date.getMonth() + 1) + "-" + pad2(date.getDate())
    }

    function formatTime() {
        return pad2(dueHourSpin.value) + ":" + pad2(dueMinuteSpin.value)
    }

    function setDueDateValue(value) {
        const date = value instanceof Date && !isNaN(value.getTime()) ? value : new Date()
        selectedDate = date
        calendarMonth = date.getMonth()
        calendarYear = date.getFullYear()
        dueHourSpin.value = date.getHours()
        dueMinuteSpin.value = date.getMinutes()
    }

    function shiftCalendarMonth(delta) {
        const next = new Date(calendarYear, calendarMonth + delta, 1)
        calendarYear = next.getFullYear()
        calendarMonth = next.getMonth()
    }

    function tagsToArray(value) {
        if (!value)
            return []

        const result = []
        for (let i = 0; i < value.length; i++) {
            const tag = ("" + value[i]).trim()
            if (tag.length > 0 && result.indexOf(tag) === -1)
                result.push(tag)
        }
        return result
    }

    function tagsToText(tags) {
        const visibleTags = tagsToArray(tags).filter(function(tag) { return tag !== flagTag })
        return visibleTags.map(function(tag) { return "#" + tag }).join(" ")
    }

    function tagsFromText(text) {
        const result = []
        const parts = (text || "").split(/[#\s,，、;；]+/)

        function addTag(value) {
            const tag = (value || "").trim()
            if (tag.length > 0 && tag !== flagTag && result.indexOf(tag) === -1)
                result.push(tag)
        }

        for (let i = 0; i < parts.length; i++)
            addTag(parts[i])

        return result
    }

    function parseTagsForSave() {
        return tagsFromText(tagField.text)
    }

    function mergeTagPresets(values) {
        const result = []

        function addTag(value) {
            const tag = (value || "").trim()
            if (tag.length > 0 && tag !== flagTag && result.indexOf(tag) === -1)
                result.push(tag)
        }

        for (let i = 0; i < baseTagPresets.length; i++)
            addTag(baseTagPresets[i])

        if (values) {
            for (let j = 0; j < values.length; j++)
                addTag(values[j])
        }

        return result
    }

    function refreshTagPresets() {
        let tags = []
        if (typeof todoService !== "undefined" && todoService.getAllTags)
            tags = todoService.getAllTags()
        tagPresets = mergeTagPresets(tags)
    }

    function addCurrentTagsToPresetBar() {
        const typedTags = tagsFromText(tagField.text)
        if (typedTags.length === 0) {
            tagField.forceActiveFocus()
            return
        }

        tagPresets = mergeTagPresets(tagPresets.concat(typedTags))
        tagField.text = tagsToText(typedTags)
        tagField.forceActiveFocus()
    }

    function hasTagInField(tag) {
        const tags = tagsFromText(tagField.text)
        const target = (tag || "").trim()
        for (let i = 0; i < tags.length; i++) {
            if (tags[i] === target)
                return true
        }
        return false
    }

    function toggleTagPreset(tag) {
        const target = (tag || "").trim()
        if (target.length === 0 || target === flagTag)
            return

        const result = []
        const tags = tagsFromText(tagField.text)
        let removed = false

        for (let i = 0; i < tags.length; i++) {
            const value = tags[i]
            if (value.length === 0)
                continue
            if (value === target) {
                removed = true
                continue
            }
            if (result.indexOf(value) === -1)
                result.push(value)
        }

        if (!removed)
            result.push(target)

        tagField.text = result.map(function(value) { return "#" + value }).join(" ")
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
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                id: formBody
                width: formScrollView.availableWidth
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
                        border.color: root.submitted && titleField.text.trim().length === 0 ? "#DC2626"
                                      : titleField.activeFocus ? "#007AFF" : "#E5E7EB"
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
                columns: root.compact ? 1 : 2
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
                                { label: "低", value: Todo.Low, color: "#16A34A" },
                                { label: "中", value: Todo.Medium, color: "#CA8A04" },
                                { label: "高", value: Todo.High, color: "#EA580C" },
                                { label: "紧急", value: Todo.Urgent, color: "#DC2626" }
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
                    implicitHeight: dueDateLayout.implicitHeight + 18
                    radius: 8
                    color: hasDueDate.checked ? "#EFF6FF" : "#F9FAFB"
                    border.color: hasDueDate.checked ? "#BFDBFE" : "#E5E7EB"
                    border.width: 1

                    ColumnLayout {
                        id: dueDateLayout
                        anchors.fill: parent
                        anchors.margins: 9
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            CheckBox {
                                id: hasDueDate
                                text: "设置截止日期"
                                font.pixelSize: 13
                                checked: false
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: hasDueDate.checked
                                text: formatDate(root.selectedDate) + " " + root.formatTime()
                                color: "#007AFF"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignRight
                                elide: Text.ElideRight
                            }
                        }

                        ColumnLayout {
                            id: dueDateRow
                            Layout.fillWidth: true
                            spacing: 8
                            visible: hasDueDate.checked

                            Button {
                                id: dateButton
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                text: formatDate(root.selectedDate)
                                onClicked: calendarPopup.open()
                                background: Rectangle {
                                    color: "#FFFFFF"
                                    radius: 6
                                    border.color: calendarPopup.opened ? "#007AFF" : "#E5E7EB"
                                    border.width: 1
                                }
                                contentItem: RowLayout {
                                    spacing: 8
                                    Text {
                                        Layout.fillWidth: true
                                        text: dateButton.text
                                        color: "#111827"
                                        font.pixelSize: 14
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    Text {
                                        text: "选择"
                                        color: "#007AFF"
                                        font.pixelSize: 12
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                SpinBox {
                                    id: dueHourSpin
                                    Layout.preferredWidth: 76
                                    Layout.preferredHeight: 40
                                    from: 0
                                    to: 23
                                    editable: true
                                    font.pixelSize: 14
                                    textFromValue: function(value) { return root.pad2(value) }
                                    valueFromText: function(text) { return Number(text) }
                                    background: Rectangle {
                                        color: "#FFFFFF"
                                        radius: 6
                                        border.color: dueHourSpin.activeFocus ? "#007AFF" : "#E5E7EB"
                                        border.width: 1
                                    }
                                }

                                Text {
                                    text: ":"
                                    color: "#6B7280"
                                    font.pixelSize: 16
                                    Layout.preferredWidth: 8
                                    Layout.preferredHeight: 40
                                    verticalAlignment: Text.AlignVCenter
                                }

                                SpinBox {
                                    id: dueMinuteSpin
                                    Layout.preferredWidth: 76
                                    Layout.preferredHeight: 40
                                    from: 0
                                    to: 59
                                    editable: true
                                    font.pixelSize: 14
                                    textFromValue: function(value) { return root.pad2(value) }
                                    valueFromText: function(text) { return Number(text) }
                                    background: Rectangle {
                                        color: "#FFFFFF"
                                        radius: 6
                                        border.color: dueMinuteSpin.activeFocus ? "#007AFF" : "#E5E7EB"
                                        border.width: 1
                                    }
                                }

                                Item { Layout.fillWidth: true }
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
                                color: statusEditCombo.currentIndex === 2 ? "#16A34A"
                                       : statusEditCombo.currentIndex === 3 ? "#6B7280" : "#111827"
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
                                    color: !addTagPresetBtn.enabled ? "#F3F4F6"
                                           : addTagPresetBtn.down ? "#DCFCE7"
                                           : addTagPresetBtn.hovered ? "#ECFDF5" : "#FFFFFF"
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
                                        color: presetTagButton.selected
                                               ? "#16A34A"
                                               : presetTagButton.hovered ? "#ECFDF5" : "#FFFFFF"
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
                    Layout.preferredWidth: root.tiny ? 68 : 82
                    Layout.preferredHeight: 34
                    id: cancelBtn
                    text: "取消"
                    onClicked: root.close()
                    background: Rectangle {
                        color: cancelBtn.down ? "#D1D5DB" : cancelBtn.hovered ? "#E5E5EA" : "#F5F5F7"
                        radius: 17
                        border.color: "#DADAE0"
                        border.width: 1
                        scale: cancelBtn.down ? 0.96 : 1.0
                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
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
                    Layout.preferredWidth: root.tiny ? (root.isEdit ? 82 : 68) : root.isEdit ? 96 : 82
                    Layout.preferredHeight: 34
                    id: submitBtn
                    text: root.isEdit ? "保存修改" : "创建"
                    onClicked: root.save()
                    background: Rectangle {
                        color: submitBtn.down ? "#0062CC" : submitBtn.hovered ? "#0A84FF" : "#007AFF"
                        radius: 17
                        scale: submitBtn.down ? 0.96 : 1.0
                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
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
        readonly property real preferredX: dateButton.mapToItem(root.contentItem, 0, 0).x
        readonly property real preferredY: dateButton.mapToItem(root.contentItem, 0, dateButton.height + 6).y
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

                    readonly property bool isSelected: model.day === root.selectedDate.getDate()
                                                       && model.month === root.selectedDate.getMonth()
                                                       && monthGrid.year === root.selectedDate.getFullYear()
                    readonly property bool isToday: model.day === new Date().getDate()
                                                    && model.month === new Date().getMonth()
                                                    && monthGrid.year === new Date().getFullYear()

                    color: isSelected ? "#007AFF" : dayMouse.containsMouse ? "#EFF6FF" : "transparent"
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
                            root.selectedDate = new Date(monthGrid.year, model.month, model.day,
                                                         dueHourSpin.value, dueMinuteSpin.value, 0)
                            root.calendarMonth = root.selectedDate.getMonth()
                            root.calendarYear = root.selectedDate.getFullYear()
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
                        root.setDueDateValue(new Date())
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
                        root.setDueDateValue(tomorrow)
                        calendarPopup.close()
                    }
                }
            }
        }
    }
}
