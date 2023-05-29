#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  route_table_.push_back( RouteRecord( route_prefix, prefix_length, next_hop, interface_num ) );
}

void Router::route()
{
  for ( auto& interface : interfaces_ ) {
    while ( true ) {
      std::optional<InternetDatagram> datagram_maybe_send_ = interface.maybe_receive();
      if ( datagram_maybe_send_.has_value() ) {
        InternetDatagram datagram_send = datagram_maybe_send_.value();

        if ( datagram_send.header.ttl <= 1 ) {
          continue;
        }

        uint32_t destination = datagram_send.header.dst;
        optional<Address> next_hop = nullopt;
        int interface_num = -1;
        int max_match_len = -1;
        for ( auto& route_record : route_table_ ) {
          const uint32_t mask = route_record.mask;
          if ( ( destination & mask ) == route_record.route_prefix_ ) {
            if ( max_match_len < route_record.prefix_length_ ) {
              next_hop = route_record.next_hop_;
              interface_num = route_record.interface_num_;
              max_match_len = route_record.prefix_length_;
            }
          }
        }
        if ( interface_num == -1 ) {
          continue;
        }

        datagram_send.header.ttl -= 1;
        datagram_send.header.compute_checksum();

        if ( next_hop.has_value() ) {
          interfaces_[interface_num].send_datagram( datagram_send, next_hop.value() );
        } else {

          interfaces_[interface_num].send_datagram( datagram_send, Address::from_ipv4_numeric( destination ) );
        }

      } else {
        break;
      }
    }
  }
}
