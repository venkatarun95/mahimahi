#ifndef SFQ_CODEL_HH
#define SFQ_CODEL_HH

#include <cmath>
#include <string>
#include <queue>
#include <map>

#include "queued_packet.hh"

class SfqCoDel
{
private:
    /* Data structures */
    typedef struct {
         QueuedPacket p;
         bool ok_to_drop;
    } dodeque_result;

    /* Hash from packet to flow */
    uint64_t hash( const std::string & contents ) const;

    /* sfq - Initialization */
    void init_bin( const uint64_t & bin_id );

    /* sfq - purging */
    void remove_bin( const uint64_t & bin_id );

    /* sfq - next flow in cyclic order */
    uint64_t next_bin( void );

    /* sfq - helper functions */
    void enqueue_packet( const QueuedPacket & p, const uint64_t & bin_id );

    /* Codel - control rule */
    uint64_t control_law( const uint64_t & t, const uint32_t & count ) const { return t + interval/sqrt( count );}

    /* Main codel routines */
    QueuedPacket codel_deq_helper( const uint64_t & bin_id );
    dodeque_result codel_dodeque( const uint64_t & bin_id );
    QueuedPacket codel_deque( const uint64_t & bin_id );
    void drop( const QueuedPacket & p, const uint64_t & bin_id );

    /* MTU Size */
    static const uint32_t MTU_SIZE = 1514;

    /* sfqCoDel structures */
    std::map<uint64_t, std::queue<QueuedPacket>> pkt_queues_;
    std::map<uint64_t, int32_t>  bin_credits_;
    std::map<uint64_t, uint32_t> bin_quantums_;
    std::queue<uint64_t> active_list_;
    std::map<uint64_t, bool> active_indicator_;

    /* Codel - specific parameters */
    static const uint64_t    target = 5 ;    /* 5   ms as per the spec */
    static const uint64_t  interval = 100;   /* 100 ms as per the spec */

    /* Codel - specific tracker variables */
    std::map<uint64_t, uint64_t> first_above_time_;
    std::map<uint64_t, uint64_t> drop_next_;
    std::map<uint64_t, uint32_t> count_;
    std::map<uint64_t, bool    > dropping_;
    std::map<uint64_t, uint32_t> drop_count_; /* for statistics */
    uint32_t bytes_in_queue_; /* Total number of bytes across all queues */

public:
    SfqCoDel(); 

    void enqueue( const std::string & contents );

    QueuedPacket dequeue( void );

    QueuedPacket & front( void );

    bool empty( void ) const;
};

#endif /* SFQ_CODEL_HH */ 
