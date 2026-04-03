import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Window {
    id: root
    width: 1250
    height: 750
    visible: true
    title: "Modern Cinema Player"
    color: "#050505"

    flags: Qt.Window | Qt.FramelessWindowHint

    // ==================== 状态变量 ====================
    property bool isPlaylistVisible: true
    property int playlistWidth: 350

    property bool isFullScreen: root.visibility === Window.FullScreen
    property bool isMaximized: root.visibility === Window.Maximized

    // 最大化前窗口尺寸与位置
    property real prevWidth: width
    property real prevHeight: height
    property real prevX: x
    property real prevY: y

    // ==================== 播放列表假数据 ====================
    ListModel {
        id: playlistModel
        ListElement { title: "01. 4K 自然风光演示"; duration: "03:45"; author: "Nature"; color: "#1a3a3a" }
        ListElement { title: "02. FFmpeg 底层解码原理"; duration: "12:20"; author: "TechDev"; color: "#3a1a1a" }
        ListElement { title: "03. QML 粒子特效动画"; duration: "05:10"; author: "Design"; color: "#1a1a3a" }
        ListElement { title: "04. 极地冰川纪录片"; duration: "45:00"; author: "Discovery"; color: "#2a2a2a" }
    }

    // ==================== 自动隐藏控制栏（仅全屏生效） ====================
    Timer {
        id: controlHideTimer
        interval: 5000
        onTriggered: {
            if (isFullScreen) controlBar.opacity = 0
        }
    }

    onVisibilityChanged: {
        if (isFullScreen) {
            controlBar.opacity = 0
            topBar.opacity = 0
        } else {
            controlBar.opacity = 1
            topBar.opacity = 1
        }
    }

    // ==================== 无边框拖动 ====================
    MouseArea {
        id: windowDragArea
        anchors.fill: parent
        z: -1
        enabled: !isFullScreen && !isMaximized
        onPressed: root.startSystemMove()
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ==================== 左侧视频区域 ====================
        Item {
            id: videoContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Rectangle {
                id: videoArea
                anchors.fill: parent
                color: "#000000"
                Text {
                    anchors.centerIn: parent
                    text: "VIDEO ENGINE"
                    color: "#111111"
                    font.pixelSize: 60; font.bold: true; font.letterSpacing: 10
                }
            }

            MouseArea {
                id: wakeArea
                anchors.fill: parent
                hoverEnabled: true
                onPositionChanged: {
                    if (isFullScreen) controlBar.opacity = 1
                    controlHideTimer.restart()
                }
            }

            // ==================== 顶部标题栏 ====================
            Rectangle {
                id: topBar
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: isMaximized ? 8 : 0
                height: 75
                z: 100

                opacity: isFullScreen ? 0 : 1
                visible: opacity > 0
                Behavior on opacity { NumberAnimation { duration: 300 } }

                gradient: Gradient {
                    GradientStop { position: 0; color: "#D0000000" }
                    GradientStop { position: 1; color: "transparent" }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 25
                    anchors.rightMargin: 15

                    Column {
                        Layout.alignment: Qt.AlignVCenter
                        Text { text: "Now Playing"; color: "#666666"; font.pixelSize: 11 }
                        Text {
                            text: (playlistModel.count > 0 && playlistView.currentIndex >= 0)
                                  ? playlistModel.get(playlistView.currentIndex).title
                                  : "Ready to play"
                            color: "white"; font.pixelSize: 18; font.weight: Font.DemiBold
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // ==================== 窗口控制按钮 ====================
                    Row {
                        spacing: 2
                        Layout.alignment: Qt.AlignTop | Qt.AlignRight
                        Layout.topMargin: 8

                        ControlButton {
                            iconWidth: 14; iconSource: "assets/player_minimize.svg"
                            onClicked: root.showMinimized()
                        }

                        // 最大化 / 还原按钮
                        ControlButton {
                            iconWidth: 14
                            iconSource: root.isMaximized ? "assets/player_resize.svg" : "assets/player_maximize.svg"
                            onClicked: {
                                if (root.isMaximized) {
                                    // 还原窗口到最大化前尺寸
                                    root.x = prevX
                                    root.y = prevY
                                    root.width = prevWidth
                                    root.height = prevHeight
                                    root.showNormal()
                                } else {
                                    // 记录当前窗口尺寸和位置
                                    prevWidth = root.width
                                    prevHeight = root.height
                                    prevX = root.x
                                    prevY = root.y
                                    root.showMaximized()
                                }
                            }
                        }

                        ControlButton {
                            iconWidth: 12.6
                            iconColor: "#FF5555"
                            iconSource: "assets/player_close.svg"
                            onClicked: Qt.quit()
                        }
                    }
                }
            }

            // ==================== 底部控制栏 ====================
            Rectangle {
                id: controlBar
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 30
                width: Math.min(parent.width - 60, 950)
                height: 90; radius: 24
                color: "#E61A1A1A"; border.color: "#1AFFFFFF"
                z: 10

                opacity: isFullScreen ? 0 : 1
                Behavior on opacity { NumberAnimation { duration: 500 } }

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 15; spacing: 8

                    Slider {
                        id: progressSlider
                        Layout.fillWidth: true
                        value: (typeof playerCtrl !== "undefined") ? playerCtrl.progress : 0.4
                        background: Rectangle {
                            height: 4; radius: 2; color: "#22FFFFFF"
                            Rectangle { width: progressSlider.visualPosition * parent.width; height: parent.height; color: "#0078D4"; radius: 2 }
                        }
                        handle: Rectangle { width: 14; height: 14; radius: 7; color: "white" }
                    }

                    RowLayout {
                        Layout.fillWidth: true; spacing: 18

                        ControlButton { iconSource: "assets/player_prior.svg" }
                        ControlButton {
                            id: playBtn
                            iconSource: (typeof playerCtrl !== "undefined" && playerCtrl.isPlaying) ? "assets/player_pause.svg" : "assets/player_play.svg"
                            iconWidth: 32
                            onClicked: if (typeof playerCtrl !== "undefined") playerCtrl.togglePlay()
                        }
                        ControlButton { iconSource: "assets/player_next.svg" }

                        Text {
                            text: (typeof playerCtrl !== "undefined") ? playerCtrl.currentTime + " / " + playerCtrl.totalTime : "03:45 / 12:00"
                            color: "#888"; font.pixelSize: 12; font.family: "Monospace"
                        }

                        Item { Layout.fillWidth: true }

                        ControlButton {
                            iconSource: "assets/player_menu.svg"
                            iconColor: isPlaylistVisible ? "#0078D4" : "#FFF"
                            onClicked: isPlaylistVisible = !isPlaylistVisible
                        }

                        // 全屏按钮
                        ControlButton {
                            iconSource: "assets/player_fullscreen.svg"
                            iconColor: isFullScreen ? "#0078D4" : "#FFF"
                            onClicked: {
                                if (isFullScreen)
                                    root.showNormal()
                                else
                                    root.showFullScreen()
                            }
                        }
                    }
                }
            }
        }

        // ==================== 右侧播放列表 ====================
        Rectangle {
            id: playlistArea
            Layout.preferredWidth: (isPlaylistVisible && !isFullScreen) ? playlistWidth : 0
            Layout.fillHeight: true
            color: "#0A0A0A"; border.color: "#151515"
            clip: true

            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: 400; easing.type: Easing.OutCubic }
            }

            ColumnLayout {
                width: playlistWidth
                height: parent.height
                spacing: 0

                Rectangle { Layout.fillWidth: true; height: 80; color: "transparent"
                    Text { anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; anchors.leftMargin: 25
                        text: "Playlist"; color: "white"; font.pixelSize: 20; font.bold: true }
                }

                ListView {
                    id: playlistView
                    Layout.fillWidth: true; Layout.fillHeight: true
                    model: playlistModel; clip: true; currentIndex: 0
                    delegate: Item {
                        width: playlistWidth; height: 95
                        Rectangle {
                            anchors.fill: parent; anchors.margins: 10; radius: 12
                            color: playlistView.currentIndex === index ? "#200078D4" : (delegateMouse.containsMouse ? "#10FFF" : "transparent")
                            RowLayout { anchors.fill: parent; anchors.margins: 12; spacing: 15
                                Rectangle { width: 90; height: 50; radius: 6; color: model.color }
                                ColumnLayout { Layout.fillWidth: true; spacing: 2
                                    Text { text: model.title; color: "white"; font.pixelSize: 14; elide: Text.ElideRight; Layout.fillWidth: true }
                                    Text { text: model.author; color: "#555"; font.pixelSize: 11 }
                                }
                            }
                        }
                        MouseArea { id: delegateMouse; anchors.fill: parent; hoverEnabled: true; onClicked: playlistView.currentIndex = index }
                    }
                }
            }
        }
    }

    // ==================== 通用按钮组件 ====================
    component ControlButton : Item {
        property url iconSource: ""
        property real iconWidth: 24
        property color iconColor: "#FFF"
        signal clicked()

        implicitWidth: 44; implicitHeight: 44

        Rectangle {
            id: bg; anchors.fill: parent; radius: 12
            color: mouse.containsMouse ? "#20FFFFFF" : "transparent"
            Behavior on color { ColorAnimation { duration: 200 } }
        }

        Image {
            anchors.centerIn: parent
            width: parent.iconWidth; height: width
            source: parent.iconSource; fillMode: Image.PreserveAspectFit
            smooth: true; antialiasing: true
            opacity: mouse.pressed ? 0.5 : 1.0
        }

        MouseArea {
            id: mouse; anchors.fill: parent; hoverEnabled: true
            onClicked: parent.clicked()
        }
    }

    Shortcut {
        sequence: "Esc"
        onActivated: {
            if (isFullScreen) {
                root.showNormal()
            }
        }
    }
}