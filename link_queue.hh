/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef LINK_QUEUE_HH
#define LINK_QUEUE_HH

#include <cstdint>
#include <string>

#include "sfq_codel.hh"
#include "file_descriptor.hh"

class LinkQueue
{
private:
    const static unsigned int PACKET_SIZE = 1504; /* default max TUN payload size */

    unsigned int next_delivery_;
    std::vector< uint64_t > schedule_;
    uint64_t base_timestamp_;

    SfqCoDel packet_queue_;

    uint64_t next_delivery_time( void ) const;

    void use_a_delivery_opportunity( void );

public:
    LinkQueue( const std::string & filename );

    void read_packet( const std::string & contents );

    void write_packets( FileDescriptor & fd );

    unsigned int wait_time( void ) const;
};

#endif /* LINK_QUEUE_HH */
