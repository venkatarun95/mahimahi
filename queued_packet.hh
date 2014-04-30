#ifndef QUEUED_PACKET_HH
#define QUEUED_PACKET_HH

#include <string>

struct QueuedPacket
{
    uint64_t entry_time;
    int bytes_to_transmit;
    std::string contents;

    QueuedPacket( const std::string & s_contents ) : entry_time( 0 ), bytes_to_transmit( s_contents.size() ), contents( s_contents ) {};

    static QueuedPacket nullpkt() { return QueuedPacket( "" ); };
};

#endif /* QUEUED_PACKET_HH */
