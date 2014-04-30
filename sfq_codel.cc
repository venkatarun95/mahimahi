#include <cassert>

#include "timestamp.hh"
#include "pkt_classifier.hh"
#include "sfq_codel.hh"

SfqCoDel::SfqCoDel()
    : pkt_queues_(),
      bin_credits_(),
      bin_quantums_(),
      active_list_(),
      active_indicator_(),
      first_above_time_(),
      drop_next_(),
      count_(),
      dropping_(),
      drop_count_(),
      bytes_in_queue_( 0 )
{}

void SfqCoDel::enqueue(const std::string & contents) {
  uint64_t bin_id = hash( contents );
  QueuedPacket p( contents );
  p.entry_time = timestamp();
  enqueue_packet( p, bin_id );
  if (active_indicator_.at( bin_id ) == false) {
    active_indicator_.at( bin_id ) = true;
    active_list_.push( bin_id );
    bin_credits_.at( bin_id ) = 0;
  }
}

QueuedPacket & SfqCoDel::front( void ) {
  uint64_t to_sched = next_bin();
  assert ( to_sched != (uint64_t) -1 );
  assert ( not pkt_queues_.at( to_sched ).empty() );
  return pkt_queues_.at( to_sched ).front();
}

QueuedPacket SfqCoDel::dequeue() {
  QueuedPacket p = QueuedPacket::nullpkt(); /* TODO Check if this is ok */
  uint64_t current_bin = 0;
  /*
     Try dequeuing a until you get a non-null pkt,
     or the queues are drained 
  */
  do {
    if ( active_list_.empty() ) {
      /* No packets in any bin */
      return QueuedPacket::nullpkt();
    }
    current_bin = next_bin();
    p = codel_deque( current_bin );
  } while ( p.contents.size() == 0 );

  /* Decrement credits for the bin */
  bin_credits_.at( current_bin ) -= p.contents.size();
  return p;
}

bool SfqCoDel::empty( void ) const {
  for ( auto it = pkt_queues_.begin(); it != pkt_queues_.end(); it++ ) {
    if ( not it->second.empty() ) {
      return false;
    }
  }
  return true;
}

uint64_t SfqCoDel::hash( const std::string & contents ) const {
  return PktClassifier::get_flow_id( contents );
}

void SfqCoDel::init_bin( const uint64_t & bin_id ) {
  /* Initialize bin */
  pkt_queues_[ bin_id ] = std::queue<QueuedPacket>();
  bin_credits_[ bin_id ] = 0;
  bin_quantums_[ bin_id ] = MTU_SIZE;
  active_indicator_[ bin_id ] = false;
  first_above_time_[ bin_id ] = 0;
  drop_next_[ bin_id ] = 0;
  count_[ bin_id ] = 0;
  dropping_[ bin_id ] = false;
  drop_count_[ bin_id ] = 0;
}

void SfqCoDel::remove_bin( const uint64_t & bin_id ) {
  /* Remove bin from active list */
  bin_credits_.at( bin_id ) = 0;
  assert( active_list_.front() == bin_id ); /* Shouldn't be removing anything else */
  active_list_.pop();
  active_indicator_.at( bin_id ) = false;
}

uint64_t SfqCoDel::next_bin( void ) {
  uint64_t ret = -1;
  while ( not active_list_.empty() ) {
    uint64_t front_bin = active_list_.front();

    if ( pkt_queues_.at( front_bin ).empty() ) {
      /* Remove empty bins from the list */
      remove_bin( front_bin );
    } else if ( bin_credits_.at( front_bin ) < 0 ) {
      /* Replenish credits if they fall to below zero */
      /* Since queue is non-empty reinsert bin */
      remove_bin( front_bin );
      active_indicator_.at( front_bin ) = true;
      active_list_.push( front_bin );
      bin_credits_.at( front_bin ) += MTU_SIZE;
    } else {
      return front_bin;
    }
  }
  return ret;
}

void SfqCoDel::enqueue_packet( const QueuedPacket & p, const uint64_t & bin_id ) {
  if ( pkt_queues_.find( bin_id )  != pkt_queues_.end() ) {
    pkt_queues_.at( bin_id ).push( p );
    bytes_in_queue_ += p.contents.size();
  } else {
    init_bin( bin_id );
    pkt_queues_.at( bin_id ).push(p);
    bytes_in_queue_ += p.contents.size();
  }
}

