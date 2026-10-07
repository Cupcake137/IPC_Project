#include "DistanceMetrics.h"

#include <QSettings>
#include <cmath>

void DistanceMetrics::recordSpeed(int speed, qint64 nowMs)
{
    m_speed = qBound(0, speed, 255);
    m_lastSpeedMs = nowMs;
    // Do not count the gap before a first or restored speed sample.
    m_lastUpdateMs = nowMs;
}

void DistanceMetrics::update(qint64 nowMs, bool connected)
{
    const qint64 previousMs = m_lastUpdateMs;
    m_lastUpdateMs = nowMs;
    if (!connected || m_lastSpeedMs < 0 || nowMs <= previousMs
        || nowMs - previousMs > 1000 || m_speed == 0) {
        return;
    }
    // Integrate only the part of this interval covered by fresh telemetry.
    const qint64 validEndMs = qMin(nowMs, m_lastSpeedMs + 500);
    const qint64 elapsedMs = qMax(qint64(0), validEndMs - previousMs);
    const double distance = m_speed * static_cast<double>(elapsedMs) / 3600000.0;
    m_odometerKm += distance;
    m_tripKm += distance;
    m_driveMs += elapsedMs;
}

void DistanceMetrics::resetTrip()
{
    m_tripKm = 0;
    m_driveMs = 0;
}

void DistanceMetrics::load(const QString& path)
{
    QSettings settings(path, QSettings::IniFormat);
    const double odo = settings.value("distance/odometerKm", 0).toDouble();
    const double trip = settings.value("distance/tripKm", 0).toDouble();
    const qint64 driveMs = settings.value("distance/driveMs", 0).toLongLong();
    m_odometerKm = std::isfinite(odo) && odo >= 0 ? odo : 0;
    m_tripKm = std::isfinite(trip) && trip >= 0 && trip <= m_odometerKm ? trip : 0;
    m_driveMs = driveMs >= 0 ? driveMs : 0;
}

bool DistanceMetrics::save(const QString& path) const
{
    QSettings settings(path, QSettings::IniFormat);
    settings.setValue("distance/odometerKm", m_odometerKm);
    settings.setValue("distance/tripKm", m_tripKm);
    settings.setValue("distance/driveMs", m_driveMs);
    settings.sync();
    return settings.status() == QSettings::NoError;
}
