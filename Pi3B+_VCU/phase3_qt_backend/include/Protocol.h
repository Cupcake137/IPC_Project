#ifndef IPC_PROTOCOL_H
#define IPC_PROTOCOL_H

#include <QByteArray>
#include <QMetaType>
#include <QtGlobal>
#include <array>
#include <optional>

namespace IpcProtocol {

static constexpr quint8 Sof0 = 0xAA;
static constexpr quint8 Sof1 = 0x55;
static constexpr quint8 MaxDlc = 8;
static constexpr qint32 BaudRate = 9600;

enum MessageId : quint16 {
    MotorCommand = 0x0100,
    PedalSpeed   = 0x0101,
    BatterySoc   = 0x0201,
    GearState    = 0x0401,
    DtcStatus    = 0x0501,
};

struct Frame {
    quint16 id = 0;
    quint8 dlc = 0;
    std::array<quint8, MaxDlc> data {};
    quint8 counter = 0;
};

inline quint8 checksum(quint16 id, quint8 dlc, quint8 counter, const std::array<quint8, MaxDlc>& data)
{
    quint16 sum = 0;
    sum += static_cast<quint8>(id >> 8);
    sum += static_cast<quint8>(id & 0xFF);
    sum += dlc;
    sum += counter;
    for (quint8 i = 0; i < dlc; ++i) {
        sum += data[i];
    }
    return static_cast<quint8>(sum & 0xFF);
}

inline QByteArray encode(quint16 id, quint8 dlc, const std::array<quint8, MaxDlc>& data, quint8& txCounter)
{
    if (dlc > MaxDlc) {
        return {};
    }

    const quint8 counter = txCounter & 0x0F;
    QByteArray out;
    out.reserve(7 + dlc);
    out.append(static_cast<char>(Sof0));
    out.append(static_cast<char>(Sof1));
    out.append(static_cast<char>(id >> 8));
    out.append(static_cast<char>(id & 0xFF));
    out.append(static_cast<char>(dlc));
    out.append(static_cast<char>(counter));
    for (quint8 i = 0; i < dlc; ++i) {
        out.append(static_cast<char>(data[i]));
    }
    out.append(static_cast<char>(checksum(id, dlc, counter, data)));
    txCounter = static_cast<quint8>((counter + 1) & 0x0F);
    return out;
}

class FrameParser {
public:
    std::optional<Frame> feed(quint8 byte)
    {
        switch (m_state) {
        case WaitSof0:
            if (byte == Sof0) {
                m_state = WaitSof1;
            }
            break;
        case WaitSof1:
            if (byte == Sof1) {
                m_state = ReadIdH;
                m_checksum = 0;
            } else {
                restart(byte);
            }
            break;
        case ReadIdH:
            m_frame.id = static_cast<quint16>(byte) << 8;
            m_checksum += byte;
            m_state = ReadIdL;
            break;
        case ReadIdL:
            m_frame.id |= byte;
            m_checksum += byte;
            m_state = ReadDlc;
            break;
        case ReadDlc:
            if (byte > MaxDlc) {
                ++m_formatErrors;
                restart(byte);
                break;
            }
            m_frame.dlc = byte;
            m_frame.data.fill(0);
            m_index = 0;
            m_checksum += byte;
            m_state = ReadCounter;
            break;
        case ReadCounter:
            m_frame.counter = byte;
            m_checksum += byte;
            m_state = (m_frame.dlc == 0) ? ReadChecksum : ReadData;
            break;
        case ReadData:
            m_frame.data[m_index++] = byte;
            m_checksum += byte;
            if (m_index >= m_frame.dlc) {
                m_state = ReadChecksum;
            }
            break;
        case ReadChecksum:
            if (m_checksum == byte) {
                Frame parsed = m_frame;
                ++m_validFrames;
                reset();
                return parsed;
            }
            ++m_checksumErrors;
            restart(byte);
            break;
        }
        return std::nullopt;
    }

    quint64 validFrames() const { return m_validFrames; }
    quint64 checksumErrors() const { return m_checksumErrors; }
    quint64 formatErrors() const { return m_formatErrors; }

private:
    enum State {
        WaitSof0,
        WaitSof1,
        ReadIdH,
        ReadIdL,
        ReadDlc,
        ReadCounter,
        ReadData,
        ReadChecksum,
    };

    void reset()
    {
        m_state = WaitSof0;
        m_index = 0;
        m_checksum = 0;
        m_frame = {};
    }

    void restart(quint8 byte)
    {
        reset();
        if (byte == Sof0) {
            m_state = WaitSof1;
        }
    }

    State m_state = WaitSof0;
    quint8 m_index = 0;
    quint8 m_checksum = 0;
    Frame m_frame;
    quint64 m_validFrames = 0;
    quint64 m_checksumErrors = 0;
    quint64 m_formatErrors = 0;
};

} // namespace IpcProtocol

Q_DECLARE_METATYPE(IpcProtocol::Frame)

#endif
