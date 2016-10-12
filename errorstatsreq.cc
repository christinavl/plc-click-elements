/*
 * errorstatsreq.{cc,hh} -- Retrieves error and collision statistics for a specific PLC link
 *
 * This click element periodically sends error/collision statistics requests for a specific destination and priority
 * and dumps the statistics.
 * Christina Vlachou, 2016
*/
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <click/config.h>
#include "errorstatsreq.hh"
#include <click/etheraddress.hh>
#include <click/args.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include <click/straccum.hh>
#include <click/packet_anno.hh>
#include <clicknet/llc.h>
#include <click/handlercall.hh>


CLICK_DECLS

#define TIMER_INTERVAL 1000 // timer interval in ms

ErrorStatsReq::ErrorStatsReq()
     :_expire_timer_ms(this)
{
}

ErrorStatsReq::~ErrorStatsReq()
{
}


void *
ErrorStatsReq::cast(const char *name)
{   
    if (strcmp(name, "ErrorStatsReq") == 0)
        return this;
    else
        return Element::cast(name);
}

int
ErrorStatsReq::initialize(ErrorHandler *)
{
    _expire_timer_ms.initialize(this);
    _expire_timer_ms.schedule_after_msec(TIMER_INTERVAL);
    return 0;
}


int
ErrorStatsReq::configure(Vector<String> &conf, ErrorHandler *errh)
{
    return Args(conf, this, errh).read_m("SRC", _src)
                                 .read_m("DST", _dst)
                                 .read_m("DIRECTION", _dir)
                                 .read_m("PRIORITY", _prio)
                                 .complete();
}

void
ErrorStatsReq::run_timer(Timer *t)
{   
    // Get statistics for PLC rates. Send the management message with request.
    sendErrorStatsReq();
    t->schedule_after_msec(TIMER_INTERVAL);
}


void
ErrorStatsReq::push(int port,Packet *p)
{	
    click_ether *e = (click_ether *) p->data();
    if(e->ether_type == htons(ETHERTYPE_HP_AV)) {
        click_hp_av_header *hpavh = (click_hp_av_header *) (e + 1);
        if(ntohs(hpavh->MMType) == ERROR_STATS_REP) {
            processErrorStatsRep((click_hp_av_error_stats_rep*)(hpavh+1));
            p->kill();
        }
        else
            output(0).push(p);
    }
    else
        output(0).push(p);
}

void
ErrorStatsReq::sendErrorStatsReq(){
    const unsigned char dst[6] = {0x00, 0xB0, 0x52, 0x00, 0x00, 0x01};

    static_assert(Packet::default_headroom >= sizeof(click_ether));
    WritablePacket *q = Packet::make(Packet::default_headroom,
            NULL, sizeof(click_ether) + sizeof(click_hp_av_header) + sizeof(click_hp_av_error_stats_req), 0);
    if (!q) {
        click_chatter("[ErrorStatsReq] cannot make packet!");
        return;
    }

    click_ether *e = (click_ether *) q->data();
    q->set_ether_header(e);
    memset(e->ether_shost, 0x00, 6);
    memcpy(e->ether_dhost, dst, 6);
    e->ether_type = htons(ETHERTYPE_HP_AV);

    click_hp_av_header *hpav = (click_hp_av_header *) (e + 1);
    hpav->MMType = htons(ERROR_STATS_REQ);
    hpav->version = 0;

    click_hp_av_error_stats_req *error_req = (click_hp_av_error_stats_req *) (hpav + 1);
    memcpy(error_req->macaddr, &_dst, 6);
    error_req->link_id = _prio;
    error_req->direction = _dir;
    error_req->control = 0;
    const unsigned char oui[3] = {0x00, 0xB0, 0x52};
    memcpy(error_req->oui, oui, 3);

    q->timestamp_anno().assign_now();
    output(1).push(q); 
}

void 
ErrorStatsReq::print_tx_stats(tx_link_stats *tx) {
    click_chatter("[ErrorStatsReq] Printing statistics for Transmission.");
    click_chatter("[ErrorStatsReq] MPDUs ACKed: %u.", tx->mpdu_ack);
    click_chatter("[ErrorStatsReq] MPDUs Collided: %u.", tx->mpdu_coll);
    click_chatter("[ErrorStatsReq] MPDUs Failed: %u.", tx->mpdu_fail);
    click_chatter("[ErrorStatsReq] PBs Passed FEC block: %u.", tx->pb_pass);
    click_chatter("[ErrorStatsReq] PBs Failed FEC block: %u.", tx->pb_fail);
}

void 
ErrorStatsReq::print_rx_stats(rx_link_stats *rx) {
    click_chatter("[ErrorStatsReq] Printing statistics for Reception.");
    click_chatter("[ErrorStatsReq] MPDUs ACKed: %u.", rx->mpdu_ack);
    click_chatter("[ErrorStatsReq] MPDUs Failed: %u.", rx->mpdu_fail);
    click_chatter("[ErrorStatsReq] PBs Passed FEC block: %u.", rx->pb_pass);
    click_chatter("[ErrorStatsReq] PBs Failed FEC block: %u.", rx->pb_fail);
    click_chatter("[ErrorStatsReq] Turbo Error bits Passed: %u.", rx->tbe_pass);
    click_chatter("[ErrorStatsReq] Turbo Error bits Failed: %u.", rx->tbe_fail);
    // Printing stats per tonemap slot. Useful for analyzing noise/capacity per slot.
    for (int i = 0; i < rx->num_rx_intervals; i++) {
        click_chatter("[ErrorStatsReq] Stats for Tonemap Slot %d ", i);
        click_chatter("[ErrorStatsReq]      PHY Rate: %u", rx->rx_interval_stats[i].phyrate);
        click_chatter("[ErrorStatsReq]      PBs Passed: %u", rx->rx_interval_stats[i].pb_pass);
        click_chatter("[ErrorStatsReq]      PBs Failed: %u", rx->rx_interval_stats[i].pb_fail);
        click_chatter("[ErrorStatsReq]      Turbo Error bits Passed: %u", rx->rx_interval_stats[i].tbe_pass);
        click_chatter("[ErrorStatsReq]      Turbo Error bits Failed: %u", rx->rx_interval_stats[i].tbe_fail);
    }

}

void
ErrorStatsReq::processErrorStatsRep(click_hp_av_error_stats_rep *error_rep){
    switch(error_rep->mstatus) {
    case HPAV_SUC:
        click_chatter("[ErrorStatsReq] Status of received MME: Success\n");
        break;
    case HPAV_INV_CTL:
        click_chatter("[ErrorStatsReq] Status of received MME: Invalid control\n");
        break;
    case HPAV_INV_DIR:
        click_chatter("[ErrorStatsReq] Status of received MME: Invalid direction\n");
        break;
    case HPAV_INV_LID:
        click_chatter("[ErrorStatsReq] Status of received MME: Invalid Link ID\n");
        break;
    case HPAV_INV_MAC:
        click_chatter("[ErrorStatsReq] Status of received MME: Invalid MAC address\n");
        break;
    }


    if (error_rep->direction == HPAV_SD_TX) {
        print_tx_stats(&(error_rep->tx));
    }
    else if  (error_rep->direction == HPAV_SD_RX) {
        print_rx_stats(&(error_rep->rx));
    }
    else if (error_rep->direction == HPAV_SD_BOTH) {
        print_tx_stats(&(error_rep->txboth));
        print_rx_stats(&(error_rep->rxboth));
    }
    else
        click_chatter("[ErrorStatsReq] Unknown direction.");


    return;
}


CLICK_ENDDECLS
EXPORT_ELEMENT(ErrorStatsReq)

