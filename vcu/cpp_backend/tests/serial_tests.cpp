#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QThread>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include "CanDatabase.h"
#include "SerialManager.h"
#include "VCUStateMachine.h"

enum Traffic { FreshDrive, DuplicateDrive, BadPayload, EnergyOnly, UnknownId, Silence };

static bool runScenario(Traffic traffic)
{
    // A pseudo-terminal exercises the real serial parser without a motor.
    const int terminal = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (terminal < 0) return false;
    if (grantpt(terminal) != 0 || unlockpt(terminal) != 0) {
        close(terminal);
        return false;
    }

    VehicleModel model;
    DTCManager dtc;
    VCUStateMachine state(model, dtc);
    SerialManager serial;
    QObject::connect(&serial, &SerialManager::transportStateChanged,
                     [&](bool open) { state.setTransportOpen(open); });
    QObject::connect(&serial, &SerialManager::communicationStateChanged, [&](bool healthy) {
        state.setCommunicationHealthy(healthy);
        serial.sendMotorCommand(model.authorizedPwm(), model.gear());
    });
    QObject::connect(&serial, &SerialManager::frameReceived, [&](IpcProtocol::Frame frame) {
        state.processFrame(frame);
        serial.sendMotorCommand(model.authorizedPwm(), model.gear());
    });

    if (!serial.open(QString::fromLocal8Bit(ptsname(terminal)))) {
        close(terminal);
        return false;
    }
    CanDatabase::DriveStatus drive;
    drive.pedal = 50;
    drive.speed = 60;
    drive.gear = CanDatabase::Drive;
    IpcProtocol::Frame frame;
    CanDatabase::packDriveStatus(drive, &frame);
    const QByteArray initial = IpcProtocol::encode(frame);
    bool passed = write(terminal, initial.constData(), initial.size()) == initial.size();
    bool sawRunning = false;
    bool sawLateCommand = false;
    IpcProtocol::FrameParser outputParser;
    QElapsedTimer clock;
    clock.start();
    qint64 lastSend = 0;
    while (clock.elapsed() < 1200) {
        QCoreApplication::processEvents();
        const qint64 now = clock.elapsed();
        sawRunning |= model.authorizedPwm() == 127;
        if (now - lastSend >= 80 && traffic != Silence) {
            lastSend = now;
            if (traffic == EnergyOnly) {
                CanDatabase::EnergyStatus energy;
                energy.soc = 80;
                energy.aliveCounter = (++drive.aliveCounter) & 0x0F;
                CanDatabase::packEnergyStatus(energy, &frame);
            } else {
                if (traffic == FreshDrive || traffic == BadPayload)
                    drive.aliveCounter = (drive.aliveCounter + 1) & 0x0F;
                CanDatabase::packDriveStatus(drive, &frame);
                if (traffic == BadPayload) frame.data[0] ^= 1;
                if (traffic == UnknownId) frame.id = 0x777;
            }
            const QByteArray encoded = IpcProtocol::encode(frame);
            passed &= write(terminal, encoded.constData(), encoded.size()) == encoded.size();
        }
        char output[4096];
        ssize_t count;
        while ((count = read(terminal, output, sizeof(output))) > 0) {
            for (ssize_t i = 0; i < count; ++i) {
                IpcProtocol::Frame commandFrame;
                if (!outputParser.feed(static_cast<quint8>(output[i]), &commandFrame)) continue;
                CanDatabase::MotorCommand command;
                if (now >= 800 && CanDatabase::unpackMotorCommand(commandFrame, &command)) {
                    sawLateCommand = true;
                    passed &= command.pwm == (traffic == FreshDrive ? 127 : 0);
                }
            }
        }
        QThread::msleep(5);
    }
    passed &= sawRunning && sawLateCommand;
    passed &= traffic == FreshDrive
        ? model.state() == VehicleModel::Ready && model.authorizedPwm() == 127
        : model.state() == VehicleModel::Fault && model.authorizedPwm() == 0
          && model.activeDtc() == 0xE1;
    serial.close();
    close(terminal);
    qInfo() << (passed ? "PASS" : "FAIL") << "UART scenario" << traffic;
    return passed;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    bool passed = true;
    for (Traffic traffic : {FreshDrive, DuplicateDrive, BadPayload, EnergyOnly, UnknownId, Silence})
        passed &= runScenario(traffic);
    return passed ? 0 : 1;
}
