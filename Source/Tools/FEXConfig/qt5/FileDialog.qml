// SPDX-License-Identifier: MIT
import QtQuick.Dialogs 1.3 as FromQt

FromQt.FileDialog {
    property url selectedFile
    property url currentFolder

    folder: currentFolder
    onAccepted: selectedFile = fileUrl
}
