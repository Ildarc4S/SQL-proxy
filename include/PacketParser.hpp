#pragma once

#include <string>

class PacketParser {
public:
    std::string parsePacket(const char* packet, int packet_length);
private:
    bool isSQLPacket(const char* packet, int packet_length);
    static const int COMMAND_OFFSET = 5;
    static const int MIN_PACKET_LENGTH = 6; 
};