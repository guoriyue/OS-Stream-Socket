#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "parser.hh"
#include <queue>
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
  optional<pair<EthernetAddress, uint32_t>> MAC_address = map_ip_addr_to_ethernet_addr( next_hop_ip );
  if ( MAC_address.has_value() ) {
    EthernetFrame frame;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.header.src = ethernet_address_; // EthernetAddress
    frame.header.dst = MAC_address.value().first;

    frame.payload = serialize( dgram );
    maybe_send_queue.push( frame );
  } else {
    optional<pair<queue<InternetDatagram>, uint32_t>> ethernet_datagram_queue
      = map_ip_addr_to_ethernet_datagram_queue( next_hop_ip );
    bool ARP_request = false;
    if ( ethernet_datagram_queue.has_value() ) {
      ethernet_datagram_queue.value().first.push( dgram );
      if ( ethernet_datagram_queue.value().second >= 5000 ) {
        // If the network interface already sent an ARP request about the same IP address in the last
        //  five seconds, don’t send a second request—just wait for a reply to the first one.
        ARP_request = true;
      }
    } else {
      pair<queue<InternetDatagram>, uint32_t> new_ethernet_datagram_queue
        = make_pair( queue<InternetDatagram>(), 0 );
      new_ethernet_datagram_queue.first.push( dgram );
      ip_addr_to_ethernet_datagram_queue[next_hop_ip] = new_ethernet_datagram_queue;
      ARP_request = true;
    }
    if ( ARP_request ) {
      EthernetFrame frame;
      frame.header.type = EthernetHeader::TYPE_ARP;
      frame.header.src = ethernet_address_;
      frame.header.dst = ETHERNET_BROADCAST;

      ARPMessage send_ARP;
      send_ARP.opcode = ARPMessage::OPCODE_REQUEST;
      send_ARP.sender_ethernet_address = ethernet_address_;
      send_ARP.sender_ip_address = ip_address_.ipv4_numeric();
      send_ARP.target_ip_address = next_hop_ip;
      frame.payload = serialize( send_ARP );
      maybe_send_queue.push( frame );
    }
  }
}

optional<pair<EthernetAddress, uint32_t>> NetworkInterface::map_ip_addr_to_ethernet_addr( uint32_t ip_addr )
{
  map<uint32_t, pair<EthernetAddress, uint32_t>>::iterator iter;
  iter = ip_addr_to_ethernet_addr.find( ip_addr );
  if ( iter != ip_addr_to_ethernet_addr.end() ) {
    return iter->second;
  }
  return nullopt;
}

optional<pair<queue<InternetDatagram>, uint32_t>> NetworkInterface::map_ip_addr_to_ethernet_datagram_queue(
  uint32_t ip_addr )
{
  map<uint32_t, pair<queue<InternetDatagram>, uint32_t>>::iterator iter;
  iter = ip_addr_to_ethernet_datagram_queue.find( ip_addr );
  if ( iter != ip_addr_to_ethernet_datagram_queue.end() ) {
    return iter->second;
  }
  return nullopt;
}

void NetworkInterface::update_maps( uint32_t ip_addr, EthernetAddress MAC_address )
{
  map<uint32_t, pair<EthernetAddress, uint32_t>>::iterator iter1;
  iter1 = ip_addr_to_ethernet_addr.find( ip_addr );
  if ( iter1 != ip_addr_to_ethernet_addr.end() ) {
    iter1->second.first = MAC_address;
    iter1->second.second = 0;

  } else {
    pair<EthernetAddress, uint32_t> tmp = make_pair( MAC_address, 0 );
    ip_addr_to_ethernet_addr[ip_addr] = tmp;
  }

  map<uint32_t, pair<queue<InternetDatagram>, uint32_t>>::iterator iter2;
  iter2 = ip_addr_to_ethernet_datagram_queue.find( ip_addr );
  if ( iter2 != ip_addr_to_ethernet_datagram_queue.end() ) {
    while ( !iter2->second.first.empty() ) {
      InternetDatagram dgram = iter2->second.first.front();
      iter2->second.first.pop();
      EthernetFrame frame;
      frame.header.type = EthernetHeader::TYPE_IPv4;
      frame.header.src = ethernet_address_; // EthernetAddress
      frame.header.dst = MAC_address;

      frame.payload = serialize( dgram );
      maybe_send_queue.push( frame );
    }
  }
  ip_addr_to_ethernet_datagram_queue.erase( ip_addr );
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.dst == ETHERNET_BROADCAST || frame.header.dst == ethernet_address_ ) {
    // valid frame
  } else {
    return nullopt;
  }

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) ) {
      return dgram;
    }
  } else {
    ARPMessage recv_ARP;
    if ( parse( recv_ARP, frame.payload ) ) {
      update_maps( recv_ARP.sender_ip_address, recv_ARP.sender_ethernet_address );
      if ( recv_ARP.opcode == ARPMessage::OPCODE_REQUEST
           && recv_ARP.target_ip_address == ip_address_.ipv4_numeric() ) {
        // send reply
        EthernetFrame send_frame;
        send_frame.header.type = EthernetHeader::TYPE_ARP;
        send_frame.header.src = ethernet_address_;
        send_frame.header.dst = recv_ARP.sender_ethernet_address;
        ARPMessage reply_ARP;

        reply_ARP.opcode = ARPMessage::OPCODE_REPLY;
        reply_ARP.sender_ethernet_address = ethernet_address_;
        reply_ARP.sender_ip_address = ip_address_.ipv4_numeric();
        reply_ARP.target_ethernet_address = recv_ARP.sender_ethernet_address;
        reply_ARP.target_ip_address = recv_ARP.sender_ip_address;
        send_frame.payload = serialize( reply_ARP );
        maybe_send_queue.push( send_frame );
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
  for ( iter1 = ip_addr_to_ethernet_addr.begin(); iter1 != ip_addr_to_ethernet_addr.end(); iter1++ ) {
    iter1->second.second += ms_since_last_tick;
    if ( iter1->second.second >= 30000 ) {
      // remember the mapping between the sender’s IP address and Ethernet address for 30 seconds
      expired_ip_addresses.push( iter1->first );
    }
  }

  map<uint32_t, pair<queue<InternetDatagram>, uint32_t>>::iterator iter2;
  for ( iter2 = ip_addr_to_ethernet_datagram_queue.begin(); iter2 != ip_addr_to_ethernet_datagram_queue.end();
        iter2++ ) {
    iter2->second.second += ms_since_last_tick;
  }

  while ( !expired_ip_addresses.empty() ) {
    uint32_t ip = expired_ip_addresses.front();
    expired_ip_addresses.pop();
    ip_addr_to_ethernet_addr.erase( ip );
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if ( !maybe_send_queue.empty() ) {
    EthernetFrame tmp = maybe_send_queue.front();
    maybe_send_queue.pop();
    return tmp;
  }
  return nullopt;
}
