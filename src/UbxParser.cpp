#include "UbxParser.h"

UbxParser::UbxParser()
{
    state_ = GOT_NONE;
    msgclass_ = -1;
    msgid_ = -1;
    msglen_ = -1;
    chka_ = 0;
    chkb_ = 0;
    count_ = 0;
}

bool UbxParser::read(Stream *port)
{
    while (port->available())
    {
        if (parse(port->read()))
        {
            return true;
        }
    }
    return false;
}

bool UbxParser::parse(uint8_t b)
{
    switch (state_)
    {
    case GOT_NONE:
        if (b == START_BYTE_1)
        {
            state_ = GOT_START_BYTE1;
        }
        break;
    case GOT_START_BYTE1:
        if (b == START_BYTE_2)
        {
            state_ = GOT_START_BYTE2;
            chka_ = 0;
            chkb_ = 0;
        }
        else
        {
            // DebugPrintln("bad b2");
            state_ = GOT_NONE;
        }
        break;
    case GOT_START_BYTE2:
        msgclass_ = b;
        state_ = GOT_CLASS;
        addToChecksum(b);
        break;
    case GOT_CLASS:
        msgid_ = b;
        state_ = GOT_ID;
        addToChecksum(b);
        break;
    case GOT_ID:
        state_ = GOT_LENGTH1;
        msglen_ = b;
        addToChecksum(b);
        break;
    case GOT_LENGTH1:
        msglen_ += (b << 8);
        if (msglen_ < kMessageLengthMax)
        {
            state_ = GOT_LENGTH2;
            count_ = 0;
            addToChecksum(b);
        }
        else
        {
            state_ = GOT_NONE;
            // DebugPrintln("Msg length too big");
        }
        break;
    case GOT_LENGTH2:
        if (count_ < kPayloadSize)
        {
            addToChecksum(b);
            payload_[count_] = b;
            count_++;

            if (count_ == msglen_)
            {
                state_ = GOT_PAYLOAD;
            }
        }
        else
        {
            // DebugPrintln("overrun");
            state_ = GOT_NONE;
        }
        break;
    case GOT_PAYLOAD:
        if (b == chka_)
        {
            state_ = GOT_CHKA;
            // DebugPrintln("good chka");
        }
        else
        {
        state_:
            GOT_NONE;
            // DebugPrintln("bad chka. exp/rx: " + String(chka_) + "/" + String(b));
        }
        break;
    case GOT_CHKA:
        state_ = GOT_NONE;
        if (b == chkb_)
        {
            // DebugPrintln("good msg");
            return true;
        }
        else
        {
            // DebugPrintln("bad chkb. exp/rx: " + String(chkb_) + "/" + String(b));
        }
        break;
    default:
        // DebugPrintln("unk :" + String(b) + "/" + String(state));
        break;
    }
    return false;
}

void UbxParser::addToChecksum(int b)
{
    chka_ = (chka_ + b) & 0xFF;
    chkb_ = (chkb_ + chka_) & 0xFF;
}

uint32_t UbxParser::unpackUint32(int offset)
{
    return (uint32_t)unpack(offset, 4);
}

int32_t UbxParser::unpackInt32(int offset)
{

    return unpack(offset, 4);
}

uint16_t UbxParser::unpackUint16(int offset)
{
    return (uint16_t)unpack(offset, 2);
}

int16_t UbxParser::unpackInt16(int offset)
{
    return (int16_t)unpack(offset, 2);
}

uint8_t UbxParser::unpackUint8(int offset)
{
    return (uint8_t)unpack(offset, 1);
}

int8_t UbxParser::unpackInt8(int offset)
{
    return (int8_t)unpack(offset, 1);
}

int32_t UbxParser::unpack(int offset, int size)
{
    long value = 0; // four bytes on most Arduinos

    for (int k = 0; k < size; ++k)
    {
        value <<= 8;
        value |= (0xFF & payload_[offset + (size - 1) - k]);
    }

    return value;
}

int UbxParser::buildMessage(int msg_class, int msg_id, int payload_length, uint8_t payload[], uint8_t msg_buffer[])
{
    int index = 0;
    msg_buffer[index++] = START_BYTE_1;
    msg_buffer[index++] = START_BYTE_2;
    msg_buffer[index++] = msg_class;
    msg_buffer[index++] = msg_id;
    msg_buffer[index++] = payload_length & 0xFF;
    msg_buffer[index++] = (payload_length >> 8) & 0xFF;
    memcpy(&msg_buffer[index], &payload[0], payload_length);
    index += payload_length;
    uint8_t chka, chkb;
    calculateChecksum(&msg_buffer[2], payload_length+4, chka, chkb);
    msg_buffer[index++] = chka;
    msg_buffer[index] = chkb;

    return payload_length + 8;
}

void UbxParser::calculateChecksum(uint8_t *payload, int payload_length, uint8_t &chka, uint8_t &chkb)
{
    chka = 0;
    chkb = 0;
    for (int i = 0; i < payload_length; ++i)
    {
        chka = (chka + payload[i]) & 0xFF;
        chkb = (chkb + chka) & 0xFF;
    }
}

void UbxParser::printBuffer(uint8_t msg_buffer[], int msg_length, Stream *port, int output_type)
{
    int i = 0;
    for (; i < msg_length-1; i++)
    {
        port->print(msg_buffer[i], output_type);
        port->print(F(","));
    }
    port->println(msg_buffer[i]);
}

uint8_t UbxParser::msgClass()
{
    return msgclass_;
}

uint8_t UbxParser::msgId()
{
    return msgid_;
}