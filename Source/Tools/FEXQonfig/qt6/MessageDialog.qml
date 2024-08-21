import QtQuick.Dialogs as FromQt

FromQt.MessageDialog {
    readonly property int buttonSave: MessageDialog.Save
    readonly property int buttonDiscard: MessageDialog.Discard
    readonly property int buttonCancel: MessageDialog.Cancel

    property var enabledButtons
    buttons: enabledButtons
}
