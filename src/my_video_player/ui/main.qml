// qmllint disable missing-property
// qmllint disable unqualified
// qmllint disable import
// qmllint disable unresolved-type
// qmllint disable missing-type

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Effects   // Qt6 标准特效库，用于图标变色

Window {
    id: root
    width: 1250
    height: 750
    visible: true
    title: "Modern Cinema Player"
    color: "#050505"

    // 无边框
    flags: Qt.Window | Qt.FramelessWindowHint

    // 核心状态变量
    property bool isPlaylistVisible: true
    property int playlistWidth: 350

    // 监听全屏状态
    property bool isFullScreen: root.visibility === Window.FullScreen

    // 【核心修改 1】自定义最大化状态，纯手动控制，不再依赖不靠谱的系统 visibility
    property bool isCustomMaximized: false

    // 播放列表假数据
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

    // 自动隐藏控制栏
    Timer {
        id: controlHideTimer
        interval: 5000
        onTriggered: {
            if (isFullScreen)
                controlBar.opacity = 0;
        }
    }

    onVisibilityChanged: {
        if (!isFullScreen) {
            controlBar.opacity = 1;
            topBar.opacity = 1;
        } else {
            controlBar.opacity = 0;
            topBar.opacity = 0;
        }
    }

    // 无边框拖动
    MouseArea {
        id: windowDragArea
        anchors.fill: parent
        z: -1
        // 【核心修改 3】非全屏且非最大化时才允许拖动
        enabled: !isFullScreen && !isCustomMaximized
        onPressed: root.startSystemMove()
    }

    // ==============================================
    // 【核心修改 4】纯手动最大化/还原逻辑，彻底避开底层 Bug
    // ==============================================
    function toggleCustomMaximize() {
        if (isFullScreen)
            return; // 全屏时禁止操作

        if (isCustomMaximized) {
            // 还原：计算屏幕的 1/2 并完美居中
            root.width = Screen.desktopAvailableWidth / 2;
            root.height = Screen.desktopAvailableHeight / 2;
            root.x = Screen.virtualX + (Screen.desktopAvailableWidth - root.width) / 2;
            root.y = Screen.virtualY + (Screen.desktopAvailableHeight - root.height) / 2;

            // 状态改变，按钮图标会自动响应切换为 Maximize
            isCustomMaximized = false;
        } else {
            // 最大化：直接填充满可用区域（自动避开任务栏）
            root.x = Screen.virtualX;
            root.y = Screen.virtualY;
            root.width = Screen.desktopAvailableWidth;
            root.height = Screen.desktopAvailableHeight;

            // 状态改变，按钮图标会自动响应切换为 Resize
            isCustomMaximized = true;
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 左侧视频区域
        Item {
            id: videoContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Rectangle {
                id: videoArea
                anchors.fill: parent
                color: "#000000"

                // Text {
                //     anchors.centerIn: parent
                //     text: "VIDEO ENGINE"
                //     color: "#111111"
                //     font.pixelSize: 60
                //     font.bold: true
                //     font.letterSpacing: 10
                // }
                Image {
                    id: videoOutput
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    // 关键点：使用 image:// 前缀访问 Provider
                    // 后面加个变量来强制刷新缓存
                    source: "image://my_player_image/frame"
                    cache: false
                }

                Connections {
                    target: playerCtrl
                    function onFrameReady() {
                        // 通过修改 source 路径触发 QML 重新调用 requestImage
                        videoOutput.source = "image://my_player_image/frame?t=" + new Date().getTime();
                    }
                }
            }

            MouseArea {
                id: wakeArea
                anchors.fill: parent
                hoverEnabled: true
                onPositionChanged: {
                    if (isFullScreen) {
                        controlBar.opacity = 1;
                        controlHideTimer.restart();
                    }
                }
            }

            // 顶部标题栏
            Rectangle {
                id: topBar
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: root.isCustomMaximized ? 8 : 0
                height: 75
                z: 100
                opacity: isFullScreen ? 0 : 1
                visible: opacity > 0
                Behavior on opacity {
                    NumberAnimation {
                        duration: 300
                    }
                }
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
                    anchors.leftMargin: 25
                    anchors.rightMargin: 15

                    Column {
                        Layout.alignment: Qt.AlignVCenter
                        Text {
                            text: "Now Playing"
                            color: "#666666"
                            font.pixelSize: 11
                        }
                        Text {
                            text: (playlistModel.count > 0 && playlistView.currentIndex >= 0) ? playlistModel.get(playlistView.currentIndex).title : "Ready to play"
                            color: "white"
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    // 窗口控制按钮
                    Row {
                        spacing: 2
                        Layout.alignment: Qt.AlignTop | Qt.AlignRight
                        Layout.topMargin: 8

                        ControlButton {
                            iconWidth: 14
                            iconSource: "assets/player_minimize.svg"
                            onClicked: root.showMinimized()
                        }

                        // 最大化/还原按钮
                        ControlButton {
                            id: maximizeBtn
                            iconWidth: 14
                            // 纯依赖 isCustomMaximized 变量，逻辑坚不可摧
                            iconSource: root.isCustomMaximized ? "assets/player_resize.svg" : "assets/player_maximize.svg"
                            onClicked: toggleCustomMaximize()
                        }

                        // 关闭按钮
                        ControlButton {
                            iconWidth: 10.6
                            // iconColor: "#FF5555"
                            iconSource: "assets/player_close.svg"
                            onClicked: Qt.quit()
                        }
                    }
                }
            }

            // 底部控制栏
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
                z: 10
                Behavior on opacity {
                    NumberAnimation {
                        duration: 500
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 8

                    Slider {
                        id: progressSlider
                        Layout.fillWidth: true
                        from: 0.0
                        to: 1.0
                        implicitHeight: 24 // 增加高度，方便点击

                        // 清空所有内边距，确保坐标系从 (0,0) 开始
                        padding: 0
                        leftPadding: 0
                        rightPadding: 0
                        topPadding: 0
                        bottomPadding: 0

                        Binding {
                            target: progressSlider
                            property: "value"
                            value: (typeof playerCtrl !== "undefined") ? playerCtrl.progress : 0.0
                            when: !progressSlider.pressed
                        }

                        // --- 背景轨道 ---
                        background: Item {
                            width: progressSlider.width
                            height: progressSlider.height

                            // 灰色底条
                            Rectangle {
                                id: trackBase
                                width: parent.width
                                height: 4
                                radius: 2
                                color: "#22FFFFFF"
                                y: (parent.height - height) / 2
                            }

                            // 蓝色进度条
                            Rectangle {
                                // 确保起点对齐
                                x: 0
                                // 终点对齐球的中心
                                width: sliderHandle.x + (sliderHandle.width / 2)
                                height: 4
                                radius: 2
                                color: "#0078D4"
                                y: (parent.height - height) / 2
                            }
                        }

                        // --- 滑块球 ---
                        handle: Rectangle {
                            id: sliderHandle
                            // x 计算：确保球不会超出左右边界
                            x: progressSlider.visualPosition * (progressSlider.width - width)

                            y: (progressSlider.height - height) / 2

                            width: progressSlider.pressed ? 16 : 12
                            height: width
                            radius: width / 2
                            color: "white"

                            Behavior on width {
                                NumberAnimation {
                                    duration: 100
                                }
                            }

                            // 鼠标移入时的外发光
                            Rectangle {
                                anchors.centerIn: parent
                                width: parent.width + 8
                                height: parent.height + 8
                                radius: width / 2
                                color: "#20FFFFFF"
                                visible: progressSlider.hovered || progressSlider.pressed
                                border.color: "#44000000"
                                border.width: 1
                            }
                        }

                        onMoved: {
                            if (typeof playerCtrl != "undefined") {
                                playerCtrl.progress = value;
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 18

                        ControlButton {
                            iconSource: "assets/player_prior.svg"
                        }
                        ControlButton {
                            id: playBtn
                            iconSource: (typeof playerCtrl !== "undefined" && playerCtrl.isPlaying) ? "assets/player_pause.svg" : "assets/player_play.svg"
                            iconWidth: 32
                            onClicked: if (typeof playerCtrl !== "undefined") {
                                playerCtrl.TogglePlay();
                            }
                        }
                        ControlButton {
                            iconSource: "assets/player_next.svg"
                        }

                        Text {
                            text: (typeof playerCtrl !== "undefined") ? playerCtrl.currentTime + " / " + playerCtrl.totalTime : "03:45 / 12:00"
                            color: "#888"
                            font.pixelSize: 12
                            font.family: "Monospace"
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        ControlButton {
                            iconSource: "assets/player_menu.svg"
                            iconColor: isPlaylistVisible ? "#0078D4" : "#FFF"
                            onClicked: isPlaylistVisible = !isPlaylistVisible
                        }

                        ControlButton {
                            iconSource: "assets/player_fullscreen.svg"
                            iconColor: isFullScreen ? "#0078D4" : "#FFF"
                            onClicked: {
                                if (isFullScreen) {
                                    root.showNormal();
                                    // 强制恢复 UI
                                    // isPlaylistVisible = true;
                                    controlBar.opacity = 1;
                                    topBar.opacity = 1;
                                } else {
                                    root.showFullScreen();
                                    isPlaylistVisible = false;
                                }
                            }
                        }
                    }
                }
            }
        }

        // 右侧播放列表
        Rectangle {
            id: playlistArea
            Layout.preferredWidth: (isPlaylistVisible && !isFullScreen) ? playlistWidth : 0
            Layout.fillHeight: true
            color: "#0A0A0A"
            border.color: "#151515"
            clip: true
            Behavior on Layout.preferredWidth {
                NumberAnimation {
                    duration: 400
                    easing.type: Easing.OutCubic
                }
            }

            ColumnLayout {
                width: playlistWidth
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
                        text: "Playlist"
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
                        width: playlistWidth
                        height: 95
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 10
                            radius: 12
                            color: playlistView.currentIndex === index ? "#200078D4" : (delegateMouse.containsMouse ? "#10FFF" : "transparent")
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 15
                                Rectangle {
                                    width: 90
                                    height: 50
                                    radius: 6
                                    color: model.color
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text {
                                        text: model.title
                                        color: "white"
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
                                if (typeof playerCtrl !== "undefined") {
                                    playerCtrl.OpenFile("D:/Workspace/AudioVideo/test_media_files/trailer/trailer.mp4");
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 通用按钮组件
    component ControlButton: Item {
        property url iconSource: ""
        property real iconWidth: 24
        property color iconColor: "#FFF"
        signal clicked
        implicitWidth: 44
        implicitHeight: 44

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: parent.clicked()
        }

        Rectangle {
            anchors.fill: parent
            radius: 12
            color: mouse.containsMouse ? "#20FFFFFF" : "transparent"
            Behavior on color {
                ColorAnimation {
                    duration: 200
                }
            }
        }

        Image {
            id: btnIcon
            anchors.centerIn: parent
            width: parent.iconWidth
            height: width
            source: parent.iconSource
            fillMode: Image.PreserveAspectFit
            smooth: true
            antialiasing: true
            visible: false
        }

        MultiEffect {
            anchors.fill: btnIcon
            source: btnIcon
            colorizationColor: parent.iconColor
            colorization: 1.0
            opacity: mouse.pressed ? 0.5 : 1.0
        }
    }

    Shortcut {
        sequence: "Esc"
        onActivated: {
            if (isFullScreen)
                root.showNormal();
        }
    }
}
