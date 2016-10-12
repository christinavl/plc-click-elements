#ifndef CLICK_TONEMAPREQ_HH
#define CLICK_TONEMAPREQ_HH
#include <click/element.hh>
#include <clicknet/ether.h>
#include <click/etheraddress.hh>
#include <click/sync.hh>
#include <click/timer.hh>
#include "PLCStats.h"

CLICK_DECLS


class TonemapReq : public Element { public:

    TonemapReq();
    ~TonemapReq();

    EtherAddress _src;
    EtherAddress _dst;

    const char *class_name() const	{ return "TonemapReq"; }
    const char *port_count() const	{ return "1/2"; }
    const char *processing() const	{ return PUSH; }
    void *cast(const char *name);
    void run_timer(Timer *);
    int initialize(ErrorHandler *errh);
    int configure(Vector<String> &, ErrorHandler *);
    void push(int,Packet *);
    //void add_handlers();

private:
    Timer _expire_timer_ms;

    uint8_t get_carrier_modulation(short unsigned int);
    void sendToneMapReq(int);
    void processToneMapRep(click_hp_av_tone_map_rep *); 
    void print_frequency_response(int*, int);  

};

CLICK_ENDDECLS
#endif

