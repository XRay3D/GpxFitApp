#include "fit.h"
#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <set>

auto messageHandler = qInstallMessageHandler(nullptr);
void myMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    auto file = context.file;
    // if(type == QtInfoMsg) return;
    QMessageLogContext& context_ = const_cast<QMessageLogContext&>(context);
    while(file && *file)
        if(std::set{'/', '\\'}.contains(*file++))
            context_.file = file;

           // QString data{context_.function};
           // data.replace(QRegularExpression(R"((\w+\:\:))"), "");
           // context_.function = data.toUtf8().data();
    messageHandler(type, context, message);
    // if(App::mainWindowPtr())
    //     emit App::mainWindow().logMessage(type, {
    //                                                 QString::fromUtf8(context.category),
    //                                                 QString::fromUtf8(context.file),
    //                                                 QString::fromUtf8(context.function),
    //                                                 QString::number(context.line),

    //                                             },
    //         message);
}

int main(int argc, char* argv[]) {
    qInstallMessageHandler(myMessageHandler);
    qSetMessagePattern(QLatin1String(
        "%{if-critical}\x1b[38;2;255;0;0m"
        "C %{endif}"
        "%{if-debug}\x1b[38;2;196;196;196m"
        "D %{endif}"
        "%{if-fatal}\x1b[1;38;2;255;0;0m"
        "F %{endif}"
        "%{if-info}\x1b[38;2;128;255;255m"
        "I %{endif}"
        "%{if-warning}\x1b[38;2;255;128;0m"
        "W %{endif}"
        // "%{time HH:mm:ss.zzz} "
        // "%{appname} %{pid} %{threadid} "
        // "%{type} "
        // "%{file}:%{line} %{function} "
        "%{if-category}%{category}%{endif}%{message} "
        "\x1b[38;2;64;64;64m <- %{function} <- %{file} : %{line}\x1b[0m"));

    QApplication /*QGuiApplication*/ app(argc, argv);

    QQmlApplicationEngine engine;

    // qmlRegisterType<Fit>("org.example", 1, 0, "Fit");

    Fit fit;
    engine.rootContext()->setContextProperty("fit", &fit);
    fit.loadFile("C:/Users/bakiev/Downloads/Telegram Desktop/MAGENE_C206Pro_2023-08-05-09-57-40_50229574_1691267580997.fit");

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, [] { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &app, [&engine] {
            Fit* fit = engine.singletonInstance<Fit*>("FitApp", "fit");
            if(fit) fit->setText("77");
        },
        Qt::QueuedConnection);

    engine.loadFromModule("FitApp", "Main");
    // fit.setText("77");

    return app.exec();
}
