#include "tcp_receiver.hh"
#include "iostream"
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  if ( !syn && !message.SYN ) {
    return;
  }

  // if first SYN
  if ( message.SYN && !syn ) {
    syn = true;
    isn = message.seqno;
  }

  // need to record fin cause it may arrive in the middle of the stream
  // + 1 for SYN
  uint64_t last_ack_abs_seqno = inbound_stream.bytes_pushed() + 1;
  if ( fin && inbound_stream.is_closed() ) {
    // is_closed ensures that fin is assembled
    last_ack_abs_seqno++;
  }

  uint64_t checkpoint = inbound_stream.bytes_pushed();
  // Convert seqno → absolute seqno

  uint64_t abs_seqno = message.seqno.unwrap( isn, checkpoint );
  uint64_t window_size = inbound_stream.available_capacity();

  string data = message.payload;

  // minus SYN
  uint64_t idx = abs_seqno;
  if ( idx >= 1 ) {
    idx--;
  }

  if ( abs_seqno < last_ack_abs_seqno + window_size
       && abs_seqno + message.sequence_length() > last_ack_abs_seqno ) {
    // overlap with the window
    if ( syn && message.FIN ) {
      // FIN inside the window, can be assembled
      fin = true;
    }
  } else {
    return;
  }

  reassembler.insert( idx, data, fin, inbound_stream );
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  uint64_t last_ack_abs_seqno = inbound_stream.bytes_pushed() + 1;
  if ( fin && inbound_stream.is_closed() ) {
    // is_closed ensures that fin is assembled
    last_ack_abs_seqno++;
  }

  uint64_t window_size = inbound_stream.available_capacity();

  if ( window_size >= 65536 ) {
    window_size = 65535;
  }

  if ( syn ) {
    return TCPReceiverMessage { make_optional<Wrap32>( Wrap32( 0 ).wrap( last_ack_abs_seqno, isn ) ),
                                uint16_t( window_size ) };
  } else {
    return TCPReceiverMessage { nullopt, uint16_t( window_size ) };
  }
}
