#include <netinet/ip.h>
#include <netinet/tcp.h>

#include "pkt_classifier.hh"

extern "C" {
/* lookup3.c, by Bob Jenkins, May 2006, Public Domain.  */

/*
 * My best guess at if you are big-endian or little-endian.  This may
 * need adjustment.
 */
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
     __BYTE_ORDER == __LITTLE_ENDIAN) || \
    (defined(i386) || defined(__i386__) || defined(__i486__) || \
     defined(__i586__) || defined(__i686__) || defined(vax) || defined(MIPSEL))
# define HASH_LITTLE_ENDIAN 1
# define HASH_BIG_ENDIAN 0
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
       __BYTE_ORDER == __BIG_ENDIAN) || \
      (defined(sparc) || defined(POWERPC) || defined(mc68000) || defined(sel))
# define HASH_LITTLE_ENDIAN 0
# define HASH_BIG_ENDIAN 1
#else
# define HASH_LITTLE_ENDIAN 0
# define HASH_BIG_ENDIAN 0
#endif

#define hashsize(n) ((u_int32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

static inline u_int32_t jhash_3words( u_int32_t a, u_int32_t b, u_int32_t c, u_int32_t i )
{
    i += 0xdeadbeef + (3 << 2);
    a += i;
    b += i;
    c += i;
    mix(a, b, c);
    final(a, b, c);
    return c;
}

#undef mix
#undef final

}

uint64_t PktClassifier::get_flow_id( const std::string & packet_str )
{
  /* Seek to the beginning of the IP header */
  const char* packet = packet_str.c_str();
  const struct ip* ip_hdr = reinterpret_cast< const struct ip* >( packet + P2P_HDR_SIZE );
  unsigned char protocol = ip_hdr->ip_p;

  /* Get ip source and destination address */
  uint32_t src_addr = ntohl( ip_hdr->ip_src.s_addr );
  uint32_t dst_addr = ntohl( ip_hdr->ip_dst.s_addr );

  /* Get TCP src port and dst port */
  const struct tcphdr* tcp_hdr = reinterpret_cast< const struct tcphdr* >( packet + P2P_HDR_SIZE + sizeof( struct ip ) );
  uint16_t sport = ntohs( tcp_hdr -> source );
  uint16_t dport = ntohs( tcp_hdr -> dest );

  unsigned int hash = jhash_3words( protocol, src_addr, dst_addr, sport * dport );

//  printf("protocol type is %u, saddr is %u, daddr is %u, sport is %u, dport is %u, size is %lu, hash is %u\n", protocol, src_addr, dst_addr, sport, dport, packet_str.size(), hash );
  return hash;
}
