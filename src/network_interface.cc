#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "parser.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t next_hop_ip = next_hop.ipv4_numeric();
  optional<EthernetAddress> MAC_address = map_ip_addr_to_ethernet_addr(next_hop_ip).first;
    if (MAC_address.has_value()) {
      EthernetFrame frame;
      frame.header().src = ethernet_address_; // EthernetAddress
      frame.header().dst = MAC_address.value();
      frame.header().type = EthernetHeader::TYPE_IPv4;

      frame.payload = serialize(dgram);
      maybe_send_queue.push(frame);
    } else {
      optional<pair<queue<InternetDatagram>, uint32_t>> ethernet_datagram_queue = map_ip_addr_to_ethernet_datagram_queue(ip_addr);
      bool ARP_request = false;
      if (ethernet_datagram_queue.has_value()) {
          ethernet_datagram_queue.value().first.push(dgram);        
          if (ethernet_datagram_queue.value().second >= 5) {
            ARP_request =true;
          }
      } else {
          pair<queue<InternetDatagram>, uint32_t>> new_ethernet_datagram_queue;
          new_ethernet_datagram_queue.first.push(make_pair(dgram, 0));
          ip_addr_to_ethernet_datagram_queue[ip_addr] = new_ethernet_datagram_queue; 
          ARP_request = true;
      }
      if (ARP_request) {
        EthernetFrame frame;
        frame.header().src = ethernet_address_;
        frame.header().dst = ETHERNET_BROADCAST;
        frame.header().type = EthernetHeader::TYPE_ARP;

        ARPMessage send_ARP;
        send_ARP.opcode = ARPMessage::OPCODE_REQUEST;
        send_ARP.sender_ethernet_address = ethernet_address_;
        send_ARP.sender_ip_address = ip_address_.ipv4_numeric();
        // send_ARP.target_ethernet_address = ip_addr;
        send_ARP.target_ip_address = ip_addr;
        frame.payload = serialize(arp);
        maybe_send_queue.push(frame);
      }
    }
}

optional<pair<EthernetAddress, uint32_t>> NetworkInterface::map_ip_addr_to_ethernet_addr(uint32_t ip_addr) {
    map<uint32_t, pair<EthernetAddress, uint32_t> >::iterator iter;
    iter = ip_addr_to_ethernet_addr.find(ip_addr);
    if (iter != ip_addr_to_ethernet_addr.end()) {
      return iter->second;
    }
    return nullopt;
}

optional<pair<queue<InternetDatagram>, uint32_t>> NetworkInterface::map_ip_addr_to_ethernet_datagram_queue(uint32_t ip_addr) {
    map<uint32_t, pair<queue<InternetDatagram>, uint32_t>>::iterator iter;
    iter = ip_addr_to_ethernet_datagram_queue.find(ip_addr);
    if (iter != ip_addr_to_ethernet_datagram_queue.end()) {
      return iter->second;
    }
    return nullopt;
}


// //! \brief [IPv4](\ref rfc::rfc791) Internet datagram
// struct IPv4Datagram
// {
//   IPv4Header header {};
//   std::vector<Buffer> payload {};

//   void parse( Parser& parser )
//   {
//     header.parse( parser );
//     parser.all_remaining( payload );
//   }

//   void serialize( Serializer& serializer ) const
//   {
//     header.serialize( serializer );
//     for ( const auto& x : payload ) {
//       serializer.buffer( x );
//     }
//   }
// };



// using InternetDatagram = IPv4Datagram;

// // Ethernet frame header
// struct EthernetHeader
// {
//   static constexpr size_t LENGTH = 14;         //!< Ethernet header length in bytes
//   static constexpr uint16_t TYPE_IPv4 = 0x800; //!< Type number for [IPv4](\ref rfc::rfc791)
//   static constexpr uint16_t TYPE_ARP = 0x806;  //!< Type number for [ARP](\ref rfc::rfc826)

//   EthernetAddress dst;
//   EthernetAddress src;
//   uint16_t type;

//   // Return a string containing a header in human-readable format
//   std::string to_string() const;

//   void parse( Parser& parser );
//   void serialize( Serializer& serializer ) const;
// };


// struct EthernetFrame
// {
//   EthernetHeader header {};
//   std::vector<Buffer> payload {};

