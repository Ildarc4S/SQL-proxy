#include "PacketParser.hpp"

std::string PacketParser::parsePacket(const char* packet, int packet_length) {
    if (isSQLPacket(packet, packet_length)) {
        std::string query(packet + COMMAND_OFFSET, packet_length - MIN_PACKET_LENGTH);
        query.erase(std::remove(query.begin(), query.end(), '\n'), query.end());
        query += "\n";
        return query;
    } else {
        return "";
    }
}
bool PacketParser::isSQLPacket(const char* packet, int packet_length) {
    if (packet == nullptr || packet_length <= 0) {
        return false;
    }
    return (packet_length > MIN_PACKET_LENGTH && packet[0] == 'Q');
}
