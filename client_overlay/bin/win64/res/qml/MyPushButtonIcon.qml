import QtQuick 2.9
import QtQuick.Controls 2.0
import "." // QTBUG-34418, singletons require explicit import to load qmldir file

Button
{
    property bool activationSoundEnabled: true
	property string imagesource: ""
	hoverEnabled: true


	background: Rectangle
	{
        color: parent.down ? "#406288" : (parent.activeFocus ? "#365473" : "#2c435d")
    }

	Image
	{
		anchors.centerIn: parent
		anchors.fill: parent
		fillMode: Image.PreserveAspectFit
		source: imagesource
	}

	onHoveredChanged:
	{
		if (hovered)
		{
            forceActiveFocus()
		}
		else
		{
            focus = false
        }
	}
}
