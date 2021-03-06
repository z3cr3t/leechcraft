import QtQuick 1.0
import org.LC.common 1.0

Rectangle {
    id: rootRect
    anchors.fill: parent
    smooth: true
    radius: 5

    color: "#ee000000"

    signal toggleMountRequested(string id)
    signal toggleHideRequested(string id)
    signal toggleShowHidden()

    Text {
        id: topLabel

        text: devicesLabelText
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: 12
        color: "#eeeeee"

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 5
    }

    ActionButton {
        actionIconURL: "image://ThemeIcons/emblem-symbolic-link"

        anchors.right: parent.right
        anchors.rightMargin: 5
        anchors.top: topLabel.top
        anchors.bottom: topLabel.bottom

        width: height

        opacity: hasHiddenItems ? 1 : 0
        Behavior on opacity { PropertyAnimation { } }

        onTriggered: rootRect.toggleShowHidden()
    }

    ListView {
        id: devicesView

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: topLabel.bottom
        anchors.topMargin: 3
        anchors.bottom: parent.bottom

        clip: true

        model: devModel
        spacing: 20

        delegate: Rectangle {
            color: "#00000000"

            width: devicesView.width
            height: devNameLabel.height + totalSizeLabel.height + mountedAtLabel.height + availIndicator.height

            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.leftMargin: 5
                anchors.right: parent.right
                anchors.rightMargin: 5

                height: 1

                color: "#777777"
            }

            Text {
                id: devNameLabel
                text: devName
                color: "#dddddd"
                font.bold: true

                anchors.top: parent.top
                anchors.topMargin: 5
                anchors.left: parent.left
                anchors.leftMargin: 5
            }

            Text {
                id: devFileLabel
                text: devFile
                color: "#888888"
                font.italic: true

                anchors.top: parent.top
                anchors.topMargin: 5
                anchors.left: parent.left
                anchors.leftMargin: parent.width - 60
                anchors.right: parent.right
                anchors.rightMargin: 5
            }

            Text {
                id: totalSizeLabel
                text: formattedTotalSize
                color: "#aaaaaa"
                font.italic: true

                anchors.top: devNameLabel.bottom
                anchors.topMargin: 2
                anchors.left: parent.left
                anchors.leftMargin: 5
            }

            Text {
                id: mountedAtLabel
                text: mountedAt
                color: "#999999"
                font.italic: true

                visible: text.length != 0

                anchors.top: totalSizeLabel.bottom
                anchors.topMargin: 2
                anchors.left: parent.left
                anchors.leftMargin: 5
            }

            ProgressBar {
                id: availIndicator

                visible: availableSize > 0
                color: usedPercentage > 90 ? "#ff1d00" : "#999999"

                minimum: 0
                maximum: 100
                value: usedPercentage

                anchors.top: mountedAtLabel.bottom
                anchors.topMargin: 2
                anchors.left: parent.left
                anchors.leftMargin: 5
            }

            ActionButton {
                id: mountButton
                actionIconURL: mountButtonIcon

                width: height
                anchors.top: devFileLabel.bottom
                anchors.topMargin: 5
                anchors.right: parent.right
                anchors.rightMargin: 5
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2

                onTriggered: rootRect.toggleMountRequested(devID)
            }

            ActionButton {
                id: hideButton
                actionIconURL: toggleHiddenIcon
                visible: devPersistentID.length > 0

                width: height
                height: mountButton.height * 2 / 3
                anchors.right: mountButton.left
                anchors.verticalCenter: mountButton.verticalCenter

                onTriggered: rootRect.toggleHideRequested(devPersistentID)
            }
        }
    }
}