QueuedPacket SfqCoDel::codel_deq_helper( const uint64_t & bin_id ) {
  if ( not pkt_queues_.at(bin_id).empty() ) {
    QueuedPacket p( pkt_queues_.at( bin_id ).front() );
    pkt_queues_.at( bin_id ).pop();
    bytes_in_queue_ -= p.contents.size();
    return p;
  } else {
    return QueuedPacket::nullpkt();
  }
}

SfqCoDel::dodeque_result SfqCoDel::codel_dodeque( const uint64_t & bin_id ) {
  uint64_t now = timestamp();
  dodeque_result r = { codel_deq_helper(bin_id), false };
  if ( r.p.contents.size() == 0 ) {
    first_above_time_.at( bin_id ) = 0;
  } else {
    uint64_t sojourn_time = now - r.p.entry_time;
    /* TODO: Check precedence order */
    if ( sojourn_time < target or bytes_in_queue_ < MTU_SIZE or pkt_queues_.at( bin_id ).empty() ) {
      // went below so we'll stay below for at least interval
      first_above_time_.at( bin_id ) = 0;
    } else {
      if ( first_above_time_.at( bin_id ) == 0 ) {
        // just went above from below. if we stay above
        // for at least interval we'll say it's ok to drop
        first_above_time_.at( bin_id ) = now + interval;
      } else if ( now >= first_above_time_.at( bin_id ) ) {
        r.ok_to_drop = true;
      }
    }
  }
  return r;
}

QueuedPacket SfqCoDel::codel_deque( const uint64_t & bin_id ) {
  assert( not pkt_queues_.at( bin_id ).empty() );
  uint64_t now = timestamp();
  dodeque_result r = codel_dodeque( bin_id );
  
  assert ( r.p.contents.size() != 0 );
  // There has to be a packet here.

  if ( dropping_.at( bin_id ) ) {
    if ( not r.ok_to_drop ) {
      // sojourn time below target - leave dropping_.at(bin_id) state
      // and send this bin's packet
      dropping_.at( bin_id ) = false;
    }

    // It’s time for the next drop. Drop the current packet and dequeue
    // the next.  If the dequeue doesn't take us out of dropping state,
    // schedule the next drop. A large backlog might result in drop
    // rates so high that the next drop should happen now, hence the
    // ‘while’ loop.
    while ( now >= drop_next_.at( bin_id ) and dropping_.at( bin_id ) ) {
      drop( r.p, bin_id );
      r = codel_dodeque( bin_id );

      //if drop emptied queue, it gets to be new on next arrival
      // and want to move on to next bin to find a packet to send
      if( r.p.contents.size() == 0 ) {
        first_above_time_.at( bin_id ) = 0;
        remove_bin( bin_id );
      }

      if ( not r.ok_to_drop ) {
        // leave dropping_.at(bin_id) state
        dropping_.at( bin_id ) = false;
      } else {
        // schedule the next drop.
        ++count_.at( bin_id );
        drop_next_.at( bin_id ) = control_law( drop_next_.at( bin_id ), count_.at( bin_id ) );
      }
    }

  // If we get here we’re not in dropping state. 'ok_to_drop' means that the
  // sojourn time has been above target for interval so enter dropping state.
  } else if ( r.ok_to_drop ) {
    drop( r.p, bin_id );
    r = codel_dodeque( bin_id );
    dropping_.at( bin_id ) = true;

    //if drop emptied bin's queue, it gets to be new on next arrival
    // and want to move on to next bin to find a packet to send
    if ( r.p.contents.size() == 0 ) {
      first_above_time_.at( bin_id ) = 0; // TODO: Why does the ns2 code reset count and drop_next as well?
      remove_bin( bin_id );
    }

    // If min went above target close to when it last went below,
    // assume that the drop rate that controlled the queue on the
    // last cycle is a good starting point to control it now.
    count_.at( bin_id ) = (count_.at( bin_id ) > 2 and now - drop_next_.at( bin_id ) < 8*interval)? count_.at( bin_id ) - 2 : 1;
    drop_next_.at( bin_id ) = control_law( now, count_.at( bin_id ) );
  }
  return (r.p);
}

void SfqCoDel::drop( const QueuedPacket & p, const uint64_t & bin_id ) {
  drop_count_.at( bin_id )++;
  fprintf(stderr,"Codel dropped a packet from bin id %ld with size %ld, count now at %u \n", bin_id, p.contents.size(), drop_count_.at( bin_id ));
}
