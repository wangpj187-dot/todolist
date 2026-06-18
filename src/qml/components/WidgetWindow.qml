import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import TodoApp 1.0

ApplicationWindow {
    id: widgetRoot
    visible: false

    // Widget window flags: frameless, always-on-top, tool window (no taskbar entry)
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool

    // Default widget size
    width: 320
    height: 400
    minimumWidth: 280
    minimumHeight: 200

    // Window opacity from config
    opacity: configService.windowOpacity

    // Apply alwaysOnTop from config
    Connections {
        target: configService
        function onAlwaysOnTopChanged() {
            if (configService.alwaysOnTop) {
                widgetRoot.flags = widgetRoot.flags | Qt.WindowStaysOnTopHint
            } else {
                widgetRoot.flags = widgetRoot.flags & ~Qt.WindowStaysOnTopHint
            }
        }
    }

    // -------------------------------------------------------------------------
    // Properties
    // -------------------------------------------------------------------------

    property bool isCompactMode: true
    property point dragStartPos
    property bool isDragging: false

    // -------------------------------------------------------------------------
    // Custom drag area for frameless window
    // -------------------------------------------------------------------------

    MouseArea {
        id: dragArea
        anchors.fill: parent
        onPressed: {
            isDragging = true
            dragStartPos = Qt.point(mouse.x, mouse.y)
        }
        onPositionChanged: {
            if (isDragging) {
                widgetRoot.x += mouse.x - dragStartPos.x
                widgetRoot.y += mouse.y - dragStartPos.y
            }
        }
        onReleased: {
            isDragging = false
            // Save window position when drag ends
            configService.saveWindowPosition(widgetRoot.x, widgetRoot.y)
        }
    }

    // -------------------------------------------------------------------------
    // Main content
    // -------------------------------------------------------------------------

    ColumnLayout {
        id: widgetLayout
        anchors.fill: parent
        spacing: 0

        // ---------------------------------------------------------------------
        // Widget header (drag handle + controls)
        // ---------------------------------------------------------------------

        Rectangle {
            id: widgetHeader
            Layout.fillWidth: true
            height: 36
            color: "#1F2937"
            radius: 8
            bottomLeftRadius: 0
            bottomRightRadius: 0

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 8
                spacing: 8

                Text {
                    text: "待办事项"
                    color: "#FFFFFF"
                    font.pixelSize: 13
                    font.bold: true
                }

                Item {
                    Layout.fillWidth: true
                }

                // Mode toggle button
                Button {
                    id: modeToggleBtn
                    text: isCompactMode ? "展开" : "收起"
                    font.pixelSize: 11
                    padding: 4
                    background: Rectangle {
                        color: "#374151"
                        radius: 4
                    }
                    contentItem: Text {
                        text: modeToggleBtn.text
                        color: "#FFFFFF"
                        font: modeToggleBtn.font
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                    onClicked: {
                        isCompactMode = !isCompactMode
                        widgetRoot.height = isCompactMode ? 400 : 600
                    }
                }

                // Close button
                Button {
                    id: closeBtn
                    text: "×"
                    font.pixelSize: 16
                    padding: 4
                    width: 24
                    height: 24
                    background: Rectangle {
                        color: closeBtn.hovered ? "#EF4444" : "transparent"
                        radius: 4
                    }
                    contentItem: Text {
                        text: closeBtn.text
                        color: "#FFFFFF"
                        font: closeBtn.font
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                    onClicked: widgetRoot.hide()
                }
            }
        }

        // ---------------------------------------------------------------------
        // Widget content area
        // ---------------------------------------------------------------------

        Rectangle {
            id: widgetContent
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#F9FAFB"
            radius: 8
            topLeftRadius: 0
            topRightRadius: 0
            border.color: "#E5E7EB"
            border.width: 1

            // Show compact view or full view based on mode
            WidgetView {
                id: compactView
                anchors.fill: parent
                visible: isCompactMode
            }

            // Full view (reuses main list view pattern)
            ColumnLayout {
                id: fullView
                anchors.fill: parent
                anchors.margins: 12
                visible: !isCompactMode
                spacing: 8

                Text {
                    text: "全部待办"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#111827"
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: todoService.todos
                    delegate: TodoItem {}
                    spacing: 6
                    clip: true
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Resize handle (bottom-right corner)
    // -------------------------------------------------------------------------

    Item {
        id: resizeHandle
        width: 16
        height: 16
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeFDiagCursor
            onPressed: {
                widgetRoot.dragStartPos = Qt.point(
                    widgetRoot.width - mouse.x,
                    widgetRoot.height - mouse.y
                )
            }
            onPositionChanged: {
                if (pressed) {
                    widgetRoot.width = Math.max(widgetRoot.minimumWidth, mouse.x + widgetRoot.dragStartPos.x)
                    widgetRoot.height = Math.max(widgetRoot.minimumHeight, mouse.y + widgetRoot.dragStartPos.y)
                }
            }
            onReleased: {
                configService.saveWindowSize(widgetRoot.width, widgetRoot.height)
            }
        }

        // Visual indicator
        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: 8
            height: 8
            color: "#D1D5DB"
            radius: 2
        }
    }

    // -------------------------------------------------------------------------
    // Window position/size persistence
    // -------------------------------------------------------------------------

    Component.onCompleted: {
        // Restore position and size from config
        if (configService.windowX !== 0 || configService.windowY !== 0) {
            widgetRoot.x = configService.windowX
            widgetRoot.y = configService.windowY
        }
        if (configService.windowWidth >= widgetRoot.minimumWidth) {
            widgetRoot.width = configService.windowWidth
        }
        if (configService.windowHeight >= widgetRoot.minimumHeight) {
            widgetRoot.height = configService.windowHeight
        }
    }

    // Save position when window is moved (via drag release handled above)
    // Save size when window is resized (via resize handle release handled above)
}
