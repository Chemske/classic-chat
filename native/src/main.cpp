#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/AbstractPane>
#include "ChatController.hpp"
#include <QTextCodec>
#include <QDebug>
#include <QtDeclarative/QDeclarativeError>
#include <QtDeclarative/QtDeclarative>
#include <bb/system/SystemDialog>
#include <bb/system/SystemUiResult>
#include <bb/system/SystemUiButton>
#include <QDir>
#include <QFile>
#include <stdio.h>
using namespace bb::cascades;
namespace {
void startupMessage(QtMsgType, const char *message) {
    FILE *log = fopen("logs/classicchat-startup.log", "a");
    if (log) { fprintf(log, "%s\n", message); fclose(log); }
    fprintf(stderr, "%s\n", message);
}
}
Q_DECL_EXPORT int main(int argc, char **argv) {
    Application app(argc, argv);
    remove("logs/classicchat-startup.log");
    qInstallMsgHandler(startupMessage);
    qWarning() << "ClassicChat startup" << QDir::homePath() << QDir::currentPath();
    qmlRegisterType<bb::system::SystemDialog>("bb.system", 1, 0, "SystemDialog");
    qmlRegisterUncreatableType<bb::system::SystemUiButton>("bb.system", 1, 0, "SystemUiButton", "Provided by SystemDialog");
    qmlRegisterUncreatableType<bb::system::SystemUiResult>("bb.system", 1, 0, "SystemUiResult", "Enum values only");
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    ChatController controller;
    qWarning() << "ClassicChat controller ready";
    QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(&app);
    qWarning() << "ClassicChat document loaded" << qml->hasErrors() << qml->errors().size();
    qml->setContextProperty("_chat", &controller);
    AbstractPane *root = qml->createRootObject<AbstractPane>();
    if (!root) {
        foreach (const QDeclarativeError &error, qml->errors())
            qWarning() << "ClassicChat QML:" << error.toString();
        return 1;
    }
    app.setScene(root);
    return Application::exec();
}
