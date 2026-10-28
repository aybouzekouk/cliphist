import QtQuick

Rectangle {
    id: root

    required property int index
    required property string kind
    required property string text
    required property string imagePath
    property bool selected: false

    height: Theme.rowHeight
    radius: Theme.rowRadius
    color: selected ? Theme.selection : (mouseArea.containsMouse ? Theme.hover : "transparent")

    Behavior on color { ColorAnimation { duration: 120 } }

    Image {
        visible: kind === "image"
        anchors {
            left: parent.left
            leftMargin: 8
            verticalCenter: parent.verticalCenter
        }
        width: 34
        height: 34
        source: imagePath
        sourceSize: Qt.size(68, 68)
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        smooth: true
    }

    Text {
        visible: kind !== "image"
        anchors {
            left: parent.left
            leftMargin: 10
            right: badge.left
            rightMargin: 8
            verticalCenter: parent.verticalCenter
        }
        text: root.text
        color: Theme.textPrimary
        font.pixelSize: Theme.bodySize
        elide: Text.ElideRight
        maximumLineCount: 1
    }

    Rectangle {
        id: badge
        anchors {
            right: parent.right
            rightMargin: 10
            verticalCenter: parent.verticalCenter
        }
        width: badgeText.implicitWidth + 12
        height: 16
        radius: 5
        color: "#14ffffff"

        Text {
            id: badgeText
            anchors.centerIn: parent
            text: kind === "image" ? "IMG" : "TXT"
            color: Theme.textSecondary
            font.pixelSize: Theme.captionSize
            font.weight: Font.DemiBold
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: clipboard.choose(index)
    }
}
