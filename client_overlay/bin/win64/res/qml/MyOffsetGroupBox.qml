import QtQuick 2.9
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

GroupBox
{
	property string boxTitle: "No title provided"

    property double offsetX: 0.0
    property double offsetY: 0.0
    property double offsetZ: 0.0

    property double offsetTranslationStep: 0.001

    property var setTranslationOffset: function(x, y, z) {}
    property var updateValues: function() {}
	height: 90

    function updateGUI()
    {
        xInputField.text = offsetX.toFixed(3)
        yInputField.text = offsetY.toFixed(3)
        zInputField.text = offsetZ.toFixed(3)
    }

    Layout.fillWidth: true

    label: MyText
    {
        leftPadding: 10
		text: boxTitle
        bottomPadding: 0
    }

    background: Rectangle
    {
        height: 90
        color: "transparent"
        border.color: "#ffffff"
        radius: 8
    }

    ColumnLayout
    {
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 27
        anchors.fill: parent

        GridLayout
        {
            columns: 12

            MyText
            {
                text: "X:"
                horizontalAlignment: Text.AlignRight
                Layout.preferredWidth: 80
                Layout.rightMargin: 12
            }

            MyPushButton2
            {
                id: xMinusButton
                Layout.preferredWidth: 40
                text: "-"
                onClicked:
                {
                    var value = offsetX - offsetTranslationStep
                    setTranslationOffset(value, offsetY, offsetZ)
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
                        setTranslationOffset(val.toFixed(3), offsetY, offsetZ)
                    }
                    else
                    {
                        getOffsets()
                    }
                }
            }

            MyPushButton2
            {
                id: xPlusButton
                Layout.preferredWidth: 40
                text: "+"
                onClicked:
                {
                    var value = offsetX + offsetTranslationStep
                    setTranslationOffset(value, offsetY, offsetZ)
                }
            }

            MyText
            {
                text: "Y:"
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                Layout.leftMargin: 12
                Layout.rightMargin: 12
            }

            MyPushButton2
            {
                id: yMinusButton
                Layout.preferredWidth: 40
                text: "-"
                onClicked:
                {
                    var value = offsetY - offsetTranslationStep
                    setTranslationOffset(offsetX, value, offsetZ)
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
                        setTranslationOffset(offsetX, val.toFixed(3), offsetZ)
                    }
                    else
                    {
                        getOffsets()
                    }
                }
            }

            MyPushButton2
            {
                id: yPlusButton
                Layout.preferredWidth: 40
                text: "+"
                onClicked:
                {
                    var value = offsetY + offsetTranslationStep
                    setTranslationOffset(offsetX, value, offsetZ)
                }
            }

            MyText
            {
                text: "Z:"
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                Layout.leftMargin: 12
                Layout.rightMargin: 12
            }

            MyPushButton2
            {
                id: zMinusButton
                Layout.preferredWidth: 40
                text: "-"
                onClicked:
                {
                    var value = offsetZ - offsetTranslationStep
                    setTranslationOffset(offsetX, offsetY, value)
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
                        setTranslationOffset(offsetX, offsetY, val.toFixed(3))
                    }
                    else
                    {
                        getOffsets()
                    }
                }
            }

            MyPushButton2
            {
                id: zPlusButton
                Layout.preferredWidth: 40
                text: "+"
                onClicked:
                {
                    var value = offsetZ + offsetTranslationStep
                    setTranslationOffset(offsetX, offsetY, value)
                }
            }
        }
    }
}
