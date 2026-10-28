import QtQuick

Window {
    id: popup

    width: 340
    height: cardHeight
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    visible: false

    readonly property int cardHeight:
        header.height + Math.min(list.count, 10) * Theme.rowHeight + 2 * Theme.pad

    function hidePopup() {
        if (exitAnim.running)
            return
        exitAnim.restart()
    }

    onVisibleChanged: {
        if (visible) {
            card.opacity = 0
            card.scale = 0.94
            enterAnim.restart()
            list.currentIndex = 0
            popup.requestActivate()
            focusTimer.restart()
        }
    }

    onActiveChanged: {
        if (active)
            list.forceActiveFocus()
        else if (visible)
            dismissTimer.restart()
    }

    Timer {
        id: focusTimer
        interval: 60
        onTriggered: {
            popup.requestActivate()
            list.forceActiveFocus()
        }
    }

    Timer {
        id: dismissTimer
        interval: 120
        onTriggered: {
            if (!popup.active && popup.visible)
                hidePopup()
        }
    }

    Rectangle {
        id: card
        anchors.fill: parent
        radius: Theme.radius
        opacity: 0
        scale: 0.94
        transformOrigin: Item.Center

        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.surfaceTop }
            GradientStop { position: 1.0; color: Theme.surface }
        }
        border.width: 1
        border.color: Theme.border

        Rectangle {
            anchors { left: parent.left; right: parent.right; top: parent.top; leftMargin: 1; rightMargin: 1 }
            height: 1
            color: "#26ffffff"
        }

        Item {
            id: header
            anchors { left: parent.left; right: parent.right; top: parent.top }
            height: 44

            Text {
                anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter }
                text: "Clipboard"
                color: Theme.textPrimary
                font.pixelSize: Theme.titleSize
                font.weight: Font.DemiBold
            }

            Text {
                anchors { right: parent.right; rightMargin: 14; verticalCenter: parent.verticalCenter }
                text: "↑↓  ·  Enter  ·  Esc"
                color: Theme.textSecondary
                font.pixelSize: Theme.captionSize
            }
        }

        Rectangle {
            anchors { left: parent.left; right: parent.right; top: header.bottom; leftMargin: Theme.pad; rightMargin: Theme.pad }
            height: 1
            color: Theme.hairline
        }

        ListView {
            id: list
            anchors {
                left: parent.left
                right: parent.right
                top: header.bottom
                bottom: parent.bottom
                leftMargin: Theme.pad
                rightMargin: Theme.pad
                topMargin: Theme.pad
                bottomMargin: Theme.pad
            }
            model: clipboard
            delegate: ClipEntryDelegate {
                width: list.width
                selected: index === list.currentIndex
            }
            spacing: 3
            interactive: false
            currentIndex: 0
            keyNavigationWraps: true

            Keys.onUpPressed: currentIndex = Math.max(0, currentIndex - 1)
            Keys.onDownPressed: currentIndex = Math.min(count - 1, currentIndex + 1)
            Keys.onReturnPressed: if (count > 0) clipboard.choose(currentIndex)
            Keys.onEscapePressed: hidePopup()
        }
    }

    ParallelAnimation {
        id: enterAnim
        NumberAnimation { target: card; property: "opacity"; to: 1; duration: 170; easing.type: Easing.OutCubic }
        NumberAnimation { target: card; property: "scale"; to: 1; duration: 170; easing.type: Easing.OutCubic }
    }

    ParallelAnimation {
        id: exitAnim
        NumberAnimation { target: card; property: "opacity"; to: 0; duration: 100; easing.type: Easing.InCubic }
        NumberAnimation { target: card; property: "scale"; to: 0.96; duration: 100; easing.type: Easing.InCubic }
        onFinished: popup.visible = false
    }

    Connections {
        target: clipboard
        function onHideRequested() { hidePopup() }
    }
}
