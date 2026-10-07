#ifndef DISTANCEMETRICS_H
#define DISTANCEMETRICS_H

#include <QString>
#include <QtGlobal>

class DistanceMetrics {
public:
    double odometerKm() const { return m_odometerKm; }
    double tripKm() const { return m_tripKm; }
    int tripMinutes() const { return static_cast<int>(m_driveMs / 60000); }
    bool speedFresh(qint64 nowMs) const { return m_lastSpeedMs >= 0 && nowMs - m_lastSpeedMs <= 500; }
    void recordSpeed(int speed, qint64 nowMs);
    void update(qint64 nowMs, bool connected);
    void resetTrip();
    void load(const QString& path);
    bool save(const QString& path) const;

private:
    double m_odometerKm = 0;
    double m_tripKm = 0;
    qint64 m_driveMs = 0;
    qint64 m_lastUpdateMs = 0;
    qint64 m_lastSpeedMs = -1;
    int m_speed = 0;
};

#endif
