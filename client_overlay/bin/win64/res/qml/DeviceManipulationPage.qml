import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.2
import ovrmc.motioncompensation 1.0
import "." // QTBUG-34418, singletons require explicit import to load qmldir file


MyStackViewPage
{
    id: devicePage
    width: 1200
    height: 800
    headerText: "OpenVR Motion Compensation"
    headerShowBackButton: false

    property int deviceIndex: 0

    //Generic popup
    MyDialogOkPopup
    {
        id: deviceManipulationMessageDialog
        function showMessage(title, text)
        {
            dialogTitle = title
            dialogText = text
            open()
        }
    }

    //Profile delete
    MyDialogOkCancelPopup
    {
        id: deviceManipulationDeleteProfileDialog
        property int profileIndex: -1
        dialogTitle: "Delete Profile"
        dialogText: "Do you really want to delete this profile?"
        onClosed:
        {
            if (okClicked)
            {
                DeviceManipulationTabController.deleteDeviceManipulationProfile(profileIndex)
            }
        }
    }

    //Profile popup
    MyDialogOkCancelPopup
    {
        id: deviceManipulationNewProfileDialog
        dialogTitle: "Create New Profile"
        dialogWidth: 600
        dialogHeight: 400
        dialogContentItem: ColumnLayout {
            RowLayout {
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                MyText {
                    text: "Name: "
                }
                MyTextField {
                    id: deviceManipulationNewProfileName
                    color: "#cccccc"
                    text: ""
                    Layout.fillWidth: true
                    font.pointSize: 20
                    function onInputEvent(input) {
                        text = input
                    }
                }
            }
            ColumnLayout {
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
            }
        }
        onClosed: {
            if (okClicked)
            {
                if (deviceManipulationNewProfileName.text == "")
                {
                    deviceManipulationMessageDialog.showMessage("Create New Profile", "ERROR: Empty profile name.")
                }
                else
                {
                    DeviceManipulationTabController.addDeviceManipulationProfile(deviceManipulationNewProfileName.text, deviceIndex, false, false)
                }
            }
        }
        function openPopup(device)
        {
            deviceManipulationNewProfileName.text = ""
            deviceIndex = device
            open()
        }
    }

    content: ColumnLayout
    {
        spacing: 18

        //Device
        RowLayout
        {
            spacing: 18

            MyText
            {
                text: "Device:"
            }

            MyComboBox
            {
                id: deviceSelectionComboBox
                Layout.maximumWidth: 799
                Layout.minimumWidth: 799
                Layout.preferredWidth: 799
                Layout.fillWidth: true
                model: []
                onCurrentIndexChanged:
                {
					if (currentIndex >= 0)
                    {
						DeviceManipulationTabController.updateDeviceInfo(currentIndex);
						fetchDeviceInfo()
                    }
                }
            }

            //How do identify a HMD?
            /*MyPushButton
            {
                id: deviceIdentifyButton
                enabled: deviceSelectionComboBox.currentIndex >= 0
                Layout.preferredWidth: 194
                text: "Identify"
                onClicked:
                {
                    if (deviceSelectionComboBox.currentIndex >= 0)
                    {
                    }
                }
            }*/
        }

        //Status device
        RowLayout
        {
            spacing: 18

            MyText {
                text: "Status:"
            }

            MyText {
                id: deviceStatusText
                text: ""
            }
        }

        //Reference tracker
        RowLayout
        {
            spacing: 18

            MyText
            {
                text: "Reference Tracker:"
            }

            MyComboBox
            {
                id: referenceTrackerSelectionComboBox
                Layout.maximumWidth: 660
                Layout.minimumWidth: 660
                Layout.preferredWidth: 660
                Layout.fillWidth: true
                model: []
                onCurrentIndexChanged:
                {
                    if (currentIndex >= 0)
                    {
                        //DeviceManipulationTabController.updateDeviceInfo(currentIndex);
                        //fetchDeviceInfo()
                    }
                }
            }

            MyPushButton
            {
                id: referenceTrackerIdentifyButton
                enabled: deviceSelectionComboBox.currentIndex >= 0
                Layout.preferredWidth: 194
                text: "Identify"
                onClicked:
                {
                    if (referenceTrackerSelectionComboBox.currentIndex >= 0)
                    {
                    }
                }
            }
        }

        //Status reference tracker
        RowLayout
        {
            spacing: 18
            Layout.bottomMargin: 16

            MyText {
                text: "Status:"
            }

            MyText {
                id: referenceTrackerStatusText
                text: ""
            }
        }

        //Enable Motion Compensation checkbox
        RowLayout
        {
        spacing: 18
            MyText
            {
                text: "Enable Motion Compensation:"
            }

            CheckBox
            {
                id: enableMotionCompensationCheckBox
            }
        }

        //Reference Tracker Invisible checkbox
        RowLayout
        {
        spacing: 18
            MyText
            {
                text: "Reference Tracker Invisible:"
            }

            Item {
                Layout.preferredWidth: 6
            }

            CheckBox
            {
                id: referenceTrackerInvisibleCheckBox
            }
        }

        //LPF Beta Value
        RowLayout
        {
            MyText
            {
                text: "LPF Beta value:"
            }

            Item
            {
                Layout.preferredWidth: 178
            }

            MyTextField
            {
                id: lpfBetaInputField
                text: "0.0000"
                Layout.preferredWidth: 140
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                horizontalAlignment: Text.AlignHCenter
                function onInputEvent(input)
                {
                    var val = parseFloat(input)
                    if (!isNaN(val) && val >= 0.0)
                    {
                        if (!DeviceManipulationTabController.setLPFBeta(val.toFixed(4)))
                        {
                            deviceManipulationMessageDialog.showMessage("LPF Beta value", "Could not set new value: " + DeviceManipulationTabController.getDeviceModeErrorString())
                        }
                    }
                    else
                    {
                        lpfBetaInputField.text = DeviceManipulationTabController.setLPFBeta().toFixed(4)
                    }
                }
            }

            MyText
            {
                Layout.leftMargin: 40
                text: "0 < value < 1"
            }
        }

        //Apply button
        RowLayout
        {
            MyPushButton
            {
                id: deviceModeApplyButton
                Layout.preferredWidth: 200
                enabled: false
                text: "Apply"
                onClicked:
                {
                    if (deviceSelectionComboBox.currentIndex != referenceTrackerSelectionComboBox.currentIndex)
                    {
                        if (!DeviceManipulationTabController.setMotionCompensationMode(deviceSelectionComboBox.currentIndex, referenceTrackerSelectionComboBox.currentIndex, enableMotionCompensationCheckBox.checked))
                        {
                            deviceManipulationMessageDialog.showMessage("Set Device Mode", "Could not set device mode: " + DeviceManipulationTabController.getDeviceModeErrorString())
                        }
                    }
                    else
                    {
                        deviceManipulationMessageDialog.showMessage("Set Device Mode", "\"Device\" and \"Reference Tracker\" cannot be the same!")
                    }                    
                }
            }
        }

       /* RowLayout {
            spacing: 18

        }*/

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        //Profile
        ColumnLayout {
            Layout.bottomMargin: 6
            spacing: 18
            RowLayout {
                spacing: 18

                MyText {
                    text: "Profile:"
                }

                MyComboBox
                {
                    id: deviceManipulationProfileComboBox
                    Layout.maximumWidth: 799
                    Layout.minimumWidth: 799
                    Layout.preferredWidth: 799
                    Layout.fillWidth: true
                    model: [""]
                    onCurrentIndexChanged:
                    {
                        if (currentIndex > 0)
                        {
                            deviceManipulationApplyProfileButton.enabled = true
                            deviceManipulationDeleteProfileButton.enabled = true
                        }
                        else
                        {
                            deviceManipulationApplyProfileButton.enabled = false
                            deviceManipulationDeleteProfileButton.enabled = false
                        }
                    }
                }

                MyPushButton
                {
                    id: deviceManipulationApplyProfileButton
                    enabled: false
                    Layout.preferredWidth: 200
                    text: "Apply"
                    onClicked:
                    {
                        if (deviceManipulationProfileComboBox.currentIndex > 0 && deviceSelectionComboBox.currentIndex >= 0)
                        {
                            DeviceManipulationTabController.applyDeviceManipulationProfile(deviceManipulationProfileComboBox.currentIndex - 1, deviceSelectionComboBox.currentIndex);
                            deviceManipulationProfileComboBox.currentIndex = 0
                        }
                    }
                }
            }
            RowLayout {
                spacing: 18
                Item {
                    Layout.fillWidth: true
                }
                MyPushButton {
                    id: deviceManipulationDeleteProfileButton
                    enabled: false
                    Layout.preferredWidth: 200
                    text: "Delete Profile"
                    onClicked: {
                        if (deviceManipulationProfileComboBox.currentIndex > 0) {
                            deviceManipulationDeleteProfileDialog.profileIndex = deviceManipulationProfileComboBox.currentIndex - 1
                            deviceManipulationDeleteProfileDialog.open()
                        }
                    }
                }
                MyPushButton {
                    id: deviceManipulationNewProfileButton
                    Layout.preferredWidth: 200
                    text: "New Profile"
                    onClicked: {
                        if (deviceSelectionComboBox.currentIndex >= 0) {
                            deviceManipulationNewProfileDialog.openPopup(deviceSelectionComboBox.currentIndex)
                        }
                    }
                }
            }
        }

        //Version number
        RowLayout {
            spacing: 18
            Item {
                Layout.fillWidth: true
            }
            MyText {
                id: appVersionText
                text: "v0.0"
            }
        }


        Component.onCompleted: {
            appVersionText.text = OverlayController.getVersionString()
            reloadDeviceManipulationProfiles()
        }

        Connections {
            target: DeviceManipulationTabController
            onDeviceCountChanged: {
                fetchDevices()
                fetchDeviceInfo()
            }
            onDeviceInfoChanged: {
                if (index == deviceSelectionComboBox.currentIndex) {
                    fetchDeviceInfo()
                }
            }
            onDeviceManipulationProfilesChanged: {
                reloadDeviceManipulationProfiles()
            }
        }

    }

    function fetchDevices()
    {
        var devices = []
        var oldIndex = deviceSelectionComboBox.currentIndex
        var deviceCount = DeviceManipulationTabController.getDeviceCount()

        for (var i = 0; i < deviceCount; i++)
        {
            var deviceId = DeviceManipulationTabController.getDeviceId(i)
            var deviceName = deviceId.toString() + ": "
            deviceName += DeviceManipulationTabController.getDeviceSerial(i)
            var deviceClass = DeviceManipulationTabController.getDeviceClass(i)

            if (deviceClass == 1)
            {
                deviceName += " (HMD)"
            }
            else if (deviceClass == 2)
            {
                deviceName += " (Controller)"
            }
            else if (deviceClass == 3)
            {
                deviceName += " (Tracker)"
            }
            else if (deviceClass == 4)
            {
                deviceName += " (Base-Station)"
            }
            else
            {
                deviceName += " (Unknown " + deviceClass.toString() + ")"
            }

            devices.push(deviceName)
        }

        deviceSelectionComboBox.model = devices
        referenceTrackerSelectionComboBox.model = devices

        if (deviceCount <= 0)
        {
            deviceSelectionComboBox.currentIndex = -1
            enableMotionCompensationCheckBox.checkState = Qt.Unchecked
            deviceModeApplyButton.enabled = false
            referenceTrackerIdentifyButton.enabled = false
            deviceManipulationNewProfileButton.enabled = false
            deviceManipulationProfileComboBox.enabled = false
        }
        else
        {
            enableMotionCompensationCheckBox.checkState = Qt.Checked
            deviceModeApplyButton.enabled = true
            referenceTrackerIdentifyButton.enabled = true
            deviceManipulationNewProfileButton.enabled = true
            deviceManipulationProfileComboBox.enabled = true

            if (oldIndex >= 0 && oldIndex < deviceCount)
            {
                deviceSelectionComboBox.currentIndex = oldIndex
            }
            else
            {
                deviceSelectionComboBox.currentIndex = 0
            }
        }
    }

    function fetchDeviceInfo()
    {
        var index = deviceSelectionComboBox.currentIndex

        if (index >= 0)
        {
            var deviceMode = DeviceManipulationTabController.getDeviceMode(index)
            var deviceState = DeviceManipulationTabController.getDeviceState(index)
            var deviceClass = DeviceManipulationTabController.getDeviceClass(index)
            var statusText = ""

            if (deviceMode == 0)        // default
            {
                enableMotionCompensationCheckBox.checked = false

                if (deviceState == 0)
                {
                    statusText = "Default"
                }
                else if (deviceState == 1)
                {
                    statusText = "Default (Disconnected)"
                }
                else
                {
                    statusText = "Default (Unknown state " + deviceState.toString() + ")"
                }
            }
            else if (deviceMode == 1)       // motion compensated
            {
                enableMotionCompensationCheckBox.checked = true

                if (deviceState == 0)
                {
                    statusText = "Motion Compensated"
                }
                else
                {
                    statusText = "Motion Compensated (Unknown state " + deviceState.toString() + ")"
                }
            }
            else if (deviceMode == 2)       //reference tracker
            {
                enableMotionCompensationCheckBox.checked = false
            }
            else
            {
                statusText = "Unknown Mode " + deviceMode.toString()
            }

            deviceStatusText.text = statusText
        }
    }

    function reloadDeviceManipulationProfiles()
    {
        var profiles = [""]
        var profileCount = DeviceManipulationTabController.getDeviceManipulationProfileCount()

        for (var i = 0; i < profileCount; i++)
        {
            profiles.push(DeviceManipulationTabController.getDeviceManipulationProfileName(i))
        }

        deviceManipulationProfileComboBox.model = profiles
        deviceManipulationProfileComboBox.currentIndex = 0
    }
}
