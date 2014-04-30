#ifndef QUEUED_PACKET_HH
#define QUEUED_PACKET_HH

#include <string>

struct QueuedPacket
{
    int bytes_to_transmit;
    std::string contents;

    QueuedPacket( const std::string & s_contents ) : bytes_to_transmit( s_contents.size() ), contents( s_contents ) {};
};

#endif /* QUEUED_PACKET_HH */ 
