#ifndef PKT_CLASSIFIER_HH
#define PKT_CLASSIFIER_HH

#include<string>

class PktClassifier
{
public :
    static uint64_t  get_flow_id( const std::string & packet_str );
};

#endif
