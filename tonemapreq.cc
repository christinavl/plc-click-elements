/*
 * tonemapreq.{cc,hh} -- Retrieves tonemap statistics for a specific PLC link
 *
 * This click element periodically sends tonemap requests for a specific slot and destination
 * and dumps the statistics.
 * Christina Vlachou, 2016
*/
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <click/config.h>
#include "tonemapreq.hh"
#include <click/etheraddress.hh>
#include <click/args.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include <click/straccum.hh>
#include <click/packet_anno.hh>
#include <clicknet/llc.h>
#include <click/handlercall.hh>
#include <click/straccum.hh>



CLICK_DECLS

#define TIMER_INTERVAL 1000 // timer interval in ms
#define NUMBER_OF_SLOTS 6 // The number of tonemap slots according to IEEE 1901
#define NUM_AVG_INTERVALS 23 // number of intervals to average for frequency response graph
#define NUM_CAR_INTERVALS 40 // number of carriers per interval

TonemapReq::TonemapReq()
     :_expire_timer_ms(this)
{
}

TonemapReq::~TonemapReq()
{
}


void *
TonemapReq::cast(const char *name)
{   
    if (strcmp(name, "TonemapReq") == 0)
        return this;
    else
        return Element::cast(name);
}

int
TonemapReq::initialize(ErrorHandler *)
{
    _expire_timer_ms.initialize(this);
    _expire_timer_ms.schedule_after_msec(TIMER_INTERVAL);
    return 0;
}


int
TonemapReq::configure(Vector<String> &conf, ErrorHandler *errh)
{
    return Args(conf, this, errh).read_m("SRC", _src)
                                 .read_m("DST", _dst)
                                 .complete();
}

void
TonemapReq::run_timer(Timer *t)
{   
    // Get statistics for PLC rates. Send the management message with request.
    for (int s = 0; s < NUMBER_OF_SLOTS; s++)
        sendToneMapReq(s);
    t->schedule_after_msec(TIMER_INTERVAL);
}


void
TonemapReq::push(int port,Packet *p)
{	
    click_ether *e = (click_ether *) p->data();
    click_hp_av_header *hpavh = (click_hp_av_header *) (e + 1);

    if((e->ether_type == htons(ETHERTYPE_HP_AV)) && (hpavh->MMType == htons(TONE_MAP_REP))) {
        processToneMapRep((click_hp_av_tone_map_rep*)(hpavh + 1));
        p->kill();
    }
    else 
        output(0).push(p);
}


void
TonemapReq::sendToneMapReq(int slot){
    const unsigned char dst[6] = {0x00, 0xB0, 0x52, 0x00, 0x00, 0x01};

    static_assert(Packet::default_headroom >= sizeof(click_ether));
    WritablePacket *q = Packet::make(Packet::default_headroom,
            NULL, sizeof(click_ether) + sizeof(click_hp_av_header) + sizeof(click_hp_av_tone_map_req), 0);
    if (!q) {
        click_chatter("TonemapReq: cannot make packet!");
        return;
    }

    click_ether *e = (click_ether *) q->data();
    q->set_ether_header(e);
    memcpy(e->ether_shost, &_src, 6);
    memcpy(e->ether_dhost, dst, 6);
    e->ether_type = htons(ETHERTYPE_HP_AV);

    click_hp_av_header *hpav = (click_hp_av_header *) (e + 1);
    hpav->version = 0;
    hpav->MMType = htons(TONE_MAP_REQ);


    click_hp_av_tone_map_req *tm_req = (click_hp_av_tone_map_req *) (hpav + 1);
    memcpy(tm_req->macaddr, &_dst, 6);
    tm_req->tmslot = slot;
    const unsigned char oui[3] = {0x00, 0xB0, 0x52};
    memcpy(tm_req->oui, oui, 3);
    q->timestamp_anno().assign_now();
    output(1).push(q); 
}


