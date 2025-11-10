import QtQuick 2.15
import QtQuick.Controls 2.15
import osgVerse 1.0

ApplicationWindow
{
    visible: true
    width: 1280
    height: 720
    title: qsTr("osgVerse for QML")
    color: "darkGray"

    Row
    {
        OsgFramebufferObject
        {
            objectName: "main_view"
            renderMode: "pbr"
            focus: true
            width:  parent.width / 2 - parent.spacing / 2
            height: parent.height
        }

        OsgFramebufferObject
        {
            objectName: "sub_view"
            renderMode: "normal"
            focus: false
            width:  parent.width / 2 - parent.spacing / 2
            height: parent.height
        }

        anchors.fill: parent
        spacing: 1
    }
}
