import QtQuick 1.0

Rectangle {
    width: 360
    height: 360
    color: "black"
    Rectangle {
        id: sendGoogleRequest
        width: 100
        height: 100
        color: "red"
        border.color: "black"
        border.width: 5
        radius: 20
        MouseArea {
            anchors.fill: parent
            onClicked: {
                myObject.sendGoogleReq("test")
                //myObject.cppSlot(12345)
            }
        }
        Text {
            text: "Send to google"
            anchors.centerIn: parent
        }
    }

    Rectangle {
        id: recordButton
        anchors.left: sendGoogleRequest.right
        width: 100
        height: 100
        color: "green"
        border.color: "black"
        border.width: 5
        radius: 20
        MouseArea {
            anchors.fill: parent
            onClicked: {
                myObject.recordVoice()
            }
        }
        Text {
            text: "Record Voice"
            anchors.centerIn: parent
        }
    }
}
