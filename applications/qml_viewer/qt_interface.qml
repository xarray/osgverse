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

    OsgFramebufferObject
    {
        focus: true
        anchors.fill: parent
        anchors.margins: 10
    }
}
