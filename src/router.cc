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


  route_table_.push_back(RouteRecord(route_prefix, prefix_length, next_hop, interface_num));
}


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





// // IPv4 Internet datagram header (note: IP options are not supported)
// struct IPv4Header
// {
//   static constexpr size_t LENGTH = 20;        // IPv4 header length, not including options
//   static constexpr uint8_t DEFAULT_TTL = 128; // A reasonable default TTL value
//   static constexpr uint8_t PROTO_TCP = 6;     // Protocol number for TCP

//   static constexpr uint64_t serialized_length() { return LENGTH; }

//   /*
//    *   0                   1                   2                   3
//    *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    *  |Version|  IHL  |Type of Service|          Total Length         |
//    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    *  |         Identification        |Flags|      Fragment Offset    |
//    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    *  |  Time to Live |    Protocol   |         Header Checksum       |
//    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    *  |                       Source Address                          |
//    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    *  |                    Destination Address                        |
//    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    *  |                    Options                    |    Padding    |
//    *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    */

//   // IPv4 Header fields
//   uint8_t ver = 4;           // IP version
//   uint8_t hlen = LENGTH / 4; // header length (multiples of 32 bits)
//   uint8_t tos = 0;           // type of service
//   uint16_t len = 0;          // total length of packet
//   uint16_t id = 0;           // identification number
//   bool df = true;            // don't fragment flag
//   bool mf = false;           // more fragments flag
//   uint16_t offset = 0;       // fragment offset field
//   uint8_t ttl = DEFAULT_TTL; // time to live field
//   uint8_t proto = PROTO_TCP; // protocol field
//   uint16_t cksum = 0;        // checksum field
//   uint32_t src = 0;          // src address
//   uint32_t dst = 0;          // dst address

//   // Length of the payload
//   uint16_t payload_length() const;

//   // Pseudo-header's contribution to the TCP checksum
//   uint32_t pseudo_checksum() const;

//   // Set checksum to correct value
//   void compute_checksum();

//   // Return a string containing a header in human-readable format
//   std::string to_string() const;

//   void parse( Parser& parser );
//   void serialize( Serializer& serializer ) const;
// };


void Router::route() {
  for (int i=0;i<interfaces_.length();i++) {
    AsyncNetworkInterface interface = interfaces_[i];
        // auto &queue = interface.datagrams_out();
        std::queue<InternetDatagram> datagrams_send_ = interface.datagrams_in_;
        while (!datagrams_send_.empty()) {
            InternetDatagram datagram_send = datagrams_send_.front();
            datagrams_send_.pop();


            if (datagram_send.header.ttl == 0) {
              return;
            }


            uint32_t destination = datagrams_send_.header.dst;
            RouteRecord match_route_record;
            int max_match_len = 0;
            for (auto& route_record:route_table_) {
                const uint32_t mask = route_record.mask;
                if ((destination & mask) == route_record.route_prefix_) {
                    if (max_match_len < route_record.prefix_length_)
                    match_route_record = route_record;
                    max_match_len = route_record.prefix_length_;
                }
            }
            if (max_match_len == 0){
                return;
            }

            datagram_send.header.ttl -= 1;
            const optional<Address> next_hop = match_route_record.next_hop_;
            const size_t interface_num = match_route_record.interface_num_;
            if (next_hop.has_value()) {
                interfaces_[interface_num].send_datagram(datagram_send, next_hop.value());
            } else {
                interfaces_[interface_num].send_datagram(datagram_send, Address::from_ipv4_numeric(destination));
            }

    

            
        }
    }
}
