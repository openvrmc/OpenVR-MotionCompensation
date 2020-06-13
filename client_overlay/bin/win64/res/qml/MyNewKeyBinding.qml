import QtQuick 2.12
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import ovrmc.motioncompensation 1.0

Popup
{
	id: myNewKeyBindingPopup

	property int dialogWidth: 600
	property int dialogHeight: 200
	property string keyDescription: "Key description missing"

	implicitHeight: parent.height
	implicitWidth: parent.width

	property int shortcutID: 0

	// Old key settings
	property int oldKey: 0
	property string oldKeyText: ""
	property int oldEventmodifier: 0
	property string oldEventmodifierText: ""

	// New key input
	property int inputNewKey
	property int inputNewEventmodifier

	property var updateShortcut: function() {}

	background: Rectangle
	{
		color: "black"
		opacity: 0.8
	}

	x: Math.round((parent.width - width) / 2)
	y: Math.round((parent.height - height) / 2)

	contentItem: FocusScope
	{
		id: scope

		Item
		{
			id: focusItem
			focus: true
			Keys.onPressed:
			{
				// If user pressed enter, save the new keys
				if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return)
				{
					saveKey();
				}
				// User has pressed escape, discard changes
				else if (event.key === Qt.Key_Escape)
				{
					text_OldKey.text = DeviceManipulationTabController.getStringFromKey(oldKey);
					text_oldEventmodifier.text = DeviceManipulationTabController.getStringFromModifiers(oldEventmodifier);
				}
				else
				{
					inputNewKey = event.key;
					inputNewEventmodifier = event.modifiers;


					// Display the pressed buttons. Only accept certain types of keys
					if ((event.key >= 0x01000030 && event.key <= 0x0100003b)	//F1 - F12
						|| (event.key >= 0x21 && event.key <= 0xff))			//0-9, A-Z, Symbols like , . -
					{
						text_OldKey.text = DeviceManipulationTabController.getStringFromKey(inputNewKey);
					}
					else
					{
						text_OldKey.text = "";
					}

					text_oldEventmodifier.text = DeviceManipulationTabController.getStringFromModifiers(inputNewEventmodifier);
				}
			}
		}

		Rectangle
		{
			implicitWidth: dialogWidth
			implicitHeight: dialogHeight
			anchors.centerIn: parent
			radius: 24
			color: "#1b2939"
			border.color: "#cccccc"
			border.width: 2

			// Regain focus after focus is lost
			MouseArea
			{
				anchors.fill: parent
				hoverEnabled: true

				onEntered:
				{
					focusItem.forceActiveFocus()
				}
			}

			ColumnLayout
			{
				anchors.fill: parent
				anchors.margins: 12

				MyText
				{
					Layout.leftMargin: 16
					Layout.rightMargin: 16
					text: keyDescription
				}

				Rectangle
				{
					color: "#cccccc"
					height: 1
					Layout.fillWidth: true
				}

				Item
				{
					Layout.fillHeight: true
					Layout.fillWidth: true
				}

				Rectangle
				{
					id: rectangle
					Layout.alignment: Qt.AlignCenter
					color: focusItem.activeFocus ? "#406288" : "#365473"
					width: 350;
					height: 40;
					radius: 10;
					antialiasing: true

					RowLayout
					{
						anchors.centerIn: parent

						MyText
						{
							id: text_oldEventmodifier
							text: oldEventmodifierText
						}

						MyText
						{
							id: text_OldKey
							text: oldKeyText
						}
					}
				}
				RowLayout
				{
					Layout.fillWidth: true
					Layout.leftMargin: 24
					Layout.rightMargin: 24
					Layout.bottomMargin: 12

					MyPushButton
					{
						implicitWidth: 200
						text: "Save"
						onClicked:
						{
							saveKey()
							myNewKeyBindingPopup.close()
						}
					}

					Item
					{
						Layout.fillWidth: true
					}

					MyPushButton
					{
						implicitWidth: 200
						text: "Cancel"
						onClicked:
						{
							myNewKeyBindingPopup.close()
						}
					}
				}
			}
		}
	}

	onOpened:
	{
		focusItem.forceActiveFocus()
	}

	function saveKey()
	{
		DeviceManipulationTabController.newKey(shortcutID, inputNewKey, inputNewEventmodifier);
		updateShortcut();
	}

	function refreshUI()
	{
		text_oldEventmodifier.text = oldEventmodifierText;
		text_OldKey.text = oldKeyText;
	}

	function loadValues(id)
	{
		shortcutID = id;
		oldKey = DeviceManipulationTabController.getKey_AsKey(id);
		oldKeyText = DeviceManipulationTabController.getKey_AsString(id);
		oldEventmodifier = DeviceManipulationTabController.getModifiers_AsModifiers(id);
		oldEventmodifierText = DeviceManipulationTabController.getModifiers_AsString(id);
		keyDescription = DeviceManipulationTabController.getKeyDescription(id);

		refreshUI()
	}
}