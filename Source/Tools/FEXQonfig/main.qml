import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import FEX.ConfigModel 1.0
import FEX.EnvVarModel 1.0
import FEX.RootFSModel 1.0

// Qt 6 changed the API of the Dialogs module slightly.
// The differences are abstracted away in this import:
import "qrc:/dialogs"

ApplicationWindow {
    id: root

    visible: true
    width: 640
    height: 600
    title: configDirty ? qsTr("FEX configuration *") : qsTr("FEX configuration")

    property url configFilename

    property bool configDirty: false // TODO: Consistently set to true on modifications
    property bool closeConfirmed: false

    signal selectedConfigFile(name: url)
    signal triggeredSave(name: url)

    // Property used to force reloading any elements that read ConfigModel
    property bool refreshCache: false

    function refreshUI() {
        refreshCache = !refreshCache
    }

    function urlToLocalFile(theurl: url) {
        var str = theurl.toString()
        if (str.startsWith("file://")) {
            return decodeURIComponent(str.substring(7))
        }

        return str;
    }

    FileDialog {
        id: openFileDialog
        title: qsTr("Open FEX configuration")
        nameFilters: [ "Config files(*.json)", "All files(*)" ]

        property var onNextAccept: null

        function openAndThen(callback) {
            console.assert(!onNextAccept, "Tried to open dialog multiple times")
            onNextAccept = callback
            open()
        }

        onAccepted: {
            root.selectedConfigFile(selectedFile)
            configFilename = selectedFile
            if (onNextAccept) {
                onNextAccept()
                onNextAccept = null
            }
        }

        onRejected: onNextAccept = null
    }

    MessageDialog {
        id: confirmCloseDialog
        title: qsTr("Save changes")
        text: configFilename.toString() === "" ? qsTr("Save changes before quitting?") : qsTr("Save changes to %1 before quitting?").arg(urlToLocalFile(configFilename))
        buttons: buttonSave | buttonDiscard | buttonCancel

        onButtonClicked: (button) => {
            switch (button) {
            case buttonSave:
                if (configFilename.toString() === "") {
                    // Filename not yet set => trigger "Save As" dialog
                    openFileDialog.openAndThen(() => {
                        save(configFilename)
                        root.close()
                    });
                    return
                }
                save(configFilename)
                root.close()
                break

            case buttonDiscard:
                closeConfirmed = true
                root.close()
                break
            }
        }
    }

    onClosing: (close) => {
        if (configDirty) {
            close.accepted = closeConfirmed
            onTriggered: confirmCloseDialog.open()
        }
    }

    function save(filename: url) {
        if (filename === "") {
            filename = configFilename
        }

        if (filename.toString() === "") {
            // Filename not yet set => trigger "Save As" dialog
            openFileDialog.openAndThen(() => {
                save(configFilename)
            });
            return
        }

        triggeredSave(filename)
        configDirty = false
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Open...")
                shortcut: StandardKey.Open
                // TODO: Ask to discard pending changes first
                onTriggered: openFileDialog.openAndThen(() => {})
            }
            Action {
                text: qsTr("&Save")
                shortcut: StandardKey.Save
                onTriggered: root.save("")
            }
            Action {
                // TODO
                text: qsTr("Save &as...")
                shortcut: StandardKey.SaveAs
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Quit")
                shortcut: StandardKey.Quit
                onTriggered: close()
            }
        }
    }

    header: TabBar {
        id: tabBar
        currentIndex: 0

        TabButton {
            text: qsTr("General")
        }
        TabButton {
            text: qsTr("Emulation")
        }
        TabButton {
            text: qsTr("CPU")
        }
        TabButton {
            text: qsTr("Advanced")
        }
    }

    component ConfigCheckBox: CheckBox {
        property string config
        property string tooltip
        property bool invert: false

        ToolTip.visible: (visualFocus || hovered) && tooltip !== ""
        ToolTip.text: tooltip

        onToggled: {
            configDirty = true
            ConfigModel.setBool(config, checked ^ invert)
        }

        checkState: config === "" ? Qt.PartiallyChecked
                    : !ConfigModel.has(config, refreshCache) ? Qt.PartiallyChecked
                    : (ConfigModel.getBool(config, refreshCache) ^ invert) ? Qt.Checked
                    : Qt.Unchecked
    }

    component ConfigSpinBox: SpinBox {
        property string config

        textFromValue: (val) => {
            if (valueFromConfig === "") {
                return qsTr("(not set)");
            }

            return val.toString()
        }

        onValueModified: {
            configDirty = true
            ConfigModel.setInt(config, value)
        }

        property string valueFromConfig: config === "" ? 0 : ConfigModel.has(config, refreshCache) ? ConfigModel.getInt(config, refreshCache).toString() : ""

        value: valueFromConfig
        from: 0
        to: 1 << 30
    }

    component ConfigTextField: TextField {
        property string config
        property bool hasData: config !== "" && ConfigModel.has(config, refreshCache)
        text: hasData ? ConfigModel.getString(config, refreshCache) : "(none set)"
        enabled: hasData

        onTextEdited: {
            configDirty = true
            ConfigModel.setString(config, text)
        }
    }

    component ConfigTextFieldForPath: RowLayout {
        property string text
        property string config
        property bool isFolder: false // TODO: Implement

        FileDialog {
            id: fileSelectorDialog
            onAccepted: {
                configDirty = true
                ConfigModel.setString(config, urlToLocalFile(selectedFile))
            }
        }

        Label { text: parent.text }
        ConfigTextField {
            Layout.fillWidth: true
            config: parent.config
            readOnly: true
        }

        Button {
            icon.name: "search"
            onClicked: fileSelectorDialog.open()
        }
    }

    StackLayout {
        // TODO: Make items scrollable

        // width: parent.width
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: tabBar.bottom
        anchors.bottom: parent.bottom

        currentIndex: tabBar.currentIndex

        // Environment settings
        Pane {
            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                GroupBox {
                    title: qsTr("RootFS:")
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.left: parent ? parent.left : undefined
                        anchors.right: parent ? parent.right : undefined

                        ListView {
                            model: RootFSModel
                            id: rootfsList

                            clip: true
                            Layout.fillWidth: true
                            height: count * dummyDelegate.height

                            RadioDelegate {
                                // Enable measuring line height with this invisible dummy element
                                id: dummyDelegate
                                visible: false
                            }

                            property string selectedItem: ConfigModel.has("RootFS", refreshCache) ? ConfigModel.getString("RootFS", refreshCache) : null

                            // TODO: Handle empty RootFS, or external RootFS not in .fex-emu

                            delegate: RadioDelegate {
                                anchors.left: parent ? parent.left : undefined
                                anchors.right: parent ? parent.right : undefined

                                text: edit
                                checked: rootfsList.selectedItem == edit

                                onToggled: {
                                    configDirty = true;
                                    ConfigModel.setString("RootFS", edit)
                                }
                            }
                        }

                        RowLayout {
                            Button {
                                text: qsTr("Select...")
                                icon.name: "search"
                                // TODO: Implement
                            }

                            Button {
                                text: qsTr("Download...")
                                icon.name: "download"
                                // TODO: Implement
                            }
                        }
                    }
                }

                GroupBox {
                    title: qsTr("Library Forwarding:")
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.left: parent ? parent.left : undefined
                        anchors.right: parent ? parent.right : undefined

                        ConfigTextFieldForPath {
                            text: qsTr("Configuration:")
                            config: "ThunkConfig"
                        }
                        ConfigTextFieldForPath {
                            text: qsTr("Host library folder:")
                            config: "ThunkHostLibs"
                        }
                        ConfigTextFieldForPath {
                            text: qsTr("Guest library folder:")
                            config: "ThunkGuestLibs"
                        }
                    }
                }

                GroupBox {
                    title: qsTr("Logging:")
                    Layout.fillWidth: true

                    label: ConfigCheckBox {
                        id: loggingEnabledCheckBox
                        config: "SilentLog"
                        text: qsTr("Logging")
                        invert: true
                    }

                    ColumnLayout {
                        enabled: loggingEnabledCheckBox.checked

                        anchors.left: parent ? parent.left : undefined
                        anchors.right: parent ? parent.right : undefined

                        RowLayout {
                            Label { text: qsTr("Log to:") }

                            ComboBox {
                                id: loggingComboBox
                                property string configValue: ConfigModel.has("OutputLog", refreshCache) ? ConfigModel.getString("OutputLog", refreshCache) : ""

                                currentIndex: configValue === "" ? -1 : configValue == "server" ? 0 : configValue == "stderr" ? 1 : configValue == "stdout" ? 2 : 3

                                onActivated: {
                                    configDirty = true
                                    var configNames = [ "server", "stderr", "stdout" ]
                                    if (currentIndex != -1 && currentIndex < 3) {
                                        ConfigModel.setString("OutputLog", configNames[currentIndex])
                                    } else {
                                        // Set by text field below
                                    }
                                }

                                model: ListModel {
                                    ListElement { text: "FEXServer" }
                                    ListElement { text: "stderr" }
                                    ListElement { text: "stdout" }
                                    ListElement { text: qsTr("File...") }
                                }
                            }

                            ConfigTextFieldForPath {
                                visible: loggingComboBox.currentIndex === 3
                                config: "OutputLog"
                            }
                        }
                    }
                }
            }
        }

        // Emulation settings
        Pane {
            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                RowLayout {
                    Label { text: qsTr("SMC detection:") }
                    ComboBox {
                        currentIndex: ConfigModel.has("SMCChecks", refreshCache) ? ConfigModel.getInt("SMCChecks", refreshCache) : -1

                        onActivated: {
                            configDirty = true
                            ConfigModel.setInt("SMCChecks", currentIndex)
                        }

                        model: ListModel {
                            ListElement { text: qsTr("None") }
                            ListElement { text: qsTr("MTrack") }
                            ListElement { text: qsTr("Full") }
                        }
                    }
                }

                GroupBox {
                    title: qsTr("Memory Model")
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.left: parent ? parent.left : undefined
                        anchors.right: parent ? parent.right : undefined

                        ButtonGroup {
                            id: tsoButtonGroup
                            buttons: [tso1, tso2, tso3]
                            // Trying to be too clever here will trigger property binding loops,
                            // so require both TSOEnabled and ParanoidTSO to be listed in the config.
                            // If they are not, the state will be displayed as undetermined.
                            checkedButton: !(ConfigModel.has("TSOEnabled", refreshCache) && (ConfigModel.has("ParanoidTSO", refreshCache))) ? null
                                            : ConfigModel.getBool("ParanoidTSO", refreshCache) ? tso3
                                            : ConfigModel.getBool("TSOEnabled", refreshCache) ? tso2 : tso1

                            property int pendingItemChange: -1

                            function onClickedButton(index: int) {
                                pendingItemChange = index;

                                configDirty = true;

                                var newIndex = pendingItemChange
                                var TSOEnabled = newIndex === 1
                                var ParanoidTSO = newIndex === 2
                                ConfigModel.setBool("ParanoidTSO", ParanoidTSO)
                                ConfigModel.setBool("TSOEnabled", TSOEnabled)

                                pendingItemChange = -1;
                            }

                            onClicked: {
                                if (pendingItemChange !== -1) {
                                    return;
                                }
                                pendingItemChange = tso1.checked ? 0 : tso2.checked ? 1 : tso3.checked ? 2 : -1;
                                if (pendingItemChange) {
                                    // Undetermined state, leave as is
                                    return;
                                }

                                var newIndex = pendingItemChange
                                var TSOEnabled = newIndex === 1
                                var ParanoidTSO = newIndex === 2
                                ConfigModel.setBool("ParanoidTSO", ParanoidTSO)
                                ConfigModel.setBool("TSOEnabled", TSOEnabled)

                                pendingItemChange = -1;
                            }
                        }

                        ColumnLayout {
                            RadioButton {
                                id: tso1
                                text: qsTr("Inaccurate")
                                onToggled: tsoButtonGroup.onClickedButton(0)
                            }

                            ColumnLayout {
                                RadioButton {
                                    id: tso2
                                    text: qsTr("Accurate (TSO)")
                                    onToggled: tsoButtonGroup.onClickedButton(1)
                                }

                                ColumnLayout {
                                    visible: tso2.checked

                                    ConfigCheckBox {
                                        leftPadding: 24
                                        text: qsTr("... for vector instructions")
                                        tooltip: qsTr("Controls TSO emulation on vector load/store instructions")
                                        config: "VectorTSOEnabled"
                                    }
                                    ConfigCheckBox {
                                        leftPadding: 24
                                        text: qsTr("... for memcpy instructions")
                                        tooltip: qsTr("Controls TSO emulation on memcpy/memset instructions")
                                        config: "MemcpySetTSOEnabled"
                                    }
                                    ConfigCheckBox {
                                        leftPadding: 24
                                        text: qsTr("... for unaligned half-barriers")
                                        tooltip: qsTr("Controls half-barrier TSO emulation on unaligned load/store instructions")
                                        config: "HalfBarrierTSOEnabled"
                                    }
                                }
                            }

                            RadioButton {
                                id: tso3
                                text: qsTr("Overly accurate (paranoid TSO)")
                                onToggled: tsoButtonGroup.onClickedButton(2)
                            }
                        }
                    }
                }

                component EnvVarList: GroupBox {
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.left: parent ? parent.left : undefined
                        anchors.right: parent ? parent.right : undefined

                        Repeater {
                            model: EnvVarModel
                            Layout.fillWidth: true
                            ItemDelegate { text: edit }
                        }

                        RowLayout {
                            // TODO: Save to config
                            TextField { Layout.fillWidth: true }
                            Button { icon.name : "list-add" }
                        }
                    }
                }

                EnvVarList {
                    title: qsTr("Guest environment variables:")
                }

                EnvVarList {
                    title: qsTr("Host environment variables:")
                }
            }
        }

        // CPU settings
        Pane {
            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                ConfigCheckBox {
                    text: qsTr("Multiblock")
                    config: "Multiblock"
                }

                RowLayout {
                    Layout.fillWidth: true

                    Label { text: qsTr("Block size:") }
                    ConfigSpinBox {
                        config: "MaxInst"
                        from: 0
                        to: 1 << 30
                    }
                }

                GroupBox {
                    title: qsTr("JIT caches:")
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.left: parent ? parent.left : undefined
                        anchors.right: parent ? parent.right : undefined

                        ConfigCheckBox {
                            text: qsTr("Generate AOT")
                            config: "AOTIRGenerate"
                        }
                        ConfigCheckBox {
                            text: qsTr("Capture AOT")
                            config: "AOTIRCapture"
                        }
                        ConfigCheckBox {
                            text: qsTr("Load AOT")
                            config: "AOTIRLoad"
                        }

                        RowLayout {
                            Label { text: qsTr("Cache object code:") }

                            ButtonGroup {
                                buttons: cacheObjCodeRadios.children

                                checkedButton: ConfigModel.has("CacheObjectCodeCompilation", refreshCache)
                                                ? cacheObjCodeRadios.children[ConfigModel.getInt("CacheObjectCodeCompilation", refreshCache)]
                                                : null

                                onClicked: (button) => {
                                    configDirty = true
                                    for (var idx in buttons) {
                                        if (button === buttons[idx]) {
                                            ConfigModel.setInt("CacheObjectCodeCompilation", idx)
                                            return
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                id: cacheObjCodeRadios
                                RadioButton {
                                    text: qsTr("Off")
                                }
                                RadioButton {
                                    text: qsTr("Read-only")
                                }
                                RadioButton {
                                    text: qsTr("Read & write")
                                }
                            }
                        }
                    }
                }

                ConfigCheckBox {
                    text: qsTr("Reduced x87 precision")
                    config: "X87ReducedPrecision"
                }

                ConfigCheckBox {
                    text: qsTr("Unsafe local flags optimization")
                    config: "ABILocalFlags"
                }

                ConfigCheckBox {
                    text: qsTr("Disable JIT optimization passes")
                    config: "O0"
                }
            }
        }

        // Advanced settings
        // TODO: Options contained multiple times in JSON aren't listed (neither are they in old FEXConfig though)
        Pane { Frame {
            anchors.fill: parent
            ListView {
                anchors.fill: parent
                clip: true
                model: ConfigModel

                delegate: RowLayout {
                    anchors.left: parent ? parent.left : undefined
                    anchors.right: parent ? parent.right : undefined

                    anchors.leftMargin: 4
                    anchors.rightMargin: 4

                    Label {
                        id: label
                        text: display
                    }

                    ConfigCheckBox {
                        visible: optionType == "bool"
                        config: visible ? label.text : ""
                    }

                    ConfigTextField {
                        Layout.fillWidth: true
                        visible: optionType == "fextl::string"
                        config: visible ? label.text : ""
                    }

                    ConfigSpinBox {
                        visible: optionType.startsWith("int") || optionType.startsWith("uint")
                        config: visible ? label.text : ""
                        from: 0
                        to: 1 << 30
                    }

                    // Spacing
                    Item {
                        Layout.fillWidth: true
                    }

                    Button {
                        icon.name: "list-remove"
                        onClicked: {
                            ConfigModel.erase(label.text)
                        }
                    }
                }
            }
        }}
    }

    footer: Pane {
        anchors.left: parent.left
        anchors.right: parent.right

        padding: 0

        ColumnLayout{
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 0

            ToolSeparator {
                Layout.fillWidth: true
                orientation: Qt.Horizontal

                // Override padding from theme.
                // Some themes use verticalPadding, others topPadding/bottomPadding, so we set them all.
                verticalPadding: 0
                bottomPadding: 0
                topPadding: 0
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                enabled: false
                text: configFilename.toString() === ""
                        ? qsTr("Config.json not found — loaded defaults")
                        : qsTr("Editing %1").arg(urlToLocalFile(configFilename))
            }
        }
    }
}
