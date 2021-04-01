#include "UbxParser.h"

#define debug_port Serial
#define data_port Serial1

UbxParser ubx_;

int msg_class_ = 10;
int msg_id_ = 100;
const int kPayloadLength = 4;
byte payload_[kPayloadLength] = {1, 2, 3, 4};
byte msg_buffer_[100];

void setup()
{
    debug_port.begin(115200);
    while (!debug_port)
    {
    }
    data_port.begin(115200);
    while (!data_port)
        ;
    // Clear data_port buffer
    while (data_port.available())
    {
        data_port.read();
    }
}


/**
 * Examine how the parse state changes as it moves through the message buffer
 * This can be useful if your messages are not getting processed correctly
*/

void loop()
{
    // Until the first start byte is received, the expected ParseState is GOT_NONE (0)
    sendNoise();
    sendMessage(msg_class_, msg_id_);
    delay(10);
    while (data_port.available())
    {
        uint8_t b = data_port.read();
        UbxParser::ParseState parse_state = ubx_.parse(b);
        debug_port.print(F("byte/parse state: "));
        debug_port.print(b, HEX);
        debug_port.print(F("/"));
        debug_port.println(parse_state);
        if (parse_state == UbxParser::ParseState::GOT_MESSAGE)
        {
            debug_port.print("Message rx'd with class ");
            debug_port.print(ubx_.msgClass());
            debug_port.print(F(" and ID "));
            debug_port.print(ubx_.msgId());
            debug_port.print(F("\n\n"));
        }
    }
    msg_class_++;
    msg_id_ += 2;
    delay(1000);
}

void sendNoise()
{
    // Bytes that are not part of our complete message
    uint8_t buf[6] = {0,1,2,3,4,5};
    data_port.write(buf, 6);
}

void sendMessage(byte msg_class, byte msg_id)
{
    int msg_length = ubx_.buildMessage(msg_class, msg_id, kPayloadLength, payload_, msg_buffer_);
    debug_port.print(F("Send message with class "));
    debug_port.print(msg_class);
    debug_port.print(F(" and ID "));
    debug_port.println(msg_id);
    data_port.write(msg_buffer_, msg_length);
}