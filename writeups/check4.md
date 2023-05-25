Checkpoint 4 Writeup
====================

My name: Mingfei Guo

My SUNet ID: mfguo

I collaborated with: None

I would like to thank/reward these classmates for their help: Everyone on Ed

This checkpoint took me about 6 hours to do. I did attend the lab session.

Program Structure and Design of the NetworkInterface:

New Variables:

- queue<EthernetFrame> maybe_send_queue = {};
The queue to send EthernetFrame. If we push a frame to this queue, 
we could consider it to be sent.

- map<uint32_t, pair<queue<InternetDatagram>, uint32_t>> 
  ip_addr_to_ethernet_datagram_queue = {};
The map of ip address to ethernet datagram queue. Each queue also has a
time stamp, recording the time when the ARP request about the same IP
address is sent. If the time delta is smaller than 5 seconds, we will
not send another ARP request and should just wait for the previous ARP reply.

- map<uint32_t, pair<EthernetAddress, uint32_t>> 
  ip_addr_to_ethernet_addr = {};
The map of ip address to ethernet address. Each ethernet address also
has a time stamp, recording the time to remember the mapping. If the
time delta is larger than 30 seconds, we will forget the mapping and
expire it.



New Helper Functions:

- optional<pair<EthernetAddress, uint32_t>> 
  map_ip_addr_to_ethernet_addr( uint32_t ip_addr );
Find the ip_addr_to_ethernet_addr map and return the corresponding
ethernet address and time stamp. Return an optional pair if the
mapping does not exist.

- optional<pair<queue<InternetDatagram>, uint32_t>> 
  map_ip_addr_to_ethernet_datagram_queue( uint32_t ip_addr );
Find the ip_addr_to_ethernet_datagram_queue map and return the
corresponding optional pair.

- void update_maps( uint32_t ip_addr, EthernetAddress MAC_address );
After receiving an ARP reply, update the ip_addr_to_ethernet_addr map
and ip_addr_to_ethernet_datagram_queue map. For ip_addr_to_ethernet_addr,
set new MAC address, and reinitialize the time stamp to 0. For
ip_addr_to_ethernet_datagram_queue, send the ethernet datagram queue
of the corresponding IP address to the MAC address and delete the
queue from the map. 

- EthernetFrame make_frame( const EthernetAddress& src,
                            const EthernetAddress& dst,
                            const uint16_t type,
                            vector<Buffer> payload );
Make an EthernetFrame with the given parameters. Reduce
duplication.

- ARPMessage make_arp( const uint16_t opcode,
                       const EthernetAddress sender_ethernet_address,
                       const uint32_t sender_ip_address,
                       const EthernetAddress target_ethernet_address,
                       const uint32_t target_ip_address );
Make an ARPMessage with the given parameters. Reduce duplication.


Implementation Challenges:

1. It was a little complicated to use these two map data structures. So, 
I split all map operations into helper functions to make it simpler. 
Additionally, I thought it would be good to use pairs to bind the address 
and its timestamp together, making the checking of timeouts easier.

2. Thanks to the hint on Ed, the serialize() and parse() functions were 
really helpful. Initially, I was trying to use serialize() and parse() 
from the EthernetFrame class, but I couldn't figure out how to access the 
variables of the Serialize class and Parse class.

3. While trying to create the EthernetFrame or ARPMessage, I used 
two helper functions to reduce duplication. I believe it's a good 
approach to make the code cleaner and easier to read. Moreover, there 
are times when the src and dst can be confusing, so the helper functions 
can contribute to making it cleaner.

4. I believe we need to check the validity of the EthernetFrame at the 
beginning of the recv_frame() function, which I initially neglected. Only 
when it's flooding or the dst is the local MAC address, should we proceed 
with processing the frame.

Remaining Bugs:
None.

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
