
#ifndef CLICKNET_PLCSTATS_H
#define CLICKNET_PLCSTATS_H


#define ETHERTYPE_HP_AV 0x88e1
#define NW_STATS_REQ  0x4860
#define TONE_MAP_REQ  0x70a0
#define HP_AV_VERSION 0x01
#define NW_STATS_REP  0x4960
#define TONE_MAP_REP  0x71a0
#define ERROR_STATS_REQ   0x30a0
#define ERROR_STATS_REP   0x31a0
#define SNIFFER_IND 0x36a0
#define SNIFFER_REQ 0x34a0

#define HPAV_SC_DISABLE 0x00
#define HPAV_SC_ENABLE 0x01



// Standard structure of the header of a HomePlug frame
// The standard messages start with MMType 0x60
// Vendor specific messages start with MMType 0xA0
struct click_hp_av_header {
    uint8_t version;
    uint16_t MMType;
} CLICK_SIZE_PACKED_ATTRIBUTE;
////////////////////////////////////////



/// Tone Maps Structures for REQ and REP as well as useful stuff
struct click_hp_av_tone_map_req {
    uint8_t  oui[3]; 
    uint8_t  macaddr[6];            // MAC Address of the neighbour that we want to know the tonemap for
    uint8_t  tmslot;                // Timeslot 
} CLICK_SIZE_PACKED_ATTRIBUTE;  

struct carrier {
    uint8_t    mod_carrier_lo:4;
    uint8_t    mod_carrier_hi:4;
} CLICK_SIZE_PACKED_ATTRIBUTE;


struct click_hp_av_tone_map_rep {
    uint8_t  oui[3];
    uint8_t    mstatus;
    uint8_t    tmslot;
    uint8_t    num_tms;
    uint16_t   tm_num_act_carrier;
    struct carrier  carriers[0];
} CLICK_SIZE_PACKED_ATTRIBUTE;  

enum mod_carrier {
    NO  = 0x0,
    BPSK    = 0x1,
    QPSK    = 0x2,
    QAM_8   = 0x3,
    QAM_16  = 0x4,
    QAM_64  = 0x5,
    QAM_256 = 0x6,
    QAM_1024 = 0x7,
};

#define MAX_BITS_PER_CARRIER 10

////////////////////////////////////////


/// Structures for sniffer frames ///

struct click_hp_av_fc {
    uint8_t	del_type:3;
    uint8_t	access:1;
    uint8_t	snid:4;
    uint8_t	stei;
    uint8_t	dtei;
    uint8_t	lid;
    uint8_t	cfs:1;
    uint8_t	bdf:1;
    uint8_t	hp10df:1;
    uint8_t	hp11df:1;
    uint8_t	eks:4;
    uint8_t	ppb;
    uint8_t	ble;
    uint8_t	pbsz:1;
    uint8_t	num_sym:2;
    uint8_t	tmi_av:5;
    uint16_t	fl_av:12;
    uint8_t	mpdu_cnt:2;
    uint8_t	burst_cnt:2;
    uint8_t	clst:3;
    uint8_t	rg_len:6;
    uint8_t	mfs_cmd_mgmt:3;
    uint8_t	mfs_cmd_data:3;
    uint8_t	rsr:1;
    uint8_t	mcf:1;
    uint8_t	dccpcf:1;
    uint8_t	mnbf:1;
    uint8_t	rsvd:5;
    uint8_t	fccs_av[3];

} CLICK_SIZE_PACKED_ATTRIBUTE;

struct click_hp_av_bcn {
    uint8_t	del_type:3;
    uint8_t	access:1;
    uint8_t	snid:4;
    uint32_t	bts;
    uint16_t	bto_0;
    uint16_t	bto_1;
    uint16_t	bto_2;
    uint16_t	bto_3;
    uint8_t	fccs_av[3];
} CLICK_SIZE_PACKED_ATTRIBUTE;


