#ifndef IPC_PROTOCOL_H
#define IPC_PROTOCOL_H

#include <QByteArray>
#include <QMetaType>
#include <QtGlobal>
#include <array>

namespace IpcProtocol {

static constexpr quint8 Sof0 = 0xAA;
static constexpr quint8 Sof1 = 0x55;
static constexpr quint8 Version = 0x01;
static constexpr quint8 MaxDlc = 8;
static constexpr qint32 BaudRate = 9600;

struct Frame {
    quint16 id = 0;
    quint8 dlc = 0;
    std::array<quint8, MaxDlc> data {};
};

QByteArray encode(const Frame& frame);

class FrameParser {
public:
    bool feed(quint8 byte, Frame* outputFrame);

    quint64 checksumErrors() const;

private:
    enum State {
        WaitSof0,
        WaitSof1,
        ReadVersion,
        ReadIdH,
        ReadIdL,
        ReadDlc,
        ReadData,
        ReadCrcH,
        ReadCrcL,
    };

    void reset();
    void restart(quint8 byte);

    State m_state = WaitSof0;
    quint8 m_index = 0;
    quint16 m_calculatedCrc = 0xFFFF;
    quint16 m_receivedCrc = 0;
    Frame m_frame;
    quint64 m_checksumErrors = 0;
};

} // namespace IpcProtocol

Q_DECLARE_METATYPE(IpcProtocol::Frame)

#endif
