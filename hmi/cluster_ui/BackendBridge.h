#ifndef BACKENDBRIDGE_H
#define BACKENDBRIDGE_H

#include <QObject>
#include <QElapsedTimer>
#include <QString>

#include "DTCManager.h"
#include "DistanceMetrics.h"
#include "SerialManager.h"
#include "SimulationManager.h"
#include "SteeringInputManager.h"
#include "VCUStateMachine.h"
#include "VehicleModel.h"

class BackendBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(int speed READ speed NOTIFY dataChanged)
    Q_PROPERTY(int soc READ soc NOTIFY dataChanged)
    Q_PROPERTY(int motorOutput READ motorOutput NOTIFY dataChanged)
    Q_PROPERTY(int dteKm READ dteKm NOTIFY dataChanged)
    Q_PROPERTY(double tripKm READ tripKm NOTIFY dataChanged)
    Q_PROPERTY(double odometerKm READ odometerKm NOTIFY dataChanged)
    Q_PROPERTY(int tripMinutes READ tripMinutes NOTIFY dataChanged)
    Q_PROPERTY(int regenLevel READ regenLevel NOTIFY dataChanged)
    Q_PROPERTY(QString gear READ gear NOTIFY dataChanged)
    Q_PROPERTY(int driveMode READ driveMode NOTIFY dataChanged)
    Q_PROPERTY(QString driveModeName READ driveModeName NOTIFY dataChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY dataChanged)
    Q_PROPERTY(int warningLevel READ warningLevel NOTIFY dataChanged)
    Q_PROPERTY(QString warningText READ warningText NOTIFY dataChanged)
    Q_PROPERTY(bool keypadOnline READ keypadOnline NOTIFY keypadOnlineChanged)
    Q_PROPERTY(int brightness READ brightness NOTIFY brightnessChanged)
    Q_PROPERTY(bool positionLightOn READ positionLightOn NOTIFY controlsChanged)
    Q_PROPERTY(bool lowBeamOn READ lowBeamOn NOTIFY controlsChanged)
    Q_PROPERTY(bool highBeamOn READ highBeamOn NOTIFY controlsChanged)
    Q_PROPERTY(bool fogLightOn READ fogLightOn NOTIFY controlsChanged)
    Q_PROPERTY(bool leftSignalOn READ leftSignalOn NOTIFY controlsChanged)
    Q_PROPERTY(bool rightSignalOn READ rightSignalOn NOTIFY controlsChanged)
    Q_PROPERTY(bool hazardOn READ hazardOn NOTIFY controlsChanged)

public:
    explicit BackendBridge(QObject* parent = nullptr);
    ~BackendBridge() override;

    int speed() const { return m_model.speed(); }
    int soc() const { return m_model.soc(); }
    int motorOutput() const;
    int dteKm() const { return m_dteKm; }
    double tripKm() const { return m_distance.tripKm(); }
    double odometerKm() const { return m_distance.odometerKm(); }
    int tripMinutes() const { return m_distance.tripMinutes(); }
    int regenLevel() const { return m_regenLevel; }
    QString gear() const;
    int driveMode() const { return m_model.driveMode(); }
    QString driveModeName() const;
    bool ready() const;
    int warningLevel() const;
    QString warningText() const;
    bool keypadOnline() const { return m_keypadOnline; }
    int brightness() const { return m_brightness; }
    bool positionLightOn() const { return m_lightStage >= 1; }
    bool lowBeamOn() const { return m_lightStage >= 2; }
    bool highBeamOn() const { return m_highBeamOn; }
    bool fogLightOn() const { return m_fogLightOn; }
    bool leftSignalOn() const { return m_leftSignalOn; }
    bool rightSignalOn() const { return m_rightSignalOn; }
    bool hazardOn() const { return m_hazardOn; }

    void start(bool simulationMode, const QString& serialPort, bool mqttEnabled);

    Q_INVOKABLE void setDriveMode(int mode);
    Q_INVOKABLE void setRegenLevel(int level);
    Q_INVOKABLE void changeBrightness(int amount);
    Q_INVOKABLE void resetTrip();
    Q_INVOKABLE void acknowledgeWarning();
    Q_INVOKABLE void cycleLights();
    Q_INVOKABLE void toggleHighBeam();
    Q_INVOKABLE void toggleFogLight();
    Q_INVOKABLE void toggleLeftSignal();
    Q_INVOKABLE void toggleRightSignal();
    Q_INVOKABLE void toggleHazard();

signals:
    void dataChanged();
    void keypadOnlineChanged();
    void brightnessChanged();
    void controlsChanged();
    void uiAction(QString action);

private slots:
    void handleKey(int eventType, const QString& key, int aliveCounter);
    void setKeypadOnline(bool online);

private:
    void startMqtt();
    void startSimulation();
    void startHardware(const QString& serialPort);
    void updateCalculatedValues();
    void processVehicleFrame(IpcProtocol::Frame frame);
    void saveDistance();

    VehicleModel m_model;
    DTCManager m_dtcManager;
    SerialManager m_serial;
    SimulationManager m_simulation;
    SteeringInputManager m_steeringInput;
    VCUStateMachine m_stateMachine;
    bool m_keypadOnline = false;
    int m_brightness = 100;
    int m_lightStage = 2;
    bool m_highBeamOn = false;
    bool m_fogLightOn = false;
    bool m_leftSignalOn = false;
    bool m_rightSignalOn = false;
    bool m_hazardOn = false;
    int m_dteKm = 385;
    int m_regenLevel = 1;
    DistanceMetrics m_distance;
    QString m_distancePath;
    bool m_distanceConnected = false;
    QElapsedTimer m_tripClock;
};

#endif
