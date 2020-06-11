import QtQuick 2.9
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.2
import ovrmc.motioncompensation 1.0

MyStackViewPage
{
    id: settingsPage
	width: 1200
	height: 800
    headerText: "Settings"

    content: ColumnLayout
    {
		MyNewKeyBinding
		{
			id: keybinding
			width: 400
			height: 200

			focus: true

			function showPopup(id)
			{
				loadValues(id)
				open()
			}

			updateShortcut: function()
			{
				refreshButtonText()
			}
		}

        spacing: 18

        GridLayout
        {
            columns: 2

            MyText
            {
                Layout.preferredWidth: 360
                Layout.leftMargin: 0
                Layout.rightMargin: 0
                horizontalAlignment: Text.AlignLeft
                text: "Motion Compensation Mode:"
            }

            MyComboBox
            {
                id: mcModeComboBox
                Layout.maximumWidth: 518
                Layout.minimumWidth: 518
                Layout.preferredWidth: 518
                Layout.fillWidth: true
                model: [
                    "Reference Tracker",
                    "FlyPT Mover with Offset"
                ]
                onCurrentIndexChanged:
                {
                    if (currentIndex >= 0)
                    {
                        DeviceManipulationTabController.setMotionCompensationMode(currentIndex)
                    }
                }
            }
        }

        // LPF Beta Value
        GridLayout
        {
            columns: 5

            MyText
            {
                Layout.preferredWidth: 360
                Layout.leftMargin: 0
                Layout.rightMargin: 0
                horizontalAlignment: Text.AlignLeft
                text: "LPF Beta value:"
            }

            MyPushButton2
            {
                id: lpfBetaIncreaseButton
                Layout.leftMargin: 0
                Layout.preferredWidth: 45
                text: "-"
                onClicked:
                {
                    DeviceManipulationTabController.increaseLPFBeta(-0.01);
                }
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

            MyPushButton2
            {
                id: lpfBetaDecreaseButton
                Layout.preferredWidth: 45
                text: "+"
                onClicked:
                {
                    DeviceManipulationTabController.increaseLPFBeta(0.01);
                }
            }

            MyText
            {
                Layout.leftMargin: 110
                text: "0 < value < 1"
            }
        }

        // DEMA Samples
        GridLayout
        {
            columns: 7

            MyText
            {
                Layout.preferredWidth: 360
                Layout.leftMargin: 0
                Layout.rightMargin: 0
                horizontalAlignment: Text.AlignLeft
                text: "DEMA samples:"
            }

            MyPushButton2
            {
                id: samplesIncreaseButton
                Layout.leftMargin: 0
                Layout.preferredWidth: 45
                text: "-"
                onClicked:
                {
                    DeviceManipulationTabController.increaseSamples(-5);
                }
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

            MyPushButton2
            {
                id: samplesDecreaseButton
                Layout.preferredWidth: 45
                text: "+"
                onClicked:
                {
                    DeviceManipulationTabController.increaseSamples(5);
                }
            }

            MyText
            {
                Layout.leftMargin: 110
                text: "2 < samples"
            }
        }

        // Offset settings
        MyOffsetGroupBox
        {
            boxTitle: "HMD to Reference Offset"
            id: hmdtoReferenceOffsetBox
            setTranslationOffset: function(x, y, z)
            {
                DeviceManipulationTabController.setHMDtoRefOffset(x, y, z)
            }
            updateValues: function()
            {
                var hasChanged = false

				value = DeviceManipulationTabController.getHMDtoRefOffset(0)
				if (offsetX != value)
                {
                    offsetX = value
                    hasChanged = true
                }

                value = DeviceManipulationTabController.getHMDtoRefOffset(1)
                if (offsetY != value)
                {
                    offsetY = value
                    hasChanged = true
                }

                value = DeviceManipulationTabController.getHMDtoRefOffset(2)
                if (offsetZ != value)
                {
                    offsetZ = value
                    hasChanged = true
                }

                if (hasChanged)
                {
                    updateGUI()
                }
            }
        }

        RowLayout
        {
            Rectangle
            {
                Layout.topMargin: 25
                color: "#ffffff"
                height: 1
                Layout.fillWidth: true
            }
        }

        // Set Vel + Acc to zero
        RowLayout
        {
        spacing: 18
            MyText
            {
                text: "Set Velocity and Acceleration to zero:"
            }

            Item
            {
                Layout.preferredWidth: 80
            }

            CheckBox
            {
                id: setZeroCheckBox
                onCheckedChanged:
                {
                    DeviceManipulationTabController.setZeroMode(setZeroCheckBox.checked)
                }
            }
        }

        RowLayout
        {
            MyText
            {
                Layout.topMargin: 20
                text: "Keyboard Hotkeys"
            }
        }

        GridLayout
        {
			columns: 3

            MyText
            {
                Layout.preferredWidth: 360
                Layout.leftMargin: 0
                Layout.rightMargin: 0
                horizontalAlignment: Text.AlignLeft
                text: "Enable / Disable MC:"
            }

			MyPushButton
			{
				id: btn_enableMC
				Layout.preferredWidth: 200
				Layout.topMargin: 0
				Layout.bottomMargin: 0
				text: ""
				onClicked:
				{
					keybinding.showPopup(0);
				}
			}

			MyPushButtonIcon
			{
				id: btn_enableMC_Remove
				Layout.preferredWidth: 45
				Layout.preferredHeight: 45
				Layout.leftMargin: 20
				Layout.topMargin: 0
				Layout.bottomMargin: 0
				imagesource : "octicons-trashcan.png"
				onClicked:
				{
					settings.removeKey(0);
					refreshButtonText();
				}
			}
        }

        GridLayout
        {
			columns: 3

            MyText
            {
                Layout.preferredWidth: 360
                Layout.leftMargin: 0
                Layout.rightMargin: 0
                horizontalAlignment: Text.AlignLeft
                text: "Reset Reference Pose:"
            }

			MyPushButton
			{
				id: btn_setZeroPose
				Layout.preferredWidth: 200
				Layout.topMargin: 0
				Layout.bottomMargin: 0
				text: ""
				onClicked:
				{
					keybinding.showPopup(1);
				}
			}

			MyPushButtonIcon
			{
				id: btn_setZeroPose_Remove
				Layout.preferredWidth: 45
				Layout.preferredHeight: 45
				Layout.leftMargin: 20
				Layout.topMargin: 0
				Layout.bottomMargin: 0
				imagesource : "octicons-trashcan.png"
				onClicked:
				{
					settings.removeKey(1);
					refreshButtonText();
				}
			}
        }

        Item
        {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        function updateSettings()
        {
            if (deviceIndex >= 0)
            {
                hmdtoReferenceOffsetBox.updateValues()
                setZeroCheckBox.checked = DeviceManipulationTabController.getZeroMode()
            }
        }

		function refreshButtonText()
		{
			btn_enableMC.text = settings.getModifiers_AsString(0) + settings.getKey_AsString(0);
			btn_setZeroPose.text = settings.getModifiers_AsString(1) + settings.getKey_AsString(1);
		}

		Component.onCompleted:
        {
            lpfBetaInputField.text = DeviceManipulationTabController.getLPFBeta().toFixed(4)
            samplesInputField.text = DeviceManipulationTabController.getSamples()
			refreshButtonText()
        }

        Connections
        {
            target: DeviceManipulationTabController			
            onSettingChanged:
            {
                lpfBetaInputField.text = DeviceManipulationTabController.getLPFBeta().toFixed(4)
                samplesInputField.text = DeviceManipulationTabController.getSamples()
            }
        }
    }
}
