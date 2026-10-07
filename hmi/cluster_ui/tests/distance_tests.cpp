#include "DistanceMetrics.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDebug>
#include <cmath>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    DistanceMetrics metrics;
    metrics.recordSpeed(72, 0);
    metrics.update(250, true);
    metrics.update(750, true);
    const double expectedKm = 72 * 500 / 3600000.0;
    if (std::abs(metrics.odometerKm() - expectedKm) > 0.000001) return 1;
    metrics.update(900, true);
    if (std::abs(metrics.odometerKm() - expectedKm) > 0.000001) return 2;
    metrics.recordSpeed(72, 1000);
    metrics.update(1250, false);
    if (std::abs(metrics.odometerKm() - expectedKm) > 0.000001) return 3;
    metrics.recordSpeed(72, 2000);
    metrics.update(2250, true);
    const double totalKm = metrics.odometerKm();
    metrics.resetTrip();
    if (metrics.tripKm() != 0 || metrics.odometerKm() != totalKm) return 4;

    QTemporaryDir directory;
    if (!directory.isValid()) return 5;
    const QString file = directory.filePath("distance.ini");
    if (!metrics.save(file)) return 6;
    DistanceMetrics restored;
    restored.load(file);
    if (std::abs(restored.odometerKm() - totalKm) > 0.000001
        || restored.tripKm() != 0) return 7;
    restored.update(100, true);
    if (restored.odometerKm() != totalKm) return 8;

    DistanceMetrics moving;
    moving.recordSpeed(60, 0);
    for (int time = 250; time <= 60000; time += 250) {
        moving.update(time, true);
        moving.recordSpeed(60, time);
    }
    if (std::abs(moving.tripKm() - 1.0) > 0.000001
        || moving.tripMinutes() != 1) return 9;
    moving.recordSpeed(0, 60000);
    moving.update(60250, true);
    if (moving.tripMinutes() != 1) return 10;
    if (!moving.save(file)) return 11;
    restored.load(file);
    if (restored.tripMinutes() != 1
        || std::abs(restored.tripKm() - 1.0) > 0.000001) return 12;
    qInfo() << "Distance: fresh intervals, reconnect, reset, persistence and drive time passed";
    return 0;
}
