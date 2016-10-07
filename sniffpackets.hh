#ifndef CLICK_SNIFFPACKETS_HH
#define CLICK_SNIFFPACKETS_HH
#include <click/element.hh>
#include <click/etheraddress.hh>
#include <click/notifier.hh>
#include "PLCStats.h"
#include <click/args.hh>
#include <clicknet/ether.h>
#include <click/confparse.hh>
#include <click/bitvector.hh>
#include <click/straccum.hh>
#include <click/router.hh>
#include <click/error.hh>
#include <click/glue.hh>


CLICK_DECLS

class SniffPackets : public Element { public:

    SniffPackets();
    ~SniffPackets();

    const char *class_name() const      { return "SniffPackets"; }
    const char *port_count() const      { return "1/2"; }
    const char *processing() const      { return PUSH; }
    const char *flow_code() const       { return "x/xx"; }
    const char *flags() const           { return "L2"; }
    void *cast(const char *name);
    int initialize(ErrorHandler *errh);
    void push(int port, Packet *p);
    int enable_sniffer_mode();
    int disable_sniffer_mode();
//    static String enable_sniffer_handler(Element *, void *);
//    static String disable_sniffer_handler(Element *, void *);
    void add_handlers();


private:
    void parse_plc_packet(click_hp_av_sniffer_indicate *p);
};

CLICK_ENDDECLS
#endif

