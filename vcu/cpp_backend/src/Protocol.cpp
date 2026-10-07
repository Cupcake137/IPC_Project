#include "Protocol.h"

namespace IpcProtocol {

static quint16 updateCrc(quint16 crc, quint8 value)
{
    crc ^= static_cast<quint16>(value) << 8;
    for (quint8 bit = 0; bit < 8; ++bit) {
        if ((crc & 0x8000) != 0) {
            crc = static_cast<quint16>((crc << 1) ^ 0x1021);
        } else {
            crc = static_cast<quint16>(crc << 1);
        }
    }
    return crc;
}

QByteArray encode(const Frame& frame)
{
    if (frame.id > 0x7FF || frame.dlc > MaxDlc) {
        return QByteArray();
    }

    QByteArray bytes;
    bytes.reserve(8 + frame.dlc);
    bytes.append(static_cast<char>(Sof0));
    bytes.append(static_cast<char>(Sof1));
    bytes.append(static_cast<char>(Version));
    bytes.append(static_cast<char>(frame.id >> 8));
    bytes.append(static_cast<char>(frame.id & 0xFF));
    bytes.append(static_cast<char>(frame.dlc));

    quint16 crc = 0xFFFF;
    for (int index = 2; index < bytes.size(); ++index) {
        crc = updateCrc(crc, static_cast<quint8>(bytes[index]));
    }
    for (quint8 index = 0; index < frame.dlc; ++index) {
        bytes.append(static_cast<char>(frame.data[index]));
        crc = updateCrc(crc, frame.data[index]);
    }
    bytes.append(static_cast<char>(crc >> 8));
    bytes.append(static_cast<char>(crc & 0xFF));
    return bytes;
}

bool FrameParser::feed(quint8 byte, Frame* outputFrame)
{
    if (outputFrame == nullptr) {
        return false;
    }

    switch (m_state) {
    case WaitSof0:
        if (byte == Sof0) {
            m_state = WaitSof1;
        }
        break;

    case WaitSof1:
        if (byte == Sof1) {
            m_state = ReadVersion;
            m_calculatedCrc = 0xFFFF;
        } else {
            restart(byte);
        }
        break;

    case ReadVersion:
        if (byte != Version) {
            restart(byte);
            break;
        }
        m_calculatedCrc = updateCrc(m_calculatedCrc, byte);
        m_state = ReadIdH;
        break;

    case ReadIdH:
        m_frame.id = static_cast<quint16>(byte) << 8;
        m_calculatedCrc = updateCrc(m_calculatedCrc, byte);
        m_state = ReadIdL;
        break;

    case ReadIdL:
        m_frame.id |= byte;
        m_calculatedCrc = updateCrc(m_calculatedCrc, byte);
        if (m_frame.id > 0x7FF) {
            restart(byte);
        } else {
            m_state = ReadDlc;
        }
        break;

    case ReadDlc:
        if (byte > MaxDlc) {
            restart(byte);
            break;
        }
        m_frame.dlc = byte;
        m_frame.data.fill(0);
        m_index = 0;
        m_calculatedCrc = updateCrc(m_calculatedCrc, byte);
        if (m_frame.dlc == 0) {
            m_state = ReadCrcH;
        } else {
            m_state = ReadData;
        }
        break;

    case ReadData:
        m_frame.data[m_index] = byte;
        ++m_index;
        m_calculatedCrc = updateCrc(m_calculatedCrc, byte);
        if (m_index >= m_frame.dlc) {
            m_state = ReadCrcH;
        }
        break;

    case ReadCrcH:
        m_receivedCrc = static_cast<quint16>(byte) << 8;
        m_state = ReadCrcL;
        break;

    case ReadCrcL:
        m_receivedCrc |= byte;
        if (m_receivedCrc == m_calculatedCrc) {
            *outputFrame = m_frame;
            reset();
            return true;
        }
        ++m_checksumErrors;
        restart(byte);
        break;
    }

    return false;
}

quint64 FrameParser::checksumErrors() const
{
    return m_checksumErrors;
}

void FrameParser::reset()
{
    m_state = WaitSof0;
    m_index = 0;
    m_calculatedCrc = 0xFFFF;
    m_receivedCrc = 0;
    m_frame = Frame();
}

void FrameParser::restart(quint8 byte)
{
    reset();
    if (byte == Sof0) {
        m_state = WaitSof1;
    }
}

} // namespace IpcProtocol
