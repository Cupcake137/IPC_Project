#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include "BackendBridge.h"
#include "radialbar.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("IPCProject"));
    QCoreApplication::setApplicationName(QStringLiteral("IPCCluster"));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({{"s", "simulate"}, "Run without Uno hardware."});
    parser.addOption({"no-mqtt", "Disable the ESP32 keypad."});
    parser.addOption({"screenshot", "Save a UI screenshot and exit.", "file"});
    parser.addOption({"screen", "Open a screen for UI testing.", "name", "drive"});
    parser.addOption({"screenshot-delay", "Screenshot delay in milliseconds.", "ms", "4000"});
    parser.addPositionalArgument("serial-port", "Uno UART device", "[/dev/ttyUSB0]");
    parser.process(app);

    const QStringList arguments = parser.positionalArguments();
    const QString serialPort = arguments.isEmpty()
        ? QStringLiteral("/dev/ttyUSB0") : arguments.first();

    BackendBridge backend;
    backend.start(parser.isSet("simulate"), serialPort, !parser.isSet("no-mqtt"));

    qmlRegisterType<RadialBar>("CustomControls", 1, 0, "RadialBar");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
    const QString screen = parser.value("screen").toLower();
    const QStringList screens = {
        QStringLiteral("drive"), QStringLiteral("energy"),
        QStringLiteral("trip"), QStringLiteral("warnings"),
        QStringLiteral("display")
    };
    engine.rootContext()->setContextProperty(
        QStringLiteral("startPage"), qMax(0, screens.indexOf(screen)));
    engine.rootContext()->setContextProperty(
        QStringLiteral("startMenuOpen"), screen != QStringLiteral("drive"));
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    if (parser.isSet("screenshot")) {
        const int delayMs = parser.value("screenshot-delay").toInt();
        QTimer::singleShot(delayMs, [&app, &engine, &parser]() {
            QQuickWindow* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            if (window) {
                window->grabWindow().save(parser.value("screenshot"));
            }
            app.quit();
        });
    }

    return app.exec();
}
