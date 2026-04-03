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

    // 控制列表展开/收起的变量
    property bool isPlaylistVisible: true
    property int playlistWidth: 350

    ListModel {
        id: playlistModel
        ListElement {
            title: "01. 4K 自然风光演示"
            duration: "03:45"
            author: "Nature"
            color: "#1a3a3a"
        }
        ListElement {
            title: "02. FFmpeg 底层解码原理"
            duration: "12:20"
            author: "TechDev"
            color: "#3a1a1a"
        }
        ListElement {
            title: "03. QML 粒子特效动画"
            duration: "05:10"
            author: "Design"
            color: "#1a1a3a"
        }
        ListElement {
            title: "04. 极地冰川纪录片"
            duration: "45:00"
            author: "Discovery"
            color: "#2a2a2a"
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ======================== 左侧：视频播放区域 ========================
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
                    font.pixelSize: 60
                    font.bold: true
                    font.letterSpacing: 10
                }
            }

            // --- 顶部标题栏 ---
            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 100
                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: "#D0000000"
                    }
                    GradientStop {
                        position: 1
                        color: "transparent"
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 25
                    Column {
                        Layout.alignment: Qt.AlignTop
                        Text {
                            text: "正在播放"
                            color: "#666666"
                            font.pixelSize: 11
                        }
                        Text {
                            text: playlistModel.get(playlistView.currentIndex).title
                            color: "white"
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }
                    }
                }
            }

            // --- 悬浮控制栏 ---
            Rectangle {
                id: controlBar
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 30
                width: Math.min(parent.width - 60, 950)
                height: 90
                radius: 24
                color: "#E61A1A1A"
                border.color: "#1AFFFFFF"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 8

                    Slider {
                        id: progressSlider
                        Layout.fillWidth: true
                        value: playerCtrl ? playerCtrl.progress : 0.4
                        background: Rectangle {
                            height: 4
                            radius: 2
                            color: "#22FFFFFF"
                            Rectangle {
                                width: progressSlider.visualPosition * parent.width
                                height: parent.height
                                color: "#0078D4"
                                radius: 2
                            }
                        }
                        handle: Rectangle {
                            x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - 14)
                            y: progressSlider.topPadding + progressSlider.availableHeight / 2 - 7
                            width: 14
                            height: 14
                            radius: 7
                            color: "white"
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 18

                        // 按钮组：使用白色图标资源
                        ControlButton {
                            iconSource: "assets/player_prior.svg"
                            onClicked: if (playlistView.currentIndex > 0)
                                playlistView.currentIndex--
                        }

                        ControlButton {
                            id: playBtn
                            iconSource: (playerCtrl && playerCtrl.isPlaying) ? "assets/player_pause.svg" : "assets/player_play.svg"
                            iconWidth: 32
                            onClicked: if (playerCtrl)
                                playerCtrl.togglePlay()
                        }

                        ControlButton {
                            iconSource: "assets/player_next.svg"
                            onClicked: if (playlistView.currentIndex < playlistModel.count - 1)
                                playlistView.currentIndex++
                        }

                        Text {
                            text: playerCtrl ? playerCtrl.currentTime + " / " + playerCtrl.totalTime : "03:45 / 12:00"
                            color: "#888888"
                            font.pixelSize: 12
                            font.family: "Monospace"
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        // 列表切换按钮 (关键：控制伸缩)
                        ControlButton {
                            iconSource: "assets/player_menu.svg"
                            // 如果列表打开，图标变蓝
                            iconColor: root.isPlaylistVisible ? "#0078D4" : "#FFFFFF"
                            onClicked: root.isPlaylistVisible = !root.isPlaylistVisible
                        }

                        ControlButton {
                            iconSource: "assets/player_fullscreen.svg"
                            onClicked: root.visibility = (root.visibility === Window.FullScreen) ? Window.Windowed : Window.FullScreen
                        }
                    }
                }
            }
        }

        // ======================== 右侧：可伸缩播放列表 ========================
        Rectangle {
            id: playlistArea
            // 核心动画逻辑：宽度随变量改变
            Layout.preferredWidth: root.isPlaylistVisible ? root.playlistWidth : 0
            Layout.fillHeight: true
            color: "#0A0A0A"
            clip: true // 必须开启裁剪，防止缩回时内容溢出

            // 伸缩动画
            Behavior on Layout.preferredWidth {
                NumberAnimation {
                    duration: 400
                    easing.type: Easing.InOutQuad
                }
            }

            ColumnLayout {
                width: root.playlistWidth // 保持内容宽度不变，避免缩放时文字挤压
                height: parent.height
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    height: 80
                    color: "transparent"
                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 25
                        text: "播放列表"
                        color: "white"
                        font.pixelSize: 20
                        font.bold: true
                    }
                }

                ListView {
                    id: playlistView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: playlistModel
                    clip: true
                    currentIndex: 0

                    delegate: Item {
                        width: root.playlistWidth
                        height: 95
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 10
                            radius: 12
                            color: playlistView.currentIndex === index ? "#200078D4" : (delegateMouse.containsMouse ? "#10FFFFFF" : "transparent")

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 15
                                Rectangle {
                                    width: 90
                                    height: 50
                                    radius: 6
                                    color: model.color
                                    Text {
                                        anchors.centerIn: parent
                                        text: "▶"
                                        color: "white"
                                        opacity: playlistView.currentIndex === index ? 1 : 0
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text {
                                        text: model.title
                                        color: playlistView.currentIndex === index ? "#0078D4" : "white"
                                        font.pixelSize: 14
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: model.author
                                        color: "#555"
                                        font.pixelSize: 11
                                    }
                                }
                            }
                        }
                        MouseArea {
                            id: delegateMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                playlistView.currentIndex = index;
                            }
                        }
                    }
                }
            }
        }
    }

    // ======================== 通用按钮组件 ========================
    component ControlButton: Item {
        property url iconSource: ""
        property int iconWidth: 24
        property color iconColor: "#FFFFFF" // 默认白色
        signal clicked

        implicitWidth: 44
        implicitHeight: 44

        Rectangle {
            id: bg
            anchors.fill: parent
            radius: 12
            color: mouse.containsMouse ? "#20FFFFFF" : "transparent"
            Behavior on color {
                ColorAnimation {
                    duration: 200
                }
            }
        }

        // 使用 IconImage 可以方便地根据状态切换颜色
        Image {
            id: img
            anchors.centerIn: parent
            width: parent.iconWidth
            height: width
            source: parent.iconSource
            sourceSize: Qt.size(64, 64)
            fillMode: Image.PreserveAspectFit

            // 简单的变色逻辑：如果想用黑色图标，可以根据逻辑切换
            // 这里我们默认使用白色图标并配合颜色滤镜（可选）
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: parent.clicked()
            onPressed: bg.scale = 0.9
            onReleased: bg.scale = 1.0
        }
    }
}

