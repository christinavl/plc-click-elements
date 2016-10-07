/*
 * phyratesreq.{cc,hh} -- PLC PHY Rate statistics
 *
 * The element periodically sends a request for the PHY rates of transmission and reception
 * to all neighboring stations registered in the network. 
 * Christina Vlachou, 2016
 */

#include <click/config.h>
#include <math.h>
#include <cstdlib>
#include <click/packet_anno.hh>
#include "phyratesreq.hh"
#include <clicknet/ether.h>
#include "PLCStats.h"
#include <click/etheraddress.hh>
#include <click/confparse.hh>
#include <click/bitvector.hh>
#include <click/straccum.hh>
#include <click/router.hh>
#include <click/error.hh>
#include <click/glue.hh>
CLICK_DECLS


#define TIMER_INTERVAL 1000 // timer interval in ms

PhyRatesReq::PhyRatesReq()
    :_expire_timer_ms(this)
{
}

PhyRatesReq::~PhyRatesReq()
{
}

void *
PhyRatesReq::cast(const char *name)
{
    if (strcmp(name, "PhyRatesReq") == 0)
        return this;
    else
        return Element::cast(name);
}



int
PhyRatesReq::initialize(ErrorHandler *)
{
    _expire_timer_ms.initialize(this);
    _expire_timer_ms.schedule_after_msec(TIMER_INTERVAL);
    return 0;
}


void
PhyRatesReq::run_timer(Timer *t)
{
    // Get statistics for PLC rates. Send the management message with request.
    send_mm_plc();
    t->schedule_after_msec(TIMER_INTERVAL);
}


void
PhyRatesReq::send_mm_plc()
{
    const unsigned char dst[6] = {0x00, 0xB0, 0x52, 0x00, 0x00, 0x01};
    static_assert(Packet::default_headroom >= sizeof(click_ether));
    WritablePacket *q = Packet::make(Packet::default_headroom - sizeof(click_ether),
                                     NULL, sizeof(click_ether) + sizeof(click_hp_av_header), 0);
    if (!q) {
        click_chatter("[PhyRatesReq] cannot make packet!");
        return;
    }

    click_ether *e = (click_ether *) q->data();
    q->set_ether_header(e);
    memset(e->ether_shost, 0x00, 6);
    memcpy(e->ether_dhost, dst, 6);
    e->ether_type = htons(ETHERTYPE_HP_AV);

    click_hp_av_header *hpavh = (click_hp_av_header *) (e + 1);
    hpavh->version = 1;
    hpavh->MMType = htons(NW_STATS_REQ);
    q->timestamp_anno().assign_now();
    output(1).push(q);
}



// We received a reply from the PLC interface. 
void
PhyRatesReq::push(int port, Packet *p)
{
    // Process the incoming packet and check if it is HP_AV
    click_ether *ethh = (click_ether *) p->data();
    if (ethh->ether_type != htons(ETHERTYPE_HP_AV)) {
        output(0).push(p);
        return;
    }    

    // Process the incoming HP_AV Management message
    click_hp_av_header *hpavh = (click_hp_av_header *) (ethh + 1);
    //click_chatter("[PhyRatesReq] ")%u type ",hpavh->MMType);

    // Forward the packet if it is not the correct one
    if (hpavh->MMType != htons(NW_STATS_REP)) {
        output(0).push(p);
        return;
    }


    click_hp_av_nw_stats_conf *nwstats = (click_hp_av_nw_stats_conf *) (hpavh + 1);
    int rxstats;
    int txstats;

    // Get current timestamp
    Timestamp now;
    now.assign_now();

    click_chatter("[PhyRatesReq] Time %s, Number of STAs in network %d", now.unparse().c_str(), (int) nwstats->sta.NumSTAs);

    for (int i = 0; i < (int) nwstats->sta.NumSTAs; i++) {
        EtherAddress station = EtherAddress(nwstats->sta.infos[i].DA);
        rxstats = nwstats->sta.infos[i].AvgPHYDR_RX;
        txstats =  nwstats->sta.infos[i].AvgPHYDR_TX;
        click_chatter("[PhyRatesReq] MAC address: %s , Avg PHY rate from STA to DA: %d", station.unparse().c_str(), txstats);
        click_chatter("[PhyRatesReq] MAC address: %s , Avg PHY rate from DA to STA: %d", station.unparse().c_str(), rxstats);
    }
    p->kill();
}




CLICK_ENDDECLS
EXPORT_ELEMENT(PhyRatesReq)
ELEMENT_MT_SAFE(PhyRatesReq)
