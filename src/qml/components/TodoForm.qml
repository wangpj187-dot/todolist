import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import TodoApp 1.0

Dialog {
    id: root
    title: isEdit ? "编辑待办" : "新建待办"
    width: 480
    height: implicitHeight
    modal: true
    closePolicy: Popup.NoAutoClose

    // -------------------------------------------------------------------------
    // Properties
    // -------------------------------------------------------------------------
    property bool isEdit: false
    property string editTodoId: ""
    property string editTitle: ""
    property string editDescription: ""
    property int editPriority: Todo.Medium
    property string editCategoryId: ""
    property var editDueDate: null
    property int editStatus: Todo.Pending

    // -------------------------------------------------------------------------
    // Signal
    // -------------------------------------------------------------------------
    signal saved(var todoId)

    // -------------------------------------------------------------------------
    // Functions
    // -------------------------------------------------------------------------
    function openForCreate() {
        resetForm()
        isEdit = false
        open()
    }

    function openForEdit(todoId, title, description, priority, categoryId, dueDate, status) {
        editTodoId = todoId
        editTitle = title
        editDescription = description
        editPriority = priority
        editCategoryId = categoryId
        editDueDate = dueDate
        editStatus = status || Todo.Pending
        isEdit = true

        // Populate form fields
        titleField.text = editTitle
        descField.text = editDescription
        priorityCombo.currentIndex = editPriority - 1

        // Set category
        let catIndex = -1
        for (let i = 0; i < categoryCombo.model.length; i++) {
            if (categoryCombo.model[i].id === editCategoryId) {
                catIndex = i
                break
            }
        }
        categoryCombo.currentIndex = catIndex

        // Set due date
        if (editDueDate && editDueDate instanceof Date && !isNaN(editDueDate.getTime())) {
            hasDueDate.checked = true
            setDueDateValue(editDueDate)
        } else {
            hasDueDate.checked = false
        }

        open()
    }

    function resetForm() {
        titleField.text = ""
        descField.text = ""
        priorityCombo.currentIndex = Todo.Medium - 1
        categoryCombo.currentIndex = -1
        hasDueDate.checked = false
        setDueDateValue(new Date())
        titleField.focus = true

        editTodoId = ""
        editTitle = ""
        editDescription = ""
        editPriority = Todo.Medium
        editCategoryId = ""
        editDueDate = null
        editStatus = Todo.Pending
    }

    function save() {
        if (titleField.text.trim().length === 0) {
            titleField.focus = true
            return
        }

        const title = titleField.text.trim()
        const description = descField.text.trim()
        const priority = priorityCombo.currentIndex + 1  // 1-4
        const categoryId = categoryCombo.currentIndex >= 0 ? categoryCombo.model[categoryCombo.currentIndex].id : ""
        let dueDate = null

        if (hasDueDate.checked) {
            // Combine date and time
            const date = parseDateInput(dueDateField.text)
            if (!date) {
                dueDateField.forceActiveFocus()
                return
            }
            dueDate = new Date(date.getFullYear(), date.getMonth(), date.getDate(),
                              dueHourSpin.value, dueMinuteSpin.value, 0)
        }

        if (isEdit) {
            todoService.updateTodoFromQml(editTodoId, title, description, priority, categoryId, dueDate, editStatus)
            saved(editTodoId)
        } else {
            const newId = todoService.createTodoFromQml(title, description, priority, categoryId, dueDate)
            saved(newId)
        }

        root.close()
    }

    function pad2(value) {
        return ("0" + value).slice(-2)
    }

    function formatDateInput(date) {
        return date.getFullYear() + "-" + pad2(date.getMonth() + 1) + "-" + pad2(date.getDate())
    }

    function parseDateInput(text) {
        const match = /^(\d{4})-(\d{2})-(\d{2})$/.exec(text.trim())
        if (!match) {
            return null
        }

        const year = Number(match[1])
        const month = Number(match[2])
        const day = Number(match[3])
        const date = new Date(year, month - 1, day)

        if (date.getFullYear() !== year || date.getMonth() !== month - 1 || date.getDate() !== day) {
            return null
        }

        return date
    }

    function setDueDateValue(value) {
        const date = value instanceof Date && !isNaN(value.getTime()) ? value : new Date()
        dueDateField.text = formatDateInput(date)
        dueHourSpin.value = date.getHours()
        dueMinuteSpin.value = date.getMinutes()
    }

    // -------------------------------------------------------------------------
    // Content
    // -------------------------------------------------------------------------
    contentItem: ColumnLayout {
        spacing: 16
        anchors.margins: 20

        // Title field
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "标题"
                font.pixelSize: 13
                color: "#374151"
            }

            TextField {
                id: titleField
                Layout.fillWidth: true
                placeholderText: "请输入待办标题"
                font.pixelSize: 14
                focus: true
                background: Rectangle {
                    color: "#FFFFFF"
                    radius: 6
                    border.color: titleField.text.trim().length === 0 && titleField.activeFocus ? "#EF4444" : "#E5E7EB"
                    border.width: 1
                }
                padding: 10
            }
        }

        // Description field
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "描述"
                font.pixelSize: 13
                color: "#374151"
            }

            TextArea {
                id: descField
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                placeholderText: "请输入待办描述（可选）"
                font.pixelSize: 14
                wrapMode: Text.Wrap
                background: Rectangle {
                    color: "#FFFFFF"
                    radius: 6
                    border.color: "#E5E7EB"
                    border.width: 1
                }
                padding: 10
            }
        }

        // Priority and Category row
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            // Priority selector
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: "优先级"
                    font.pixelSize: 13
                    color: "#374151"
                }

                ComboBox {
                    id: priorityCombo
                    Layout.fillWidth: true
                    model: ["低", "中", "高", "紧急"]
                    currentIndex: Todo.Medium - 1
                    font.pixelSize: 14
                    background: Rectangle {
                        color: "#FFFFFF"
                        radius: 6
                        border.color: "#E5E7EB"
                        border.width: 1
                    }
                    contentItem: Text {
                        text: priorityCombo.displayText
                        color: "#111827"
                        font: priorityCombo.font
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 10
                        rightPadding: priorityCombo.indicator.width + 10
                    }
                    delegate: ItemDelegate {
                        width: priorityCombo.width
                        text: modelData
                        font.pixelSize: 14
                        background: Rectangle {
                            color: highlighted ? "#EFF6FF" : "transparent"
                        }
                    }
                    indicator: Canvas {
                        x: parent.width - width - 10
                        y: parent.height / 2 - height / 2
                        width: 12
                        height: 12
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.fillStyle = "#6B7280"
                            ctx.beginPath()
                            ctx.moveTo(0, 3)
                            ctx.lineTo(6, 9)
                            ctx.lineTo(12, 3)
                            ctx.closePath()
                            ctx.fill()
                        }
                    }
                }
            }

            // Category selector
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: "分类"
                    font.pixelSize: 13
                    color: "#374151"
                }

                ComboBox {
                    id: categoryCombo
                    Layout.fillWidth: true
                    model: todoService.categories
                    currentIndex: -1
                    font.pixelSize: 14
                    displayText: currentIndex >= 0 ? model[currentIndex].name : "无分类"
                    background: Rectangle {
                        color: "#FFFFFF"
                        radius: 6
                        border.color: "#E5E7EB"
                        border.width: 1
                    }
                    contentItem: Text {
                        text: categoryCombo.displayText
                        color: categoryCombo.currentIndex >= 0 ? "#111827" : "#9CA3AF"
                        font: categoryCombo.font
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 10
                        rightPadding: categoryCombo.indicator.width + 10
                    }
                    delegate: ItemDelegate {
                        width: categoryCombo.width
                        text: model.name
                        font.pixelSize: 14
                        background: Rectangle {
                            color: highlighted ? "#EFF6FF" : "transparent"
                        }
                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 8
                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                color: model.color || "#3B82F6"
                            }
                            Text {
                                text: model.name
                                color: "#111827"
                                font: parent.font
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                    indicator: Canvas {
                        x: parent.width - width - 10
                        y: parent.height / 2 - height / 2
                        width: 12
                        height: 12
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.fillStyle = "#6B7280"
                            ctx.beginPath()
                            ctx.moveTo(0, 3)
                            ctx.lineTo(6, 9)
                            ctx.lineTo(12, 3)
                            ctx.closePath()
                            ctx.fill()
                        }
                    }
                }
            }
        }

        // Due date section
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            CheckBox {
                id: hasDueDate
                text: "设置截止日期"
                font.pixelSize: 13
                checked: false
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                visible: hasDueDate.checked

                // Date picker
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "日期"
                        font.pixelSize: 13
                        color: "#374151"
                    }

                    TextField {
                        id: dueDateField
                        Layout.fillWidth: true
                        placeholderText: "yyyy-MM-dd"
                        inputMethodHints: Qt.ImhDate
                        font.pixelSize: 14
                        background: Rectangle {
                            color: "#FFFFFF"
                            radius: 6
                            border.color: "#E5E7EB"
                            border.width: 1
                        }
                        padding: 8
                    }
                }

                // Time picker
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "时间"
                        font.pixelSize: 13
                        color: "#374151"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        SpinBox {
                            id: dueHourSpin
                            Layout.fillWidth: true
                            from: 0
                            to: 23
                            editable: true
                            font.pixelSize: 14
                            textFromValue: function(value) { return root.pad2(value) }
                            valueFromText: function(text) { return Number(text) }
                            background: Rectangle {
                                color: "#FFFFFF"
                                radius: 6
                                border.color: "#E5E7EB"
                                border.width: 1
                            }
                        }

                        Text {
                            text: ":"
                            color: "#6B7280"
                            font.pixelSize: 16
                            verticalAlignment: Text.AlignVCenter
                        }

                        SpinBox {
                            id: dueMinuteSpin
                            Layout.fillWidth: true
                            from: 0
                            to: 59
                            editable: true
                            font.pixelSize: 14
                            textFromValue: function(value) { return root.pad2(value) }
                            valueFromText: function(text) { return Number(text) }
                            background: Rectangle {
                                color: "#FFFFFF"
                                radius: 6
                                border.color: "#E5E7EB"
                                border.width: 1
                            }
                        }
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Footer
    // -------------------------------------------------------------------------
    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Cancel | DialogButtonBox.Save
        spacing: 12
        padding: 16

        Button {
            text: "取消"
            font.pixelSize: 14
            padding: 10
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            background: Rectangle {
                color: "#F3F4F6"
                radius: 6
            }
            contentItem: Text {
                text: parent.text
                color: "#374151"
                font: parent.font
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
            onClicked: root.close()
        }

        Button {
            text: "保存"
            font.pixelSize: 14
            padding: 10
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            background: Rectangle {
                color: "#3B82F6"
                radius: 6
            }
            contentItem: Text {
                text: parent.text
                color: "#FFFFFF"
                font: parent.font
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
            onClicked: save()
        }
    }
}
