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

	property double offsetRotationStep: 0.1
	property double offsetTranslationStep : 0.01
	property double autoRepeat_Delay: 300
	property double autoRepeat_Interval: 25

	// Generic popup
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

    Grid {
        id: grid
        x: 720
        y: 640
        width: 440
        height: 125
        layoutDirection: Qt.RightToLeft
        flow: Grid.LeftToRight
        padding: 0
        spacing: 20
        rows: 2
        columns: 2

        MyPushButton {
            id: deviceModeOffsetsApplyButton
            x: 960
            y: 640
            width: 200
            height: 45
            text: "Apply Offsets"
            Layout.bottomMargin: 35
            Layout.preferredWidth: 200
            Layout.topMargin: 20
            enabled: true
			onClicked:
                {
                    DeviceManipulationTabController.applyOffsets();
                }
        }
    }
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
                    "Reference Tracker"
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

		// Start of section "Offsets"
		RowLayout
		{
			Rectangle
			{
				Layout.topMargin: 0
				height: 1
				border.color: "#ffffff"
				Layout.fillWidth: true
			}
		}

		RowLayout
		{
			MyText
			{
				text: "Offsets for virtual Driver"
				horizontalAlignment: Text.AlignLeft
				//Layout.preferredWidth: 80
				Layout.leftMargin: 12
			}
		}

		// X, Y and Z offsets
		RowLayout
		{
			GridLayout
			{
				columns: 12

				MyText
				{
					text: "X:"
					horizontalAlignment: Text.AlignRight
					Layout.preferredWidth: 70
					Layout.rightMargin: 12
				}

				MyPushButton2
				{
					id: xMinusButton
					Layout.preferredWidth: 40
					text: "-"					
					autoRepeat: true
					autoRepeatDelay: autoRepeat_Delay
					autoRepeatInterval: autoRepeat_Interval
					onClicked:
					{
						DeviceManipulationTabController.increaseRefTranslationOffset(0, -offsetTranslationStep);
					}
				}

				MyTextField
				{
					id: xInputField
					text: "0.000"
					keyBoardUID: 30
					Layout.preferredWidth: 140
					Layout.leftMargin: 10
					Layout.rightMargin: 10
					horizontalAlignment: Text.AlignHCenter
					function onInputEvent(input)
					{
						var val = parseFloat(input)
						if (!isNaN(val))
						{
							DeviceManipulationTabController.setHMDtoRefTranslationOffset(0, val.toFixed(3))
						}

						text = DeviceManipulationTabController.getHMDtoRefTranslationOffset(0).toFixed(3)
					}
				}

				MyPushButton2
				{
					id: xPlusButton
					Layout.preferredWidth: 40
					text: "+"
					autoRepeat: true
					autoRepeatDelay: autoRepeat_Delay
					autoRepeatInterval: autoRepeat_Interval
					onClicked:
					{
						DeviceManipulationTabController.increaseRefTranslationOffset(0, offsetTranslationStep);
					}
				}

				MyText
				{
					text: "Y:"
					Layout.fillWidth: true
					horizontalAlignment: Text.AlignRight
					Layout.preferredWidth: 40
					Layout.leftMargin: 12
					Layout.rightMargin: 12
				}

				MyPushButton2
				{
					id: yMinusButton
					Layout.preferredWidth: 40
					text: "-"
					autoRepeat: true
					autoRepeatDelay: autoRepeat_Delay
					autoRepeatInterval: autoRepeat_Interval
					onClicked:
					{
						DeviceManipulationTabController.increaseRefTranslationOffset(1, -offsetTranslationStep);
					}
				}

				MyTextField
				{
					id: yInputField
					text: "0.000"
					keyBoardUID: 31
					Layout.preferredWidth: 140
					Layout.leftMargin: 10
					Layout.rightMargin: 10
					horizontalAlignment: Text.AlignHCenter
					function onInputEvent(input)
					{
						var val = parseFloat(input)
						if (!isNaN(val))
						{
							DeviceManipulationTabController.setHMDtoRefTranslationOffset(1, val.toFixed(3))
						}

						text = DeviceManipulationTabController.getHMDtoRefTranslationOffset(1).toFixed(3)
					}
				}

				MyPushButton2
				{
					id: yPlusButton
					Layout.preferredWidth: 40
					text: "+"
					autoRepeat: true
					autoRepeatDelay: autoRepeat_Delay
					autoRepeatInterval: autoRepeat_Interval
					onClicked:
					{
						DeviceManipulationTabController.increaseRefTranslationOffset(1, offsetTranslationStep);
					}
				}

				MyText
				{
					text: "Z:"
					Layout.fillWidth: true
					horizontalAlignment: Text.AlignRight
					Layout.preferredWidth: 40
					Layout.leftMargin: 12
					Layout.rightMargin: 12
				}

				MyPushButton2
				{
					id: zMinusButton
					Layout.preferredWidth: 40
					text: "-"
					autoRepeat: true
					autoRepeatDelay: autoRepeat_Delay
					autoRepeatInterval: autoRepeat_Interval
					onClicked:
					{
						DeviceManipulationTabController.increaseRefTranslationOffset(2, -offsetTranslationStep);
					}
				}

				MyTextField
				{
					id: zInputField
					text: "0.000"
					keyBoardUID: 32
					Layout.preferredWidth: 140
					Layout.leftMargin: 10
					Layout.rightMargin: 10
					horizontalAlignment: Text.AlignHCenter
					function onInputEvent(input)
					{
						var val = parseFloat(input)
						if (!isNaN(val))
						{
							DeviceManipulationTabController.setHMDtoRefTranslationOffset(2, val.toFixed(3))
						}

						text = DeviceManipulationTabController.getHMDtoRefTranslationOffset(2).toFixed(3)
					}
				}

				MyPushButton2
				{
					id: zPlusButton
					Layout.preferredWidth: 40
					text: "+"
					autoRepeat: true
					autoRepeatDelay: autoRepeat_Delay
					autoRepeatInterval: autoRepeat_Interval
					onClicked:
					{
						DeviceManipulationTabController.increaseRefTranslationOffset(2, offsetTranslationStep);
					}
				}
			}
		}

		// Pitch, Yaw and Roll offsets
		RowLayout
		{
			GridLayout
			{
				columns: 12

				MyText
				{
					text: "Pitch:"
					horizontalAlignment: Text.AlignRight
					Layout.preferredWidth: 70
					Layout.rightMargin: 12
				}

				MyPushButton2
				{
					id: pitchMinusButton
					Layout.preferredWidth: 40
					text: "-"
					autoRepeat: true
					autoRepeatDelay: autoRepeat_Delay
					autoRepeatInterval: autoRepeat_Interval
					onClicked:
					{
						DeviceManipulationTabController.increaseRefRotationOffset(0, -offsetRotationStep);
					}
				}

				MyTextField
				{
					id: pitchInputField
					text: "0.000"
					keyBoardUID: 33
					Layout.preferredWidth: 140
					Layout.leftMargin: 10
					Layout.rightMargin: 10
					horizontalAlignment: Text.AlignHCenter
					function onInputEvent(input)
					{
						var val = parseFloat(input)
						if (!isNaN(val))
						{
							DeviceManipulationTabController.setHMDtoRefRotationOffset(0, val.toFixed(3))
						}

						text = DeviceManipulationTabController.getHMDtoRefRotationOffset(0).toFixed(3)
					}
				}

				MyPushButton2
				{
					id: pitchPlusButton
					Layout.preferredWidth: 40
					text: "+"
					autoRepeat: true
					autoRepeatDelay: autoRepeat_Delay
					autoRepeatInterval: autoRepeat_Interval
					onClicked:
					{
						DeviceManipulationTabController.increaseRefRotationOffset(0, offsetRotationStep);
					}
				}

				MyText
				{
					text: "Yaw:"
					Layout.fillWidth: true
					horizontalAlignment: Text.AlignRight
					Layout.preferredWidth: 40
					Layout.leftMargin: 12
					Layout.rightMargin: 12
				}

				MyPushButton2
				{
					id: yawMinusButton
					Layout.preferredWidth: 40
					text: "-"
					autoRepeat: true
					autoRepeatDelay: autoRepeat_Delay
					autoRepeatInterval: autoRepeat_Interval
					onClicked:
					{
						DeviceManipulationTabController.increaseRefRotationOffset(1, -offsetRotationStep);
					}
				}

				MyTextField
				{
					id: yawInputField
					text: "0.000"
					keyBoardUID: 34
					Layout.preferredWidth: 140
					Layout.leftMargin: 10
					Layout.rightMargin: 10
					horizontalAlignment: Text.AlignHCenter
					function onInputEvent(input)
					{
						var val = parseFloat(input)
						if (!isNaN(val))
						{
							DeviceManipulationTabController.setHMDtoRefRotationOffset(1, val.toFixed(3))
						}

						text = DeviceManipulationTabController.getHMDtoRefRotationOffset(1).toFixed(3)
					}
				}

				MyPushButton2
				{
					id: yawPlusButton
					Layout.preferredWidth: 40
					text: "+"
					autoRepeat: true
					autoRepeatDelay: autoRepeat_Delay
					autoRepeatInterval: autoRepeat_Interval
					onClicked:
					{
						DeviceManipulationTabController.increaseRefRotationOffset(1, offsetRotationStep);
					}
				}

				MyText
				{
					text: "Roll:"
					Layout.fillWidth: true
					horizontalAlignment: Text.AlignRight
					Layout.preferredWidth: 40
					Layout.leftMargin: 12
					Layout.rightMargin: 12
				}

				MyPushButton2
				{
					id: rollMinusButton
					Layout.preferredWidth: 40
					text: "-"
					autoRepeat: true
					autoRepeatDelay: autoRepeat_Delay
					autoRepeatInterval: autoRepeat_Interval
					onClicked:
					{
						DeviceManipulationTabController.increaseRefRotationOffset(2, -offsetRotationStep);
					}
				}

				MyTextField
				{
					id: rollInputField
					text: "0.000"
					keyBoardUID: 35
					Layout.preferredWidth: 140
					Layout.leftMargin: 10
					Layout.rightMargin: 10
					horizontalAlignment: Text.AlignHCenter
					function onInputEvent(input)
					{
						var val = parseFloat(input)
						if (!isNaN(val))
						{
							DeviceManipulationTabController.setHMDtoRefRotationOffset(2, val.toFixed(3))
						}

						text = DeviceManipulationTabController.getHMDtoRefRotationOffset(2).toFixed(3)
					}
				}

				MyPushButton2
				{
					id: rollPlusButton
					Layout.preferredWidth: 40
					text: "+"
					autoRepeat: true
					autoRepeatDelay: autoRepeat_Delay
					autoRepeatInterval: autoRepeat_Interval
					onClicked:
					{
						DeviceManipulationTabController.increaseRefRotationOffset(2, offsetRotationStep);
					}
				}
			}
		}

		// End of section "Offsets"
		RowLayout
		{
			Rectangle
			{
				Layout.topMargin: 0
				height: 1
				border.color: "#ffffff"
				Layout.fillWidth: true
			}
		}

		// Hotkeys
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
					if (DeviceManipulationTabController.isDesktopModeActive())
					{
						keybinding.showPopup(0);
					}
					else
					{
						deviceManipulationMessageDialog.showMessage("Shortcuts", "Due to SteamVR limitations, shortcuts\ncan only be set in desktop mode!")
					}					
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
					DeviceManipulationTabController.removeKey(0);
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
					if (DeviceManipulationTabController.isDesktopModeActive())
					{
						keybinding.showPopup(1);
					}
					else
					{
						deviceManipulationMessageDialog.showMessage("Shortcuts", "Due to SteamVR limitations, shortcuts\ncan only be set in desktop mode!")
					}
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
					DeviceManipulationTabController.removeKey(1);
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

		Component.onCompleted:
        {
            lpfBetaInputField.text = DeviceManipulationTabController.getLPFBeta().toFixed(4)
            samplesInputField.text = DeviceManipulationTabController.getSamples()
			refreshButtonText()
			updateOffsets()
        }

        Connections
        {
            target: DeviceManipulationTabController			
            onSettingChanged:
            {
                lpfBetaInputField.text = DeviceManipulationTabController.getLPFBeta().toFixed(4)
                samplesInputField.text = DeviceManipulationTabController.getSamples()
            }

            onOffsetChanged:
            {
                updateOffsets()
            }
        }
    }

	function updateOffsets()
	{
		xInputField.text = DeviceManipulationTabController.getHMDtoRefTranslationOffset(0).toFixed(3)
		yInputField.text = DeviceManipulationTabController.getHMDtoRefTranslationOffset(1).toFixed(3)
		zInputField.text = DeviceManipulationTabController.getHMDtoRefTranslationOffset(2).toFixed(3)
		pitchInputField.text = DeviceManipulationTabController.getHMDtoRefRotationOffset(0).toFixed(3)
		yawInputField.text = DeviceManipulationTabController.getHMDtoRefRotationOffset(1).toFixed(3)
		rollInputField.text = DeviceManipulationTabController.getHMDtoRefRotationOffset(2).toFixed(3)
	}

	function refreshButtonText()
	{
		btn_enableMC.text = DeviceManipulationTabController.getModifiers_AsString(0) + DeviceManipulationTabController.getKey_AsString(0);
		btn_setZeroPose.text = DeviceManipulationTabController.getModifiers_AsString(1) + DeviceManipulationTabController.getKey_AsString(1);
	}
}
