#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QDebug>
#include <memory>

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    QQmlEngine engine;
    QQmlComponent fakeVehicle(&engine);
    fakeVehicle.setData(R"(
        import QtQuick 2.15
        QtObject {
            property int regenLevel: 1
            property int brightness: 100
            property int resets: 0
            function setRegenLevel(level) { regenLevel = level }
            function changeBrightness(amount) { brightness += amount }
            function resetTrip() { resets++ }
        })", QUrl());
    std::unique_ptr<QObject> vehicle(fakeVehicle.create());
    QQmlComponent component(&engine, QUrl::fromLocalFile(
        QStringLiteral(IPC_QML_DIR "/MenuController.qml")));
    std::unique_ptr<QObject> menu(component.create());
    if (!vehicle || !menu) {
        qCritical() << component.errors() << fakeVehicle.errors();
        return 1;
    }
    menu->setProperty("vehicle", QVariant::fromValue(vehicle.get()));
    auto press = [&](const char* action) {
        return QMetaObject::invokeMethod(menu.get(), "handleAction",
            Q_ARG(QVariant, QVariant(QString::fromLatin1(action))));
    };
    if (menu->property("active").toBool()) return 2;
    if (!press("NAV_UP") || !press("NAV_RIGHT") || !press("SELECT")) return 3;
    if (vehicle->property("regenLevel").toInt() != 1
        || menu->property("currentPage").toInt() != 0) return 4;
    press("OPEN_MENU");
    press("NAV_UP");
    if (!menu->property("active").toBool()
        || vehicle->property("regenLevel").toInt() != 2) return 5;
    press("NAV_LEFT");
    press("NAV_DOWN");
    if (menu->property("currentPage").toInt() != 4
        || vehicle->property("brightness").toInt() != 90) return 6;
    press("BACK");
    press("NAV_DOWN");
    if (menu->property("active").toBool()
        || vehicle->property("brightness").toInt() != 90) return 7;
    press("HOME");
    if (menu->property("currentPage").toInt() != 0) return 8;
    press("OPEN_MENU");
    press("NAV_RIGHT");
    press("NAV_RIGHT");
    press("SELECT");
    if (vehicle->property("resets").toInt() != 1
        || menu->property("active").toBool()) return 9;
    press("SELECT");
    if (vehicle->property("resets").toInt() != 1) return 10;
    press("OPEN_MENU");
    press("OPEN_MENU");
    if (menu->property("active").toBool()) return 11;
    if (!QMetaObject::invokeMethod(menu.get(), "openPage", Q_ARG(QVariant, QVariant(3)))) return 12;
    if (!menu->property("active").toBool()
        || menu->property("currentPage").toInt() != 3) return 13;
    qInfo() << "Menu: open/back/home, navigation, closed-input guards and page selection passed";
    return 0;
}
