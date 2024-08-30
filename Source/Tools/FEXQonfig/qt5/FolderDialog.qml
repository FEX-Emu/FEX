import QtQuick.Dialogs 1.3 as FromQt

FromQt.FileDialog {
    property url currentFolder
    property url selectedFolder

    folder: currentFolder

    selectFolder: true

    onAccepted: selectedFolder = fileUrl
}
