import QtQuick.Dialogs 1.3 as FromQt

FromQt.FileDialog {
    property url selectedFile

    onAccepted: selectedFile = fileUrl
}
