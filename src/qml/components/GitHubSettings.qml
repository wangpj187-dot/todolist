import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import TodoApp 1.0

Rectangle {
    id: root
    width: parent ? parent.width : 600
    height: contentColumn.implicitHeight + 40
    radius: 8
    color: "#FFFFFF"
    border.color: "#E5E7EB"
    border.width: 1

    // -------------------------------------------------------------------------
    // Properties
    // -------------------------------------------------------------------------
    property string statusMessage: ""
    property bool statusIsSuccess: false
    property bool showStatus: false

    // -------------------------------------------------------------------------
    // Connections
    // -------------------------------------------------------------------------
    Connections {
        target: syncService
        function onSyncCompleted(success, message) {
            statusMessage = message
            statusIsSuccess = success
            showStatus = true
            statusTimer.restart()
        }
        function onSyncError(message) {
            statusMessage = message
            statusIsSuccess = false
            showStatus = true
            statusTimer.restart()
        }
    }

    // -------------------------------------------------------------------------
    // Timer for auto-hiding status messages
    // -------------------------------------------------------------------------
    Timer {
        id: statusTimer
        interval: 5000
        onTriggered: showStatus = false
    }

    // -------------------------------------------------------------------------
    // Functions
    // -------------------------------------------------------------------------
    function validateFields() {
        if (tokenField.text.trim().length === 0) {
            tokenField.focus = true
            statusMessage = "Please enter a Personal Access Token"
            statusIsSuccess = false
            showStatus = true
            statusTimer.restart()
            return false
        }
        if (repoField.text.trim().length === 0) {
            repoField.focus = true
            statusMessage = "Please enter a repository address"
            statusIsSuccess = false
            showStatus = true
            statusTimer.restart()
            return false
        }
        if (branchField.text.trim().length === 0) {
            branchField.focus = true
            statusMessage = "Please enter a branch name"
            statusIsSuccess = false
            showStatus = true
            statusTimer.restart()
            return false
        }
        return true
    }

    function testConnection() {
        if (!validateFields()) {
            return
        }
        const success = syncService.testConnection()
        if (success) {
            statusMessage = "Connection successful!"
            statusIsSuccess = true
        } else {
            statusMessage = "Connection failed. Please check your settings."
            statusIsSuccess = false
        }
        showStatus = true
        statusTimer.restart()
    }

    function saveSettings() {
        if (!validateFields()) {
            return
        }
        const token = tokenField.text.trim()
        const repo = repoField.text.trim()
        const branch = branchField.text.trim()

        const success = syncService.configureGitHub(token, repo, branch)
        if (success) {
            // Save to configService
            configService.githubToken = token
            configService.githubRepo = repo
            configService.githubBranch = branch

            statusMessage = "Settings saved successfully!"
            statusIsSuccess = true
        } else {
            statusMessage = "Failed to save settings. Please check your configuration."
            statusIsSuccess = false
        }
        showStatus = true
        statusTimer.restart()
    }

    // -------------------------------------------------------------------------
    // Content
    // -------------------------------------------------------------------------
    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // ---------------------------------------------------------------------
        // Title Section
        // ---------------------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: "GitHub Sync Settings"
                font.pixelSize: 20
                font.bold: true
                color: "#111827"
            }

            Text {
                text: "Configure GitHub repository for cloud backup"
                font.pixelSize: 13
                color: "#6B7280"
            }
        }

        // ---------------------------------------------------------------------
        // Status Message
        // ---------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            visible: showStatus && statusMessage.length > 0
            height: statusText.implicitHeight + 16
            radius: 6
            color: statusIsSuccess ? "#ECFDF5" : "#FEF2F2"
            border.color: statusIsSuccess ? "#10B981" : "#EF4444"
            border.width: 1

            Text {
                id: statusText
                text: statusMessage
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
                color: statusIsSuccess ? "#10B981" : "#EF4444"
                wrapMode: Text.Wrap
            }
        }

        // ---------------------------------------------------------------------
        // GitHub Token Field
        // ---------------------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "Personal Access Token"
                font.pixelSize: 13
                color: "#374151"
            }

            TextField {
                id: tokenField
                Layout.fillWidth: true
                placeholderText: "ghp_xxxxxxxxxxxxxxxxxxxx"
                font.pixelSize: 14
                echoMode: TextInput.Password
                text: configService.githubToken || ""
                background: Rectangle {
                    color: "#F9FAFB"
                    radius: 6
                    border.color: tokenField.activeFocus ? "#3B82F6" : "#E5E7EB"
                    border.width: 1
                }
                padding: 10
            }

            Text {
                text: "Create a token at github.com/settings/tokens with repo scope"
                font.pixelSize: 12
                color: "#9CA3AF"
            }
        }

        // ---------------------------------------------------------------------
        // Repository Field
        // ---------------------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "Repository"
                font.pixelSize: 13
                color: "#374151"
            }

            TextField {
                id: repoField
                Layout.fillWidth: true
                placeholderText: "username/repository"
                font.pixelSize: 14
                text: configService.githubRepo || ""
                background: Rectangle {
                    color: "#F9FAFB"
                    radius: 6
                    border.color: repoField.activeFocus ? "#3B82F6" : "#E5E7EB"
                    border.width: 1
                }
                padding: 10
            }
        }

        // ---------------------------------------------------------------------
        // Branch Field
        // ---------------------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "Branch"
                font.pixelSize: 13
                color: "#374151"
            }

            TextField {
                id: branchField
                Layout.fillWidth: true
                placeholderText: "main"
                font.pixelSize: 14
                text: configService.githubBranch || "main"
                background: Rectangle {
                    color: "#F9FAFB"
                    radius: 6
                    border.color: branchField.activeFocus ? "#3B82F6" : "#E5E7EB"
                    border.width: 1
                }
                padding: 10
            }
        }

        // ---------------------------------------------------------------------
        // Optional Settings Section
        // ---------------------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#E5E7EB"
            }

            Text {
                text: "Optional Settings"
                font.pixelSize: 14
                font.bold: true
                color: "#374151"
            }

            // Auto-sync toggle
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                CheckBox {
                    id: autoSyncCheck
                    checked: configService.autoSync || false
                    onToggled: {
                        configService.autoSync = checked
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: "Auto-sync"
                        font.pixelSize: 13
                        color: "#374151"
                    }

                    Text {
                        text: "Automatically sync changes to GitHub"
                        font.pixelSize: 12
                        color: "#9CA3AF"
                    }
                }
            }

            // Sync interval
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                visible: autoSyncCheck.checked

                Text {
                    text: "Sync interval (minutes):"
                    font.pixelSize: 13
                    color: "#374151"
                }

                SpinBox {
                    id: syncIntervalSpin
                    Layout.preferredWidth: 100
                    from: 1
                    to: 1440
                    value: configService.syncInterval || 30
                    editable: true
                    background: Rectangle {
                        color: "#F9FAFB"
                        radius: 6
                        border.color: syncIntervalSpin.activeFocus ? "#3B82F6" : "#E5E7EB"
                        border.width: 1
                    }
                    contentItem: Text {
                        text: syncIntervalSpin.value
                        color: "#111827"
                        font: syncIntervalSpin.font
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                    onValueChanged: {
                        configService.syncInterval = value
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // Action Buttons
        // ---------------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                id: testConnectionBtn
                text: "Test Connection"
                font.pixelSize: 14
                padding: 10
                Layout.preferredWidth: 160
                background: Rectangle {
                    color: "#F3F4F6"
                    radius: 6
                    border.color: "#E5E7EB"
                    border.width: 1
                }
                contentItem: Text {
                    text: testConnectionBtn.text
                    color: "#374151"
                    font: testConnectionBtn.font
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                onClicked: testConnection()
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                id: saveBtn
                text: "Save Settings"
                font.pixelSize: 14
                padding: 10
                Layout.preferredWidth: 160
                background: Rectangle {
                    color: "#3B82F6"
                    radius: 6
                }
                contentItem: Text {
                    text: saveBtn.text
                    color: "#FFFFFF"
                    font: saveBtn.font
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                onClicked: saveSettings()
            }
        }

        // ---------------------------------------------------------------------
        // Configuration Status
        // ---------------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                width: 8
                height: 8
                radius: 4
                color: syncService.isConfigured ? "#10B981" : "#9CA3AF"
            }

            Text {
                text: syncService.isConfigured ? "GitHub sync is configured" : "GitHub sync is not configured"
                font.pixelSize: 12
                color: syncService.isConfigured ? "#10B981" : "#9CA3AF"
            }
        }
    }
}
