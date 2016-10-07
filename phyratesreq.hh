#ifndef CLICK_PHYRATESREQ_HH
#define CLICK_PHYRATESREQ_HH
#include <click/element.hh>
#include <click/sync.hh>
#include <click/timer.hh>
CLICK_DECLS

class PhyRatesReq : public Element { public:

    PhyRatesReq();
    ~PhyRatesReq();

    const char *class_name() const      { return "PhyRatesReq"; }
    const char *port_count() const      { return "1/2"; }
    const char *processing() const      { return PUSH; }
    const char *flow_code() const       { return "xyyy/xx"; }
    const char *flags() const           { return "L2"; }
    void *cast(const char *name);
    int initialize(ErrorHandler *errh);
    void push(int port, Packet *p);
    void run_timer(Timer *);


private:
    Timer _expire_timer_ms;
    void send_mm_plc();
    static void expire_hook(Timer *, void *);

};

CLICK_ENDDECLS
#endif

