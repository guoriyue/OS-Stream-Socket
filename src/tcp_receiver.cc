#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  (void)message;
  (void)reassembler;
  (void)inbound_stream;

  // struct TCPSenderMessage
  // {
  //   Wrap32 seqno { 0 };
  //   bool SYN { false };
  //   Buffer payload {};
  //   bool FIN { false };

  //   // How many sequence numbers does this segment use?
  //   size_t sequence_length() const { return SYN + payload.size() + FIN; }
  // };


  // // if SYN
  // if (message.SYN && !isn) {
  //   *isn = message.seqno;
  // }

}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  (void)inbound_stream;
  return {};
}
