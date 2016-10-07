/*
 * sniffpackets.{cc,hh} -- PLC frames (MPDUs) sniffer
 *
 * This element dumps all PLC frames send/overheard by a station,
 * including beacons, sounding frames, data frames, management frames, ACKs.  
 * Note that reading only the frame delimiters (headers) is possible, and 
 * not the frame contents, such as data. At the initialization of the element,
 * it activates the sniffer mode of the device by sending a management message.
 * Similarly, the destructor sends a management message that disables the sniffer mode
 * of the PLC device.
 * Christina Vlachou, 2016
 */

#include <click/config.h>
#include <cstdlib>
#include <click/packet_anno.hh>
#include "sniffpackets.hh"


CLICK_DECLS

SniffPackets::SniffPackets()
{
}

SniffPackets::~SniffPackets()
{
}

void *
SniffPackets::cast(const char *name)
{
    if (strcmp(name, "SniffPackets") == 0)
        return this;
    else
        return Element::cast(name);
}


int
SniffPackets::initialize(ErrorHandler *)
{
    return enable_sniffer_mode();
}


void
SniffPackets::push(int, Packet *p) {

    click_ether *eth_hdr = (click_ether *) p->data();

    if(eth_hdr->ether_type == htons(ETHERTYPE_HP_AV)) {
        click_hp_av_header *hpavh = (click_hp_av_header *) (eth_hdr + 1);
        if(ntohs(hpavh->MMType) == SNIFFER_IND) {
            click_hp_av_sniffer_indicate *hpavh_sniff = (click_hp_av_sniffer_indicate *) (hpavh + 1);
            parse_plc_packet(hpavh_sniff);
            p->kill();
        }
        else
            output(0).push(p);
    }
    else
        output(0).push(p);

}

void
SniffPackets::parse_plc_packet(click_hp_av_sniffer_indicate *p) {
    click_hp_av_fc fc = p->fc;
    Timestamp _now;
    _now.assign_now();
    
    if(fc.del_type == 0) { // beacon
        click_chatter("[SniffPackets %s] The STA overheard a beacon.", _now.unparse().c_str());
    }
    else if (fc.del_type == 2) { // ACK
        click_chatter("[SniffPackets %s] The STA overheard an ACK.", _now.unparse().c_str());
    }
    else if (fc.del_type == 3) { // RTS/CTS
        click_chatter("[SniffPackets %s] The STA overheard an RTS/CTS.", _now.unparse().c_str());
    }
    else if (fc.del_type == 4) { // sounding message for channel estimation
        click_chatter("[SniffPackets %s] The STA overheard a sounding message.", _now.unparse().c_str());
    }
    else if (fc.del_type == 1) { // data or management frames
        uint16_t frame_length = fc.fl_av * 1.28;
        // Formula given by IEEE 1901 standard
        uint16_t mant = fc.ble >> 3;
        uint16_t exp = fc.ble & 7;

        uint16_t ble = (32 + mant) * power(2, exp - 4) + power(2, exp - 5);
        click_chatter("[SniffPackets %s] The STA overheard MPDU from STEI %d to %d, duration %d, priority %d, bit-loading estimate %d, MPDU sequence in the burst %d.",
                                  _now.unparse().c_str(), (int) fc.stei, (int) fc.dtei, frame_length, (int) fc.lid, (int) ble, (int) fc.mpdu_cnt);
    }
    else
        click_chatter("[SniffPackets %s] The STA overheard an unknown message type.");
}

int 
SniffPackets::enable_sniffer_mode() {
    const unsigned char dst[6] = {0x00, 0xB0, 0x52, 0x00, 0x00, 0x01};
    static_assert(Packet::default_headroom >= sizeof(click_ether));
    WritablePacket *q = Packet::make(Packet::default_headroom - sizeof(click_ether),
                                     NULL, sizeof(click_ether) + sizeof(click_hp_av_header) + sizeof(click_sniffer_request), 0);
    if (!q) {
        click_chatter("[SniffPackets] Cannot make packet!");
        return -1;
    }

    click_ether *e = (click_ether *) q->data();
    q->set_ether_header(e);
    memset(e->ether_shost, 0x00, 6);
    memcpy(e->ether_dhost, dst, 6);
    e->ether_type = htons(ETHERTYPE_HP_AV);
    
    click_hp_av_header *hpavh = (click_hp_av_header *) (e + 1);
    hpavh->version = 0;
    hpavh->MMType = htons(SNIFFER_REQ);

    click_sniffer_request *hpavh_sniff = (click_sniffer_request *) (hpavh + 1);
    const unsigned char oui[3] = {0x00, 0xB0, 0x52};
    hpavh_sniff->control = HPAV_SC_ENABLE;
    memcpy(hpavh_sniff->oui, oui, 3);
    q->timestamp_anno().assign_now();
    output(1).push(q);
    return 0;
}

int 
SniffPackets::disable_sniffer_mode() {
    const unsigned char dst[6] = {0x00, 0xB0, 0x52, 0x00, 0x00, 0x01};
    static_assert(Packet::default_headroom >= sizeof(click_ether));
    WritablePacket *q = Packet::make(Packet::default_headroom - sizeof(click_ether),
                                     NULL, sizeof(click_ether) + sizeof(click_hp_av_header) +sizeof(click_sniffer_request), 0);
    if (!q) {
        click_chatter("[SniffPackets] cannot make packet!");
        return -1;
    }

    click_ether *e = (click_ether *) q->data();
    q->set_ether_header(e);
    memset(e->ether_shost, 0x00, 6);
    memcpy(e->ether_dhost, dst, 6);
    e->ether_type = htons(ETHERTYPE_HP_AV);

    click_hp_av_header *hpavh = (click_hp_av_header *) (e + 1);
    hpavh->version = 0;
    hpavh->MMType = htons(SNIFFER_REQ);

    click_sniffer_request *hpavh_sniff = (click_sniffer_request *) (hpavh + 1);
    const unsigned char oui[3] = {0x00, 0xB0, 0x52};
    hpavh_sniff->control = HPAV_SC_DISABLE;
    memcpy(hpavh_sniff->oui, oui, 3);

    output(1).push(q);
    return 0;
}


static String
enable_sniffer_handler(Element *e, void *) {
    SniffPackets *elmt = (SniffPackets *)e;
    StringAccum status; 
    status << elmt->enable_sniffer_mode();
    return status.take_string();
}

static String
disable_sniffer_handler(Element *e, void *) {
    SniffPackets *elmt = (SniffPackets *)e;
    StringAccum status; 
    status << elmt->disable_sniffer_mode();
    return status.take_string();
}


void SniffPackets::add_handlers() {
    add_read_handler("enable", enable_sniffer_handler);
    add_read_handler("disable", disable_sniffer_handler);
}

EXPORT_ELEMENT(SniffPackets)
CLICK_ENDDECLS
