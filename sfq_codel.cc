#include <cassert>

#include "sfq_codel.hh"

QueuedPacket SfqCoDel::dequeue( void )
{
    assert( not backing_queue_.empty() );
    auto ret = backing_queue_.front();
    backing_queue_.pop();
    return ret;
}

void SfqCoDel::enqueue( const std::string & contents )
{
    backing_queue_.emplace( contents );
}

bool SfqCoDel::empty( void ) const
{
    return backing_queue_.empty();
}

QueuedPacket & SfqCoDel::front( void )
{
    return backing_queue_.front();
}
