#include "fit.h"
#include <QApplication>
#include <QFileDialog>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
// #include <QAndroidJniObject>
#include <QQmlContext>
#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QtCore/private/qandroidextras_p.h>
#endif
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

#ifdef Q_OS_ANDROID
bool checkPermission() {
    QList<bool> permissions;

    auto r = QtAndroidPrivate::checkPermission("android.permission.READ_EXTERNAL_STORAGE").result();
    if(r != QtAndroidPrivate::Authorized) {
        r = QtAndroidPrivate::requestPermission("android.permission.READ_EXTERNAL_STORAGE").result();
        if(r == QtAndroidPrivate::Denied)
            permissions.append(false);
    }
    r = QtAndroidPrivate::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE").result();
    if(r != QtAndroidPrivate::Authorized) {
        r = QtAndroidPrivate::requestPermission("android.permission.WRITE_EXTERNAL_STORAGE").result();
        if(r == QtAndroidPrivate::Denied)
            permissions.append(false);
    }
    r = QtAndroidPrivate::checkPermission("android.permission.READ_MEDIA_IMAGES").result();
    if(r != QtAndroidPrivate::Authorized) {
        r = QtAndroidPrivate::requestPermission("android.permission.READ_MEDIA_IMAGES").result();
        if(r == QtAndroidPrivate::Denied)
            permissions.append(false);
    }
    return (permissions.count() != 3);
}
#endif

#if defined(Q_OS_ANDROID)
void /*ApplicationUI::*/ accessAllFiles() {
    if(QOperatingSystemVersion::current() < QOperatingSystemVersion(QOperatingSystemVersion::Android, 11)) {
        qDebug() << "it is less then Android 11 - ALL FILES permission isn't possible!";
        return;
    }
// Here you have to set your PackageName
#define PACKAGE_NAME "package:org.ekkescorner.examples.sharex"
    jboolean value = QJniObject::callStaticMethod<jboolean>("android/os/Environment", "isExternalStorageManager");
    if(value == false) {
        qDebug() << "requesting ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION";
        QJniObject ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION = QJniObject::getStaticObjectField("android/provider/Settings", "ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION", "Ljava/lang/String;");
        QJniObject intent("android/content/Intent", "(Ljava/lang/String;)V", ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION.object());
        QJniObject jniPath = QJniObject::fromString(PACKAGE_NAME);
        QJniObject jniUri = QJniObject::callStaticObjectMethod("android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;", jniPath.object<jstring>());
        QJniObject jniResult = intent.callObjectMethod("setData", "(Landroid/net/Uri;)Landroid/content/Intent;", jniUri.object<jobject>());
        QtAndroidPrivate::startActivity(intent, 0);
    } else {
        qDebug() << "SUCCESS ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION";
    }
}
#endif

int main(int argc, char* argv[]) {
    qInstallMessageHandler(myMessageHandler);
    qSetMessagePattern(
        QLatin1String("%{if-critical}\x1b[38;2;255;0;0m"
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

    // QStringList systemEnvironment = QProcess::systemEnvironment();
    // qWarning() << requestAndroidPermissions() << *argv << systemEnvironment;
    // QAndroidJniObject::callStaticMethod<void>();
    // qCritical() << "checkPermission" << checkPermission();
    // accessAllFiles();

    Fit fit;
    engine.rootContext()->setContextProperty("fit", &fit);
    fit.loadFile(QFileDialog::getOpenFileName(nullptr, "Open File" /*, "/home", "Fit (*.fit)"*/));
    // fit.loadFile("/home/x-ray/Загрузки/bt/3 download auto/MAGENE_C206Pro_2024-04-22-19-30-21_50229574_1713806912220.fit");
    // fit.loadFile("C:/Users/bakiev/Downloads/Telegram "
    //              "Desktop/MAGENE_C206Pro_2023-08-05-09-57-40_50229574_1691267580997.fit");

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [&engine] {
            Fit* fit = engine.singletonInstance<Fit*>("FitApp", "fit");
            if(fit)
                fit->setText("77");
        },
        Qt::QueuedConnection);

    engine.loadFromModule("FitApp", "Main");
    // fit.setText("77");

    return app.exec();
}
