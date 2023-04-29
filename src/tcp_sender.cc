#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return sequence_numbers_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  return {};
}

void TCPSender::push( Reader& outbound_stream )
{
  // generates as many TCPSenderMessages as possible, as long as there 
  // are new bytes to be read and space available in the window

  // // Your code here.
  // (void)outbound_stream;

  TCPSenderMessage sender_message;
  sender_message.payload = Buffer(outbound_stream.peek());

  // outbound_stream.pop()
  // std::string_view outbound_stream.peek()
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // generate and send a zero-length message with the sequence number set correctly
  TCPSenderMessage sender_message;
  sender_message.seqno = Wrap32(0).wrap(next_abs_seqno_, isn_);
  send_messages_.push_back(sender_message);
  return sender_message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  uint64_t abs_ackno = Wrap32(msg.ackno).unwrap(isn_, next_abs_seqno_);
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  time_elapsed_ += ms_since_last_tick;
  if (time_elapsed_ >= retransmission_timeout_) {
    send_messages_.push_back(outstanding_messages_[0]);
    if (receiver_window_size_) {
      consecutive_retransmissions_+=1;
      retransmission_timeout_ *= 2;
    }
    time_elapsed_ = 0;
  }
}

// struct TCPSenderMessage
// {
//   Wrap32 seqno { 0 };
//   bool SYN { false };
//   Buffer payload {};
//   bool FIN { false };

//   // How many sequence numbers does this segment use?
//   size_t sequence_length() const { return SYN + payload.size() + FIN; }
// };





// struct TCPReceiverMessage
// {
//   std::optional<Wrap32> ackno {};
//   uint16_t window_size {};
// };
