import QtQuick 2.15
import QtQuick.Dialogs 1.3 as FromQt

Item {
    id: dialogParent
    property alias text: child.text
    property alias title: child.title

    readonly property int buttonSave: FromQt.Dialog.Save
    readonly property int buttonDiscard: FromQt.Dialog.Discard
    readonly property int buttonCancel: FromQt.Dialog.Cancel

    property int buttons

    signal buttonClicked(button: int)

    property bool pendingResult: false

    function open() {
        // Workaround for QTBUG-91650, due to which signals may get emitted twice
        pendingResult = true
        child.open()
    }

    FromQt.MessageDialog {
        id: child

        standardButtons: buttons

        onAccepted: {
            if (pendingResult) {
                dialogParent.buttonClicked(buttonSave)
                pendingResult = false
            }
        }
        onDiscard: {
            if (pendingResult) {
                dialogParent.buttonClicked(buttonDiscard)
                pendingResult = false
            }
        }
        onRejected: {
            if (pendingResult) {
                dialogParent.buttonClicked(buttonCancel)
                pendingResult = false
            }
        }
    }
}
