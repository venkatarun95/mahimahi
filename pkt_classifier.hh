#ifndef PKT_CLASSIFIER_HH
#define PKT_CLASSIFIER_HH

#include<string>

class PktClassifier
{
public :
    static uint64_t  get_flow_id( const std::string & packet_str );
    static const uint8_t P2P_HDR_SIZE = 4; /* Best guess at P2P header size */
};

#endif  /* PKT_CLASSIFIER_HH */
