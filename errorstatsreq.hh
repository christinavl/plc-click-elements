#ifndef CLICK_ERRORSTATSREQ_HH
#define CLICK_ERRORSTATSREQ_HH
#include <click/element.hh>
#include <clicknet/ether.h>
#include <click/etheraddress.hh>
#include <click/sync.hh>
#include <click/timer.hh>
#include "PLCStats.h"

CLICK_DECLS


class ErrorStatsReq : public Element { public:

    ErrorStatsReq();
    ~ErrorStatsReq();

    EtherAddress _src;
    EtherAddress _dst;
    int _prio;
    int _dir;

    const char *class_name() const	{ return "ErrorStatsReq"; }
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

    void sendErrorStatsReq();
    void print_tx_stats(tx_link_stats *);
    void print_rx_stats(rx_link_stats *);
    void processErrorStatsRep(click_hp_av_error_stats_rep *); 
  

};

CLICK_ENDDECLS
#endif

