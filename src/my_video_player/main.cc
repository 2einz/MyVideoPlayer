#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QQmlContext>

#include <QDirIterator>
#include <QDebug>

#include "src/my_video_player/controller.h"

// 必须添加这一行此时使用 _s 就不会报错了
using namespace Qt::StringLiterals;

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    // // 在 main 函数中的 QGuiApplication 之后添加：
    // QDirIterator it(":", QDirIterator::Subdirectories);
    // while (it.hasNext()) {
    //     qDebug() << it.next();
    // }

    video_player::Controller controller;

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("playerCtrl", &controller);

    const QUrl url(u"qrc:/my/video/player/src/my_video_player/main.qml"_s);

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