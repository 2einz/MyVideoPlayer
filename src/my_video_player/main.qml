import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Window {
    id: root
    width: 1000
    height: 650
    visible: true
    title: "My Video Player"
    color: "#000000"

    Component.onCompleted: {
        if (typeof playerCtrl === "undefined") {
            console.error("playerCtrl is not registered from C++!");
        }
    }

    // --- Top title bar ---
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        width: parent.width
        height: 50
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#CC000000" }
            GradientStop { position: 1.0; color: "transparent" }
        }
        z: 2

        Text {
            anchors.centerIn: parent
            text: "My Video Player - Playing test video.mp4"
            color: "#EEEEEE"
            font.pixelSize: 14
        }
    }

    // --- Video display area ---
    Rectangle {
        id: videoArea
        anchors.fill: parent
        color: "#111111"

        Text {
            anchors.centerIn: parent
            text: "Video Output (FFmpeg)"
            color: "#333333"
            font.pixelSize: 30
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (playerCtrl)
                    playerCtrl.togglePlay();
            }
        }
    }

    // --- Bottom control bar ---
    Rectangle {
        id: controlBar
        anchors.bottom: parent.bottom
        width: parent.width
        height: 100
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: "#CC000000" }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 8

            // Progress slider
            Slider {
                id: progressSlider
                Layout.fillWidth: true
                value: playerCtrl ? playerCtrl.progress : 0
                onMoved: {
                    if (playerCtrl)
                        playerCtrl.setProgress(value);
                }

                background: Rectangle {
                    height: 4
                    width: progressSlider.availableWidth
                    y: progressSlider.topPadding + progressSlider.availableHeight / 2 - 2
                    color: "#33FFFFFF"
                    radius: 2

                    Rectangle {
                        width: progressSlider.visualPosition * parent.width
                        height: parent.height
                        color: "#0078D4"
                        radius: 2
                    }
                }

                handle: Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: "white"
                }
            }

            // Control buttons row
            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                Button {
                    text: playerCtrl && playerCtrl.isPlaying ? "Pause" : "Play"
                    onClicked: {
                        if (playerCtrl)
                            playerCtrl.togglePlay();
                    }
                }

                Text {
                    text: playerCtrl ? playerCtrl.currentTime + " / " + playerCtrl.totalTime : "00:00 / 00:00"
                    color: "white"
                    font.pixelSize: 12
                }

                Item { Layout.fillWidth: true }

                Button { text: "Settings" }
                Button { text: "Fullscreen" }
            }
        }
    }
}