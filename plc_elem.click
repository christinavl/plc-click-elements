////////////////////////////SCRIPT DETAILS////////////////////////////
// This script is an example that uses the PLC click router elements

// Socket for Click handlers 
ControlSocket("TCP", 5555);


// Our PLC interface is called eth2; to be adapted to the router's PLC interface
AddressInfo(NAME eth2, DEVNAME eth2);
///////////////////////////////////////////////////////////////////////

////////////////////// Setup host element /////////////////////////////
// Fake interface (between click and host)
AddressInfo(fake0 eth2:ip 00:01:01:01:01:01);
AddressInfo(fakearp0 10.10.11.0/24 01:01:01:01:01:01);

// Represents a host.
// Click replies to ARP requests of host with a fake MAC
elementclass Host {
    fh :: FromHost(DEVNAME fake0, DST eth2:ip/24, ETHER fake0, HEADROOM 100);
    th :: ToHost(fake0);
    fh -> cl :: Classifier(12/0806 20/0001, 12/0800);
    cl[0] -> ARPResponder(0/0 fake0) -> th;
    cl[1] -> Strip(14) -> output;
    input -> EtherEncap(0x0800, fakearp0, fake0) -> th;
}
host :: Host();
///////////////////////////////////////////////////////////////////////



/////////////////////// From and To eth2 ///////////////////////////////////           
// Classifier from input eth2                                                   
// 0. ARP queries                                                                
// 1. ARP replies
// 2. IP                  
cl_in :: Classifier(12/0806 20/0001, 12/0806 20/0002, 12/0800, -);      

// Elements for handling ARP requests and replies
arpq :: ARPQuerier(eth2);
arpr :: ARPResponder(eth2);                                                                           

// Deliver ARP responses to ARP querier.                                  
sendQueue_eth :: Queue(200) -> td_eth :: ToDevice(eth2, DEBUG false);                                                                    
cl_in[0] -> HostEtherFilter(eth2, DROP_OWN false, DROP_OTHER true) -> arpr -> sendQueue_eth;
cl_in[1] -> HostEtherFilter(eth2, DROP_OWN false, DROP_OTHER true) -> [1]arpq;                  
// Discard non-IP packets                                                                               
cl_in[3] -> Discard();                                                                       

// Examples of using out PLC Click router elements. Some elements require the MAC address of the peer node for which we request statistics.
FromDevice(eth2, SNIFFER false, PROMISC true) -> plcelem :: ErrorStatsReq(SRC eth2:eth, DST 00:0D:B9:3D:C2:AA, PRIORITY 1, DIRECTION 1) -> cl_in;
//FromDevice(eth2, SNIFFER false, PROMISC true) -> plcelem :: PhyRatesReq -> cl_in;
//FromDevice(eth2, SNIFFER false, PROMISC true) -> plcelem :: SniffPackets -> cl_in;

// Packets for eth2 Queue
arpq -> cl_ARP :: Classifier(12/0806, 12/0800);
cl_ARP[0] -> sendQueue_eth;
cl_ARP[1] -> sendQueue_eth;
plcelem[1] -> sendQueue_eth;

// Simple routing table
rt :: DirectIPLookup(eth2:ip 0,                                                                        
                     10.10.11.255/32 0,                                                     
                     10.10.11.0/32 0,                                                         
                     10.10.11.0/24 1,  
                     0/0 2);                                                                            
                                                                                                        
// IP packets for this machine.                                                                         
rt[0] -> host;   
// Packets for stations in our local network. Output to ARP Querier                                                              
rt[1] -> DropBroadcasts                                                                                 
      -> dt1 :: DecIPTTL                                                                                
      -> fr1 :: IPFragmenter(1500)                                                                     
->[0]arpq; 

// Discard all packets not complying with the above rules
rt[2] -> Discard;                                                                                       


// IP packets                                                                               
cl_in[2] -> Strip(14) -> CheckIPHeader -> rt;   
// Packets from host go to routing    
host -> CheckIPHeader -> rt;