struct click_hp_av_sniffer_indicate {
    uint8_t  oui[3];
    uint8_t	type;
    uint8_t	direction;
    uint64_t	systime;
    uint32_t	beacontime;
    struct click_hp_av_fc	fc;
    struct click_hp_av_bcn	bcn;
} CLICK_SIZE_PACKED_ATTRIBUTE;


struct click_sniffer_request {
    uint8_t  oui[3];
    uint8_t	control;
    uint8_t	reserved1[4];
} CLICK_SIZE_PACKED_ATTRIBUTE;
/////////////////////////////////

/// Structures for PHY-rates stats ///
struct cm_sta_info {
    uint8_t DA[6];
    uint8_t AvgPHYDR_TX;
    uint8_t AvgPHYDR_RX;
};
struct cm_sta_infos {
    uint8_t NumSTAs;
    struct cm_sta_info infos[0];
};

struct click_hp_av_nw_stats_conf {
    uint16_t fmi;
    struct cm_sta_infos sta;
} CLICK_SIZE_PACKED_ATTRIBUTE; 
/////////////////////////////////

/// Structures for error stats ///
/* Direction types */
enum statistics_direction {
    HPAV_SD_TX  = 0x00,
    HPAV_SD_RX  = 0x01,
    HPAV_SD_BOTH    = 0x02,
};

enum link_id {
    HPAV_LID_CSMA_CAP_0 = 0x00,
    HPAV_LID_CSMA_CAP_1 = 0x01,
    HPAV_LID_CSMA_CAP_2 = 0x02,
    HPAV_LID_CSMA_CAP_3 = 0x03,
    HPAV_LID_CSMA_SUM   = 0xF8,
    HPAV_LID_CSMA_SUM_ANY   = 0xFC,
};

struct rx_interval_stats {
    uint8_t    phyrate;
    uint64_t   pb_pass;
    uint64_t   pb_fail;
    uint64_t   tbe_pass;
    uint64_t   tbe_fail;
} CLICK_SIZE_PACKED_ATTRIBUTE;

struct tx_link_stats {
    uint64_t   mpdu_ack;
    uint64_t   mpdu_coll;
    uint64_t   mpdu_fail;
    uint64_t   pb_pass;
    uint64_t   pb_fail;
} CLICK_SIZE_PACKED_ATTRIBUTE;

struct rx_link_stats {
    uint64_t   mpdu_ack;
    uint64_t   mpdu_fail;
    uint64_t   pb_pass;
    uint64_t   pb_fail;
    uint64_t   tbe_pass;
    uint64_t   tbe_fail;
    uint8_t    num_rx_intervals;
    struct rx_interval_stats    rx_interval_stats[0];
} CLICK_SIZE_PACKED_ATTRIBUTE;

struct click_hp_av_error_stats_req {
    uint8_t  oui[3];
    uint8_t control;
    uint8_t    direction;
    uint8_t    link_id;
    uint8_t    macaddr[6];
} CLICK_SIZE_PACKED_ATTRIBUTE;

struct click_hp_av_error_stats_rep {
    uint8_t    oui[3];
    uint8_t    mstatus;
    uint8_t    direction;
    uint8_t    link_id;
    uint8_t    tei;
    union {
        tx_link_stats tx;
        rx_link_stats rx;
        struct {
            tx_link_stats txboth;
            rx_link_stats rxboth;
        } CLICK_SIZE_PACKED_ATTRIBUTE;
    } CLICK_SIZE_PACKED_ATTRIBUTE;
} CLICK_SIZE_PACKED_ATTRIBUTE;

enum statistics_status {
    HPAV_SUC    = 0x00,
    HPAV_INV_CTL    = 0x01,
    HPAV_INV_DIR    = 0x02,
    HPAV_INV_LID    = 0x10,
    HPAV_INV_MAC    = 0x20,
};
/////////////////////////////////


// Raising number x to power n, x^n
static uint64_t power(uint32_t x, uint32_t n) {
    uint64_t res = 1;
    uint64_t a = x;
    while (n) {
        if (n & 1)
            res *= a;
        a *= a;
        n >>= 1;
    }
    return res;
}

#endif
