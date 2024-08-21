import QtQuick.Dialogs 1.3 as FromQt

FromQt.MessageDialog {
    readonly property int buttonSave: FromQt.Dialog.Save
    readonly property int buttonDiscard: FromQt.Dialog.Discard
    readonly property int buttonCancel: FromQt.Dialog.Cancel

    property var enabledButtons

    standardButtons: enabledButtons

    signal buttonClicked(button: int)

    onAccepted: buttonClicked(buttonSave)
    onDiscard: buttonClicked(buttonDiscard)
    onRejected: buttonClicked(buttonCancel)
}
