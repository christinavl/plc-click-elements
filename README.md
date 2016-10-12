# plc-click-elements

The repository contains elements for Click Router (http://read.cs.ucla.edu/click/click), which configure and measure statistics with PLC devices. The elemements use management frames that are sent via the Ethernet interface and assume userlevel operation of Click. The elements assume some familiarity with PLC procedures and protocols. A crash course on these procedures and protocols can be found in Chapter 2 of the following thesis: (http://infoscience.epfl.ch/record/218641). In the following, we explain the use of each file.

 - PLCStats.h The file contains stuctures and data for frame headers, frame content and frame types. 
 - phyratesreq.{cc/hh} This element periodically sends requests for all physical rates between the station and all its neighbours. The element prints the average receive and transmit rates for all neighbors.
 - tonemapreq.{cc/hh} This element periodically sends requests for the tonemaps (the modulation per OFDM carrier that PLC uses) between the station and a specific station whose Ethernet address given as an input to the element (DST).
 - errorstatsreq.{cc/hh} This element periodically sends requests for packet delivery statistics between the station and a specific station whose Ethernet address given as an input to the element (DST). The element has to take two more inputs: the direction of communication (i.e., reception or transmission) called DIRECTION, and the priority of the packets called PRIORITY. The priority refers to the one of PLC frame headers as defined in the IEEE 1901 standard.
 - sniffpackets.{cc/hh} This element enables the sniffer mode of PLC devices and captures every frame overheard by the station. It prints all PLC frame headers with some useful information. The element has two handlers to enable and disable the sniffer mode. To access the handlers via telnet, use the command "telnet localhost 5555" (port 5555 is the one used in the example script described below) and then the commands "read plcelem.disable" or "read plcelem.enable", where the name of the SniffPackets element is "plcelem".
 - plc_elem.click This is a sample Click script that uses the elements above. It assumes that a PLC device is connected to interface eth2 and that it has an IP address in subnet 10.10.11.0/24.

The elements have been tested with certain PLC devices with hardware chips such as INT6400. As some management messages are vendor-specific, the operation of the element can depend on the PLC device. 
The structure of the PLC management frames has been inferred from experiments and from the open-source projects Faifa (http://github.com/ffainelli/faifa) and Qualcomm Atheros Open Powerline Toolkit (http://github.com/qca/open-plc-utils).

