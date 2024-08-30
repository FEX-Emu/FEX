import QtQuick.Dialogs as FromQt

FromQt.FileDialog {
    property bool selectExisting: true
    property bool selectMultiple: false
    fileMode: selectMultiple ? FileDialog.OpenFiles : selectExisting ? FileDialog.OpenFile : FileDialog.SaveFile
}
