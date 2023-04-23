#include "tcp_receiver.hh"
#include "iostream"
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  if (!syn && !message.SYN) {
    return ;
  }

  // if first SYN
  if (message.SYN && !syn) {
    syn = true;
    isn = message.seqno;
  }

  // need to record fin cause it may arrive in the middle of the stream
  // + 1 for SYN
  uint64_t last_ack_abs_seqno = inbound_stream.bytes_pushed() + 1;
  if (fin) {
    last_ack_abs_seqno ++;
  }

  uint64_t checkpoint = inbound_stream.bytes_pushed();
  // Convert seqno → absolute seqno

  uint64_t abs_seqno = message.seqno.unwrap(isn, checkpoint);
  uint64_t window_size = inbound_stream.available_capacity();


  string data = message.payload;

  // minus SYN
  uint64_t idx = abs_seqno;
  if (idx >= 1) {
    idx --;
  }

  
  // cout<<"abs_seqno: "<<abs_seqno<<endl;
  // cout<<"last_ack_abs_seqno: "<<last_ack_abs_seqno<<endl;
  // cout<<"window_size: "<<window_size<<endl;
  // cout<<"message.sequence_length(): "<<message.sequence_length()<<endl;
  if (abs_seqno < last_ack_abs_seqno + window_size && abs_seqno + message.sequence_length() > last_ack_abs_seqno) {
    // overlap with the window
    // if (inbound_stream.is_closed()) {
    //   fin = true;
    // }
    if (syn && message.FIN && abs_seqno + message.sequence_length() < last_ack_abs_seqno + window_size) {
      // FIN inside the window, can be assembled
      fin = true;
    }
  }
  else {
    return ;
  }
  // cout<<"idx: "<<idx<<endl;
  // cout<<"data: "<<data<<endl;
  // cout<<"fin: "<<fin<<endl;
  // cout<<"inbound_stream.available_capacity(): "<<inbound_stream.available_capacity()<<endl;
  // cout<<"reassembler.buffer().size(): "<<reassembler.getb().size()<<endl;

  // idx: 18446744073709551615
  // data: 
  // fin: 1
  // inbound_stream.available_capacity(): 4000
  // reassembler.buffer().size(): 0

  reassembler.insert( idx, data, fin, inbound_stream );
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  uint64_t last_ack_abs_seqno = inbound_stream.bytes_pushed() + 1;
  if (fin && inbound_stream.is_closed()) {
    last_ack_abs_seqno ++;
  }

  // segment with FIN (but can't be assembled yet)

  uint64_t window_size = inbound_stream.available_capacity();

  if (window_size >= 65536) {
    window_size = 65535;
  }

  // TCPReceiverMessage message;
  // message.window_size = uint16_t(window_size);

  // TCPReceiverMessage ret = {make_optional<Wrap32>(wrap(last_ack_abs_seqno)), uint16_t(window_size)};

  // Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
  // cout<<"last_ack_abs_seqno: "<<last_ack_abs_seqno<<endl;
  // cout<<"window_size: "<<window_size<<endl;
  if (syn) {
    return TCPReceiverMessage {make_optional<Wrap32>(Wrap32(0).wrap(last_ack_abs_seqno, isn)), uint16_t(window_size)};
    // message.ackno = make_optional<Wrap32>(Wrap32{0}.wrap(last_ack_abs_seqno, *isn));
    // return message;
  }
  else {
    // return message;
    return TCPReceiverMessage {nullopt, uint16_t(window_size)};
  }

//   // Your code here.
//   (void)inbound_stream;
//   return {};


//   /*
//  * The TCPReceiverMessage structure contains the information sent from a TCP receiver to its sender.
//  *
//  * It contains two fields:
//  *
//  * 1) The acknowledgment number (ackno): the *next* sequence number needed by the TCP Receiver.
//  *    This is an optional field that is empty if the TCPReceiver hasn't yet received the Initial Sequence Number.
//  *
//  * 2) The window size. This is the number of sequence numbers that the TCP receiver is interested
//  *    to receive, starting from the ackno if present. The maximum value is 65,535 (UINT16_MAX from
//  *    the <cstdint> header).
//  */

// struct TCPReceiverMessage
// {
//   std::optional<Wrap32> ackno {};
//   uint16_t window_size {};
// };
}
