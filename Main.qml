import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import Qt.labs.platform as Platform

import bankconv

ApplicationWindow {
    id: root
    height: 640
    width: 360
    visible: true
    title: qsTr("bankconv")

    property QtObject backend: Backend

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Choose folder")

                onTriggered: {
                    folderDialog.open()
                }
            }
            Action {
                text: qsTr("&Quit")

                onTriggered: {
                    Qt.quit()
                }
            }
        }
    }

    Platform.FolderDialog {
        id: folderDialog
        title: "Choose folder"

        // TODO start folder is not respected even if set
        folder: backend.folder
        currentFolder: backend.folder
        onAccepted: {
            backend.setFolder(folderDialog.folder)
        }
        Component.onCompleted: {
            folder = currentFolder
        }
    }

    ListView {
        anchors.fill: parent

        model: backend.pdfFiles

        delegate: Text {
            text: modelData
        }
    }
    footer: Rectangle {
        id: footer
        height: root.height * 0.2
        width: root.width

        Row {
            anchors.fill: parent
            Button {
                id: start
                enabled: backend.buttonStartEnabled
                height: parent.height
                width: root.width / 2
                text: "Convert files to .CSV"

                onPressed: {
                    backend.tryConvertToCSV()
                }
            }
        }
    }
}
