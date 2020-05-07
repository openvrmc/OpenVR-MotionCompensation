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
    content: ColumnLayout
    {
        spacing: 12

        //HMD
        RowLayout
        {
            spacing: 40

            MyText
            {
                text: "HMD:"
            }

            MyComboBox
            {
                id: hmdSelectionComboBox
                Layout.maximumWidth: 799
                Layout.minimumWidth: 799
                Layout.preferredWidth: 799
                Layout.fillWidth: true
                model: []
                onCurrentIndexChanged:
                {
					if (currentIndex >= 0)
                    {
						var openVRId = DeviceManipulationTabController.getTrackerDeviceID(currentIndex)
                        DeviceManipulationTabController.updateDeviceInfo(openVRId)
						fetchHMDInfo()
                    }
                }
            }
        }

        //Status device
        RowLayout
        {
            spacing: 18

            MyText
            {
                text: "Status:"
            }

            MyText
            {
                id: hmdStatusText
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
                    
                        var openVRId = DeviceManipulationTabController.getTrackerDeviceID(currentIndex)
                        DeviceManipulationTabController.updateDeviceInfo(openVRId)
                        fetchTrackerInfo()
                    }
                }
            }

            MyPushButton
            {
                id: referenceTrackerIdentifyButton
                enabled: hmdSelectionComboBox.currentIndex >= 0
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

            MyText
            {
                text: "Status:"
            }

            MyText
            {
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

        //Set Vel/Acc to zero
        RowLayout
        {
        spacing: 18
            MyText
            {
                text: "Set Vel + Acc to zero:"
            }

            Item
            {
                Layout.preferredWidth: 78
            }

            CheckBox
            {
                id: setZeroCheckBox
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
                keyBoardUID: 10
                Layout.preferredWidth: 140
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                horizontalAlignment: Text.AlignHCenter
                function onInputEvent(input)
                {
                    var val = parseFloat(input)
                    if (!isNaN(val))
                    {
                        if (!DeviceManipulationTabController.setLPFBeta(val.toFixed(4)))
                        {
                            deviceManipulationMessageDialog.showMessage("LPF Beta value", "Could not set new value:\n" + DeviceManipulationTabController.getDeviceModeErrorString())
                        }
                    }
                    text = DeviceManipulationTabController.getLPFBeta().toFixed(4)
                }
            }

            Item
            {
                Layout.preferredWidth: 10
            }

            MyPushButton
            {
                id: lpfBetaIncreaseButton
                Layout.preferredWidth: 45
                enabled: false
                text: "+"
                onClicked:
                {
                    DeviceManipulationTabController.increaseLPFBeta(0.01);
                }
            }

            Item
            {
                Layout.preferredWidth: 10
            }

            MyPushButton
            {
                id: lpfBetaDecreaseButton
                Layout.preferredWidth: 45
                enabled: false
                text: "-"
                onClicked:
                {
                    DeviceManipulationTabController.increaseLPFBeta(-0.01);
                }
            }

            MyText
            {
                Layout.leftMargin: 40
                text: "0 < value < 1"
            }
        }

        RowLayout
        {
            MyText
            {
                text: "EMA Samples:"
            }

            Item
            {
                Layout.preferredWidth: 192
            }

            MyTextField
            {
                id: samplesInputField
                text: "100"
                keyBoardUID: 20
                Layout.preferredWidth: 140
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                horizontalAlignment: Text.AlignHCenter
                function onInputEvent(input)
                {
                    var val = parseInt(input)
                    if (!isNaN(val))
                    {
                        if (!DeviceManipulationTabController.setSamples(val))
                        {
                            deviceManipulationMessageDialog.showMessage("Samples", "Could not set new value:\n" + DeviceManipulationTabController.getDeviceModeErrorString())
                        }
                    }
                    text = DeviceManipulationTabController.getSamples()
                }
            }

            Item
            {
                Layout.preferredWidth: 10
            }

            MyPushButton
            {
                id: samplesIncreaseButton
                Layout.preferredWidth: 45
                enabled: false
                text: "+"
                onClicked:
                {
                    DeviceManipulationTabController.increaseSamples(5);
                }
            }

            Item
            {
                Layout.preferredWidth: 10
            }

            MyPushButton
            {
                id: samplesDecreaseButton
                Layout.preferredWidth: 45
                enabled: false
                text: "-"
                onClicked:
                {
                    DeviceManipulationTabController.increaseSamples(-5);
                }
            }

            MyText
            {
                Layout.leftMargin: 40
                text: "2 < samples"
            }
        }

        //Apply button
        RowLayout
        {
            MyPushButton
            {
                id: deviceModeApplyButton
                Layout.preferredWidth: 200
                Layout.topMargin: 20
                Layout.bottomMargin: 35
                enabled: false
                text: "Apply"
                onClicked:
                {
                    if (!DeviceManipulationTabController.setMotionCompensationMode(hmdSelectionComboBox.currentIndex, referenceTrackerSelectionComboBox.currentIndex, enableMotionCompensationCheckBox.checked, setZeroCheckBox.checked))
                    {
                        deviceManipulationMessageDialog.showMessage("Set Device Mode", "Could not set device mode:\n" + DeviceManipulationTabController.getDeviceModeErrorString())
                    }
                    if (!DeviceManipulationTabController.sendMCSettings())
                    {
                        deviceManipulationMessageDialog.showMessage("Set Device Mode", "Could not send Settings:\n" + DeviceManipulationTabController.getDeviceModeErrorString())
                    }
                    if (!DeviceManipulationTabController.setDebugMode(true))
                    {
                        deviceManipulationMessageDialog.showMessage("Debug logger", "Could not start or stop logging:\n" + DeviceManipulationTabController.getDeviceModeErrorString())
                    }
                }
            }
        }

        //Debug Mode
        /*RowLayout
        {
            spacing: 18

            MyText
            {
                text: "Debug Logger"
            }
        }

        //Enable debug logger:
        RowLayout
        {
        spacing: 16

        MyPushButton
            {
                id: debugLoggerButton
                Layout.preferredWidth: 250
                text: "Start logging"
                enabled: false          // Set to "true" (without the " " ) to enable debug logger
                onClicked:
                {
                    if (!DeviceManipulationTabController.setDebugMode(false))
                    {
                        deviceManipulationMessageDialog.showMessage("Debug logger", "Could not start or stop logging:\n" + DeviceManipulationTabController.getDeviceModeErrorString())
                    }
                }
            }
        }*/
        Item
        {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        //Version number
        RowLayout
        {
            spacing: 18
            Item
            {
                Layout.fillWidth: true
            }
            MyText
            {
                id: appVersionText
                text: "v0.0.0"
            }
        }


        Component.onCompleted:
        {
            appVersionText.text = OverlayController.getVersionString()
            lpfBetaInputField.text = DeviceManipulationTabController.getLPFBeta().toFixed(4)
            samplesInputField.text = DeviceManipulationTabController.getSamples()
        }

        Connections
        {
            target: DeviceManipulationTabController
            onDeviceCountChanged:
            {
                fetchDevices()
                fetchHMDInfo()
            }
            onDeviceInfoChanged:
            {
                if (index == hmdSelectionComboBox.currentIndex)
                {
                    fetchHMDInfo()
                }
            }
            onSettingChanged:
            {
                lpfBetaInputField.text = DeviceManipulationTabController.getLPFBeta().toFixed(4)
                samplesInputField.text = DeviceManipulationTabController.getSamples()
            }
            onDebugModeChanged:
            {
                debugLoggerButton.text = DeviceManipulationTabController.getDebugModeButtonText()
            }
        }

    }

    function fetchDevices()
    {
        var hmds = []
        var tracker = []
        var oldHMDIndex = hmdSelectionComboBox.currentIndex
        var oldtrackerIndex = referenceTrackerSelectionComboBox.currentIndex
        var deviceCount = DeviceManipulationTabController.getDeviceCount()
        var hmdCount = 0;
        var trackerCount = 0;

        //Collect all found devices
        for (var i = 0; i < deviceCount; i++)
        {
            var openVRId = DeviceManipulationTabController.getOpenVRId(i)
            var deviceName = openVRId.toString() + ": "
            deviceName += DeviceManipulationTabController.getDeviceSerial(i)
            var deviceClass = DeviceManipulationTabController.getDeviceClass(i)

            if (deviceClass == 1)
            {
                deviceName += " (HMD)"                

                hmds.push(deviceName)
                DeviceManipulationTabController.setHMDArrayID(i, hmds.length - 1)
                ++hmdCount
            }
            else if (deviceClass == 2)
            {
                deviceName += " (Controller)"
            }
            else if (deviceClass == 3)
            {
                deviceName += " (Tracker)"
            }

            if (deviceClass == 2 || deviceClass == 3)
            {
                tracker.push(deviceName)
                DeviceManipulationTabController.setTrackerArrayID(i, tracker.length - 1)
                ++trackerCount
            }
        }

        hmdSelectionComboBox.model = hmds
        referenceTrackerSelectionComboBox.model = tracker

        if (hmdCount < 1 || trackerCount < 1)
        {   
            //Empty comboboxes
            if (hmdCount < 1)
            {
                hmdSelectionComboBox.currentIndex = -1

                //Uncheck check box
                enableMotionCompensationCheckBox.checkState = Qt.Unchecked
            }
            else
            {
                hmdSelectionComboBox.currentIndex = 0
            }
            
            if (trackerCount < 1)
            {
                referenceTrackerSelectionComboBox.currentIndex = -1
            }
            else
            {
                referenceTrackerSelectionComboBox.currentIndex = 0
            }

            //Disable buttons
            deviceModeApplyButton.enabled = false
            referenceTrackerIdentifyButton.enabled = false
            lpfBetaIncreaseButton.enabled = false
            lpfBetaDecreaseButton.enabled = false
            samplesIncreaseButton.enabled = false
            samplesDecreaseButton.enabled = false
        }
        else
        {
            //Enable buttons
            deviceModeApplyButton.enabled = true
            referenceTrackerIdentifyButton.enabled = true
            lpfBetaIncreaseButton.enabled = true
            lpfBetaDecreaseButton.enabled = true
            samplesIncreaseButton.enabled = true
            samplesDecreaseButton.enabled = true

            //Select a valid index
            if (oldHMDIndex >= 0 && oldHMDIndex < hmdCount)
            {
                hmdSelectionComboBox.currentIndex = oldHMDIndex
            }
            else
            {
                hmdSelectionComboBox.currentIndex = 0
            }

            if (oldtrackerIndex >= 0 && oldtrackerIndex < trackerCount)
            {
                referenceTrackerSelectionComboBox.currentIndex = oldHMDIndex
            }
            else
            {
                referenceTrackerSelectionComboBox.currentIndex = 0
            }
        }
    }

    function fetchHMDInfo()
    {
        var index = hmdSelectionComboBox.currentIndex

        if (index >= 0)
        {
            var deviceId = DeviceManipulationTabController.getHMDDeviceID(index)
            var statusText = "Error getting status"

            if (deviceId >= 0)
            {   
                var deviceMode = DeviceManipulationTabController.getDeviceMode(deviceId)
                var deviceState = DeviceManipulationTabController.getDeviceState(deviceId)
                var deviceClass = DeviceManipulationTabController.getDeviceClass(deviceId)

                if (deviceClass != 1)       // Not a HMD
                {
                    statusText = "Warning! Selection is not a HMD";
                }
                else
                {            
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
                    else if (deviceMode == 2)       // motion compensated
                    {
                        enableMotionCompensationCheckBox.checked = true

                        if (deviceState == 0)
                        {
                            statusText = "Motion Compensated"
                        }
                        else if (deviceState == 1)
                        {
                            statusText = "Motion Compensated (Disconnected)"
                        }
                        else
                        {
                            statusText = "Motion Compensated (Unknown state " + deviceState.toString() + ")"
                        }
                    }
                    else
                    {
                        statusText = "Unknown or invalid Mode " + deviceMode.toString()
                    }
                }
            }
            hmdStatusText.text = statusText
        }
    }

    function fetchTrackerInfo()
    {
        var index = hmdSelectionComboBox.currentIndex

        if (index >= 0)
        {
            var deviceId = DeviceManipulationTabController.getTrackerDeviceID(index)
            var statusText = "Error getting status"

            if (deviceId >= 0)
            {
                var deviceMode = DeviceManipulationTabController.getDeviceMode(deviceId)
                var deviceState = DeviceManipulationTabController.getDeviceState(deviceId)
                var deviceClass = DeviceManipulationTabController.getDeviceClass(deviceId)

                if (deviceClass != 2 && deviceClass != 3)       // Not a tracked controller or generic tracker
                {
                    statusText = "Warning! Selection is not a tracker. ClassID: " + deviceClass.toString();
                }
                else
                {
                    if (deviceMode == 0)        // default
                    {
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
                    else if (deviceMode == 1)       // reference tracker
                    {
                        if (deviceState == 0)
                        {
                            statusText = "Reference Tracker"
                        }
                        else if (deviceState == 1)
                        {
                            statusText = "Reference Tracker (Disconnected)"
                        }
                        else
                        {
                            statusText = "Reference Tracker (Unknown state " + deviceState.toString() + ")"
                        }
                    }
                    else
                    {
                        statusText = "Unknown or invalid Mode " + deviceMode.toString()
                    }
                }
            }
            referenceTrackerStatusText.text = statusText
        }
    }
}