void
TonemapReq::processToneMapRep(click_hp_av_tone_map_rep *tm_rep){
    uint16_t max_carriers;
    double plc_rate;


    switch (tm_rep->mstatus) {
    case 0x00:
      click_chatter("[TonemapReq] Status: Success");
      break;
    case 0x01:
      click_chatter("[TonemapReq] Status: Unknown MAC address");
      return;
      break;
    case 0x02:
      click_chatter("[TonemapReq] Status: Unknown Tonemap slot");
      return;
      break;
    }
    click_chatter("[TonemapReq] Tonemap slot: %d", tm_rep->tmslot);
    click_chatter("[TonemapReq] Number of tone maps: %d", tm_rep->num_tms);
    click_chatter("[TonemapReq] Tonemap number of active carriers: %d", tm_rep->tm_num_act_carrier);


    max_carriers = tm_rep->tm_num_act_carrier / 2;
    if (tm_rep->tm_num_act_carrier & 1)
       max_carriers += 1;

    uint32_t sum_bit_per_carrier = 0;
    int modulation_stats[NUM_AVG_INTERVALS] = {};

    for (int i = 0; i < max_carriers * 2; i = i + 2) {
        int low_carr = get_carrier_modulation(tm_rep->carriers[i / 2].mod_carrier_lo);
        int high_carr = get_carrier_modulation(tm_rep->carriers[i / 2].mod_carrier_hi);
        sum_bit_per_carrier += low_carr + high_carr;
        if ((i + 1) / NUM_CAR_INTERVALS < NUM_AVG_INTERVALS)  {
            modulation_stats[ i / NUM_CAR_INTERVALS ] += low_carr;
            modulation_stats[ (i + 1) / NUM_CAR_INTERVALS ] += high_carr;       
        }
        else
           modulation_stats[NUM_AVG_INTERVALS - 1] += (low_carr + high_carr); 
    }
    // Effective symbol duration and guard interval used by most HPAV devices
    double symbol_duration = 40.96 + 5.56;
    // Multiply FEC rate with total bits per symbol and devide by symbol duration in \mu s
    plc_rate = (double) 16 / 21 * (double) sum_bit_per_carrier / symbol_duration;

    click_chatter("[TonemapReq] PHY rate: %f", plc_rate);
    print_frequency_response(modulation_stats, max_carriers);
}



uint8_t TonemapReq::get_carrier_modulation(short unsigned int modulation)
{
    switch(modulation) {
    case NO:
      return 0;
    case BPSK:
      return 1;
    case QPSK:
      return 2;
    case QAM_8:
      return 3;
    case QAM_16:
      return 4;
    case QAM_64:
      return 6;
    case QAM_256:
      return 8;
    case QAM_1024:
      return 10;
    default:
      return 0;
    }
}

void
TonemapReq::print_frequency_response(int * modulation_stats, int max_carriers) {
    // First compute average bits per symbol, per NUM_CAR_INTERVALS carriers
    for (int i = 0; i < NUM_AVG_INTERVALS - 1; i++) 
        modulation_stats[i] = modulation_stats[i] / NUM_CAR_INTERVALS;
    modulation_stats[NUM_AVG_INTERVALS - 1] = modulation_stats[NUM_AVG_INTERVALS - 1] / ((max_carriers *2 - 1) % NUM_CAR_INTERVALS);

   // Now plot an approximate frequency response based on tonemaps
    for (int i = MAX_BITS_PER_CARRIER; i > 0; i--) {
        StringAccum modul_line;
        for (int j = 0; j < NUM_AVG_INTERVALS; j++) {
            if (modulation_stats[j] >= i)
                modul_line << "|  ";
            else
                modul_line << "   ";
        }
        String linetoprint = modul_line.take_string();
        click_chatter(linetoprint.c_str());
    }
    click_chatter("----------------------------------------------------------------->");
    click_chatter("                          Frequency                               ");
}

CLICK_ENDDECLS
EXPORT_ELEMENT(TonemapReq)

