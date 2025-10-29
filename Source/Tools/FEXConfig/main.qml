// SPDX-License-Identifier: MIT
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import FEX.ConfigModel 1.0
import FEX.HostLibsModel 1.0
import FEX.RootFSModel 1.0

// Qt 6 changed the API of the Dialogs module slightly.
// The differences are abstracted away in this import:
import "qrc:/dialogs"

ApplicationWindow {
    id: root

    visible: true
    width: 540
    height: 585
    minimumWidth: 500
    minimumHeight: 450
    title: configDirty ? qsTr("FEX configuration *") : qsTr("FEX configuration")

    property url configFilename

    property bool configDirty: false
    property bool loadedDefaults: false
    property bool closeConfirmed: false

    signal selectedConfigFile(name: url)
    signal triggeredSave(name: url)

    // Property used to force reloading any elements that read ConfigModel
    property bool refreshCache: false

    onConfigDirtyChanged: {
        if (!configDirty) {
            // We either just saved or loaded a file
            loadedDefaults = false
        }
    }

    function refreshUI() {
        refreshCache = !refreshCache
    }

    function urlToLocalFile(theurl: url): string {
        var str = theurl.toString()
        if (str.startsWith("file://")) {
            return decodeURIComponent(str.substring(7))
        }
        if (str.startsWith("file:")) {
            return decodeURIComponent(str.substring(5))
        }

        return str;
    }

    FileDialog {
        id: openFileDialog
        property bool isSaving: false

        title: isSaving ? qsTr("Save FEX configuration") : qsTr("Open FEX configuration")
        nameFilters: [ qsTr("Config files(*.json)"), qsTr("All files(*)") ]

        selectExisting: !isSaving

        property var onNextAccept: null

        // Prompts the user for an existing file and calls the callback on completion
        function loadAndThen(callback) {
            isSaving = false
            console.assert(!onNextAccept, "Tried to open dialog multiple times")
            onNextAccept = callback
            open()
        }

        // Prompts the user for a new or existing file and calls the callback on completion
        function saveAndThen(callback) {
            isSaving = true
            console.assert(!onNextAccept, "Tried to open dialog multiple times")
            onNextAccept = callback
            open()
        }

        onAccepted: {
            if (!isSaving) {
                root.selectedConfigFile(selectedFile)
            }
            configFilename = selectedFile
            configDirty = false
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
                    openFileDialog.saveAndThen(() => {
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
        if (filename.toString() === "") {
            filename = configFilename
        }

        if (filename.toString() === "") {
            // Filename not yet set => trigger "Save As" dialog
            openFileDialog.saveAndThen(() => {
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
                onTriggered: openFileDialog.loadAndThen(() => {})
            }
            Action {
                text: qsTr("&Save")
                shortcut: StandardKey.Save
                onTriggered: root.save("")
            }
            Action {
                text: qsTr("Save &as...")
                shortcut: StandardKey.SaveAs
                onTriggered: {
                    openFileDialog.saveAndThen(() => {
                        root.save(configFilename)
                    });
                }
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

        readonly property int advancedIndex: 4

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
            text: qsTr("Libraries")
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

        editable: true

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

        property var dialog: FileDialog {}

        FileDialog { id: fileSelectorDialog }

        Label { text: parent.text }
        ConfigTextField {
            Layout.fillWidth: true
            config: parent.config
            readOnly: true
        }

        Button {
            icon.name: "search"
            onClicked: dialog.open()
        }

        Component.onCompleted: {
            dialog.accepted.connect(() => {
                var selectedPath = (dialog instanceof FileDialog ? dialog.selectedFile : dialog.selectedFolder)

                configDirty = true
                ConfigModel.setString(config, urlToLocalFile(selectedPath))
            })
        }
    }

    StackLayout {
        anchors.fill: parent

        currentIndex: tabBar.currentIndex

        component ScrollablePage: ScrollView {
            id: outer

            readonly property var visibleScrollbarWidth: ScrollBar.vertical.visible ? ScrollBar.vertical.width : 0

            // Children given by the user will be moved into the inner Column
            default property alias content: inner.children

            property alias itemSpacing: inner.spacing

            Column {
                id: inner

                spacing: 8
                padding: 8

                // This must be explicitly set via the id, since parent doesn't seem to be recognized within Column
                width: outer.width - outer.visibleScrollbarWidth
            }
        }

        // Environment settings
        ScrollablePage {
            GroupBox {
                id: rootfsGroupBox
                title: qsTr("RootFS:")
                width: parent.width - parent.padding * 2

                ColumnLayout {
                    width: rootfsGroupBox.width - rootfsGroupBox.padding * 2
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.maximumHeight: 150
                        clip: true

                        Column {
                            id: rootfsList

                            property string selectedItem
                            property string explicitEntry

                            spacing: 4

                            Component.onCompleted: {
                                var initState = (ref) => {
                                    selectedItem = ConfigModel.has("RootFS", ref) ? ConfigModel.getString("RootFS", ref) : ""

                                    // RootFSModel only lists entries in the $FEX_HOME/RootFS/ folder.
                                    // If a custom path is selected, add it as a dedicated entry
                                    if (selectedItem !== "" && !RootFSModel.hasItem(selectedItem)) {
                                        explicitEntry = selectedItem

                                        // Make visible once needed.
                                        // Conversely, if the user selects something else after, keep the old option visible to allow easy undoing
                                        fallbackRootfsEntry.visible = true
                                    }
                                }

                                initState(false)
                                root.refreshCacheChanged.connect(initState)
                            }

                            function updateRootFS(fileOrFolder: url) {
                                configDirty = true
                                var base = urlToLocalFile(RootFSModel.getBaseUrl())
                                var file = urlToLocalFile(fileOrFolder)
                                if (file.startsWith(base)) {
                                    file = file.substring(base.length)
                                }

                                ConfigModel.setString("RootFS", file)
                                refreshUI()
                            }

                            component RootFSRadioDelegate: RadioButton {
                                property var name

                                text: name
                                checked: rootfsList.selectedItem === name

                                onToggled: {
                                    configDirty = true;
                                    ConfigModel.setString("RootFS", name)
                                }
                            }

                            RootFSRadioDelegate {
                                id: fallbackRootfsEntry
                                visible: false
                                name: rootfsList.explicitEntry
                            }
                            Repeater {
                                model: RootFSModel
                                delegate: RootFSRadioDelegate { name: model.display }
                            }
                        }
                    }
                    RowLayout {
                        FileDialog {
                            id: addRootfsFileDialog
                            title: qsTr("Select RootFS file")
                            nameFilters: [ qsTr("SquashFS and EroFS (*.sqsh *.ero)"), qsTr("All files(*)") ]
                            currentFolder: RootFSModel.getBaseUrl()
                            onAccepted: rootfsList.updateRootFS(fileUrl)
                        }

                        FolderDialog {
                            id: addRootfsFolderDialog
                            title: qsTr("Select RootFS folder")
                            currentFolder: RootFSModel.getBaseUrl()
                            onAccepted: rootfsList.updateRootFS(selectedFolder)
                        }

                        Button {
                            text: qsTr("Add archive")
                            icon.name: "document-open"
                            onClicked: addRootfsFileDialog.open()
                        }
                        Button {
                            text: qsTr("Add folder")
                            icon.name: "folder"
                            onClicked: addRootfsFolderDialog.open()
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Logging:")
                width: parent.width - parent.padding * 2

                label: ConfigCheckBox {
                    id: loggingEnabledCheckBox
                    config: "SilentLog"
                    text: qsTr("Logging:")
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

        // Emulation settings
        ScrollablePage {
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
                title: qsTr("Memory model:")
                width: parent.width - parent.padding * 2

                ColumnLayout {
                    anchors.left: parent ? parent.left : undefined
                    anchors.right: parent ? parent.right : undefined

                    ButtonGroup {
                        id: tsoButtonGroup
                        buttons: [tso1, tso2]
                        checkedButton: ConfigModel.getBool("TSOEnabled", refreshCache) ? tso2 : tso1

                        property int pendingItemChange: -1

                        function onClickedButton(index: int) {
                            pendingItemChange = index;

                            configDirty = true;

                            var newIndex = pendingItemChange
                            var TSOEnabled = newIndex === 1
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
                                    text: qsTr("... for vector instructions")
                                    tooltip: qsTr("Controls TSO emulation on vector load/store instructions")
                                    config: "VectorTSOEnabled"
                                }
                                ConfigCheckBox {
                                    text: qsTr("... for memcpy instructions")
                                    tooltip: qsTr("Controls TSO emulation on memcpy/memset instructions")
                                    config: "MemcpySetTSOEnabled"
                                }
                                ConfigCheckBox {
                                    text: qsTr("... for unaligned half-barriers")
                                    tooltip: qsTr("Controls half-barrier TSO emulation on unaligned load/store instructions")
                                    config: "HalfBarrierTSOEnabled"
                                }
                            }
                        }
                    }

                    ConfigCheckBox {
                        topPadding: 4
                        text: qsTr("Enable non-tearing split-lock atomics")
                        config: "StrictInProcessSplitLocks"
                    }
                    ConfigCheckBox {
                        topPadding: 4
                        text: qsTr("Use PE volatile metadata for ARM64EC")
                        config: "VolatileMetadata"
                    }
                }
            }

            component EnvVarList: GroupBox {
                width: parent.width - parent.padding * 2

                property bool ofHost: false

                ColumnLayout {
                    anchors.left: parent ? parent.left : undefined
                    anchors.right: parent ? parent.right : undefined

                    spacing: 0

                    id: envGroup
                    property var values: ConfigModel.getStringList(ofHost ? "HostEnv" : "Env", refreshCache)

                    property int editedIndex: -1
                    Repeater {
                        model: parent.values
                        Layout.fillWidth: true

                        RowLayout {
                            property bool isEditing: envGroup.editedIndex === index

                            ItemDelegate {
                                text: modelData;
                                visible: !parent.isEditing
                                onClicked: envGroup.editedIndex = index

                            }
                            TextField {
                                id: envVarEditTextField
                                visible: parent.isEditing;
                                text: modelData

                                onEditingFinished: {
                                    envGroup.editedIndex = -1
                                    if (text === modelData) {
                                        return
                                    }

                                    var newValues = envGroup.values
                                    newValues[model.index] = text
                                    configDirty = true
                                    ConfigModel.setStringList(ofHost ? "HostEnv" : "Env", newValues)
                                }
                            }
                            Button {
                                visible: parent.isEditing
                                icon.name: "list-remove"
                                onClicked: {
                                    envGroup.editedIndex = -1
                                    var newValues = []
                                    for (var i = 0; i < envGroup.values.length; ++i) {
                                        if (i != index) {
                                            newValues.push(envGroup.values[i])
                                        }
                                    }

                                    configDirty = true
                                    ConfigModel.setStringList(ofHost ? "HostEnv" : "Env", newValues)
                                }
                            }
                        }
                    }

                    RowLayout {
                        TextField {
                            id: envVarTextField
                            Layout.fillWidth: true

                            onAccepted: {
                                var newValues = envGroup.values
                                newValues.push(envVarTextField.text)
                                configDirty = true
                                ConfigModel.setStringList(ofHost ? "HostEnv" : "Env", newValues)
                                text = ""
                            }
                        }
                        Button {
                            icon.name : "list-add"
                            enabled: envVarTextField.text !== ""
                            onClicked: envVarTextField.onAccepted()
                        }
                    }
                }
            }

            EnvVarList {
                title: qsTr("Guest environment variables:")
            }

            EnvVarList {
                title: qsTr("Host environment variables:")
                ofHost: true
            }
        }

        // CPU settings
        ScrollablePage {
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

            ConfigCheckBox {
                text: qsTr("Reduced x87 precision")
                config: "X87ReducedPrecision"
            }

            ConfigCheckBox {
                text: qsTr("Disable JIT optimization passes")
                config: "O0"
            }
        }

        // Libraries settings
        ScrollablePage {
            GroupBox {
                title: qsTr("Library forwarding:")
                width: parent.width - parent.padding * 2

                ColumnLayout {
                    anchors.left: parent ? parent.left : undefined
                    anchors.right: parent ? parent.right : undefined

                    id: libfwdConfig

                    property url configDir: (() => {
                        var configPath = urlToLocalFile(configFilename)
                        var slashIndex = configPath.lastIndexOf('/')
                        if (slashIndex === -1) {
                            return ""
                        }
                        return "file://" + configPath.substr(0, slashIndex)
                    })()

                    ConfigTextFieldForPath {
                        text: qsTr("Host library folder:")
                        config: "ThunkHostLibs"
                        dialog: FolderDialog {
                            title: qsTr("Select path for host libraries")
                            currentFolder: libfwdConfig.configDir
                        }
                    }
                    ConfigTextFieldForPath {
                        text: qsTr("Guest library folder:")
                        config: "ThunkGuestLibs"
                        dialog: FolderDialog {
                            title: qsTr("Select path for guest libraries")
                            currentFolder: libfwdConfig.configDir
                        }
                    }
                }
            }

            GroupBox {
                id: hostLibsGroupBox
                title: qsTr("Use host library for:")
                width: parent.width - parent.padding * 2

                ColumnLayout {
                    width: hostLibsGroupBox.width - hostLibsGroupBox.padding * 2
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.maximumHeight: 200
                        clip: true

                        Column {
                            property string selectedItem
                            property string explicitEntry

                            spacing: 4

                            Component.onCompleted: {
                                // root.refreshCacheChanged.connect(initState)
                            }

                            Repeater {
                                model: HostLibsModel
                                delegate: CheckBox {
                                    text: model.display
                                    visible: text !== "fex_thunk_test" // Hide test library
                                    checked: (root.refreshCache, model.checked)
                                    onToggled: {
                                        configDirty = true
                                        model.checked = checked
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Advanced settings
        // NOTE: This is wrapped in a Loader that dynamically instantiates/destroys the page contents whenever the tab is selected.
        //       This avoids costly UI updates for its UI elements.
        // TODO: Options contained multiple times in JSON aren't listed (neither are they in old FEXConfig though)
        Loader { sourceComponent: tabBar.currentIndex === tabBar.advancedIndex ? advancedSettingsPage : null }
        Component {
            id: advancedSettingsPage
            ScrollablePage {
                itemSpacing: 0
                Frame {
                    width: parent.width - parent.padding * 2
                    id: frame
                    Column {
                        Repeater {
                            model: ConfigModel
                            delegate: RowLayout {
                                width: frame.width - frame.padding * 2

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
                    }
                }
            }
        }
    }

    footer: Pane {
        anchors.left: parent.left
        anchors.right: parent.right

        padding: 0

        ColumnLayout {
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
                text: loadedDefaults
                        ? qsTr("Config.json not found — loaded defaults")
                        : qsTr("Editing %1").arg(urlToLocalFile(configFilename))
            }
        }
    }
}