//   void parse( Parser& parser )
//   {
//     header.parse( parser );
//     parser.all_remaining( payload );
//   }

//   void serialize( Serializer& serializer ) const
//   {
//     header.serialize( serializer );
//     serializer.buffer( payload );
//   }
// };

void NetworkInterface::update_maps (uint32_t ip_addr, EthernetAddress MAC_address) {
    map<uint32_t, pair<EthernetAddress, uint32_t> >::iterator iter1;
    iter1 = ip_addr_to_ethernet_addr.find(ip_addr);
    if (iter1 != ip_addr_to_ethernet_addr.end()) {
      iter1->second.first = MAC_address;
      iter1->second.second = 0;
        
    } else {
        pair<EthernetAddress, uint32_t> tmp = make_pair(MAC_address, 0);
        ip_addr_to_ethernet_addr[ip_addr] = tmp;
    }


    map<uint32_t, pair<queue<InternetDatagram>, uint32_t>>::iterator iter2;
    iter2 = ip_addr_to_ethernet_datagram_queue.find(ip_addr);
    if (iter2 != ip_addr_to_ethernet_datagram_queue.end()) {
      while (!iter2->second.first.empty()) {
          InternetDatagram dgram = iter2->second.first.front();
          iter2->second.first.pop();
          EthernetFrame frame;
          frame.header().src = ethernet_address_; // EthernetAddress
          frame.header().dst = MAC_address.value();
          frame.header().type = EthernetHeader::TYPE_IPv4;

          frame.payload = serialize(dgram);
          maybe_send_queue.push(frame);
        }
    }
    ip_addr_to_ethernet_datagram_queue.erase(ip_addr);
}


    

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // if (!valid_frame(frame)) return nullopt;
  if (frame.header().type == EthernetHeader::TYPE_IPv4) {
      InternetDatagram dgram;
      // if ( parse( dgram, frame.payload ) ) {
      if (parse(dgram, frame.payload)) {
        return dgram;
      }
  } else {
      ARPMessage recv_ARP;
      if (parse(recv_ARP, frame.payload)) {
        update_maps(recv_ARP.sender_ip_address, recv_ARP.sender_ethernet_address);
        if (recv_ARP.opcode == ARPMessage::OPCODE_REQUEST && recv_ARP.target_ip_address == ip_address_.ipv4_numeric()) {
          // send reply
          EthernetFrame frame;
          frame.header().type = EthernetHeader::TYPE_ARP;
          frame.header().src = ethernet_address_;
          frame.header().dst = recv_ARP.sender_ethernet_address;
          ARPMessage reply_ARP;

          reply_ARP.opcode = ARPMessage::OPCODE_REPLY;
          reply_ARP.sender_ethernet_address = ethernet_address_;
          reply_ARP.sender_ip_address = ip_address_.ipv4_numeric();
          reply_ARP.target_ethernet_address = recv_ARP.sender_ethernet_address;
          reply_ARP.target_ip_address = ip_addr;
          frame.payload = serialize(reply_ARP);
          maybe_send_queue.push(frame);
        }
      }
  }
  return nullopt; 
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  map<uint32_t, pair<EthernetAddress, uint32_t>>::iterator iter1;
  queue<uint32_t> expired_ip_addresses;
  for (iter1 = ip_addr_to_ethernet_addr.begin(); iter1 != ip_addr_to_ethernet_addr.end(); iter1++) {
      iter1->second.second += ms_since_last_tick;
      if (iter1->second.second >= NetworkInterface::MAX_CACHE_TIME) {
          expired_ip_addresses.push(iter1->first);
      }
  }

  map<uint32_t, pair<queue<InternetDatagram>, uint32_t> >::iterator iter2; 
  for (iter2 = ip_addr_to_ethernet_datagram_queue.begin(); iter2 != ip_addr_to_ethernet_datagram_queue.end(); iter2++) {
      iter2->second.second += ms_since_last_tick;
  }
  
  while (!expired_ip_addresses.empty()) {
      uint32_t ip = expired_ip_addresses.front();
      expired_ip_addresses.pop();
      ip_addr_to_ethernet_addr.erase(ip);
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if (!maybe_send_queue.empty()) {
    EthernetFrame tmp = maybe_send_queue.front();
    maybe_send_queue.pop();
    return tmp;
  }
  return nullopt;
}
