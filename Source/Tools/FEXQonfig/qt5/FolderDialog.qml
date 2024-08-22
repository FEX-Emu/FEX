import QtQuick.Dialogs 1.3 as FromQt

FromQt.FileDialog {
    property url selectedFolder

    onAccepted: selectedFolder = fileUrl

    selectFolder: true
}
