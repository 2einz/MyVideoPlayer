#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QQmlContext>

#include <QDirIterator>
#include <QDebug>

#include "log/my_spdlog.h"

#include "controller/controller.h"
#include "player/output/qt/video_renderer_qimage.h"

// 必须添加这一行此时使用 _s 就不会报错了
using namespace Qt::StringLiterals;

int main(int argc, char* argv[]) {
    my_log::init();

    QGuiApplication app(argc, argv);

    // // 在 main 函数中的 QGuiApplication 之后添加：
    // QDirIterator it(":", QDirIterator::Subdirectories);
    // while (it.hasNext()) {
    //     qDebug() << it.next();
    // }

    auto* image_provider = new my_video_player::QmlImageProvider();
    my_video_player::Controller controller;

    controller.SetImageProvider(image_provider);

    QQmlApplicationEngine engine;

    engine.addImageProvider("my_player_image", image_provider);
    engine.rootContext()->setContextProperty("playerCtrl", &controller);

    const QUrl url(u"qrc:/my/video/player/ui/main.qml"_s);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}