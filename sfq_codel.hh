#ifndef SFQ_CODEL_HH
#define SFQ_CODEL_HH

#include <string>
#include <queue>

#include "queued_packet.hh"

class SfqCoDel
{
private:
    std::queue< QueuedPacket > backing_queue_;

public:
    SfqCoDel() : backing_queue_() {};

    void enqueue( const std::string & contents );

    QueuedPacket dequeue( void );

    QueuedPacket & front( void );

    bool empty( void ) const;
};

#endif /* SFQ_CODEL_HH */ 
