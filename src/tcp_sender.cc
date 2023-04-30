#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <iostream>
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
  
  if (preparing_send_messages_.empty()) {
    return nullopt;
  }
  TCPSenderMessage sender_message = preparing_send_messages_.front();
  preparing_send_messages_.pop_front();
  outstanding_messages_.push_back(sender_message);
  return make_optional<TCPSenderMessage>(sender_message);
}

void TCPSender::push( Reader& outbound_stream )
{

  if (!syn) {
    // SYN sent after first push
    syn = true;
    TCPSenderMessage sender_message;
    sender_message.SYN = true;
    sender_message.seqno = Wrap32(0).wrap(next_abs_seqno_, isn_);
    next_abs_seqno_ ++;
    sequence_numbers_in_flight_ ++;
    receiver_window_space_ --;
    preparing_send_messages_.push_back(sender_message);
    return;
  }

  if (fin){
    return;
  }

  if(outbound_stream.bytes_buffered())
  {
    while (receiver_window_space_ && outbound_stream.bytes_buffered()) {
      size_t read_size;
      if (outbound_stream.bytes_buffered() < receiver_window_space_) {
        read_size = outbound_stream.bytes_buffered();
      }
      else{
        read_size =receiver_window_space_;
      }
      TCPSenderMessage sender_message;

      // push_back(const value_type& __x)
      // error: passing ‘const std::vector<TCPSenderMessage>’ as ‘this’ argument discards qualifiers [-fpermissive]
      std::string read_out;
      read(outbound_stream, read_size, read_out );
      sender_message.payload = Buffer(read_out);
      if (outbound_stream.is_finished() && receiver_window_space_ > read_size)
      {
        sender_message.FIN = true;
        fin = true;
      }

      sender_message.seqno = Wrap32(0).wrap(next_abs_seqno_, isn_);
      next_abs_seqno_ += sender_message.sequence_length();
      sequence_numbers_in_flight_ += sender_message.sequence_length();
      receiver_window_space_ -= sender_message.sequence_length();

      preparing_send_messages_.push_back(sender_message);
    }
  }

}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // generate and send a zero-length message with the sequence number set correctly
  TCPSenderMessage sender_message;
  sender_message.seqno = Wrap32(0).wrap(next_abs_seqno_, isn_);
  return sender_message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  uint64_t abs_ackno = msg.ackno.value().unwrap(isn_, next_abs_seqno_);
  receiver_window_size_ = msg.window_size;
  receiver_window_space_ = msg.window_size;

  bool valid_ack = false;

  std::cout<<"abs_ackno "<<abs_ackno<<std::endl;
  std::cout<<"next_abs_seqno_ "<<next_abs_seqno_<<std::endl;
  std::cout<<"outstanding_messages_[0].seqno.unwrap(isn_, next_abs_seqno_) "<<outstanding_messages_[0].seqno.unwrap(isn_, next_abs_seqno_)<<std::endl;
  if (abs_ackno <= next_abs_seqno_){
    if (outstanding_messages_.empty()){
      valid_ack = true;
    }
    else
    {
      if (abs_ackno >= outstanding_messages_[0].seqno.unwrap(isn_, next_abs_seqno_) + outstanding_messages_[0].sequence_length())
      {
        valid_ack = true;
      }
    }
  }

  if (!valid_ack)
  {
    return;
  }
  
  long unsigned int idx = 0;
  while (idx < outstanding_messages_.size()){
    TCPSenderMessage sender_message = outstanding_messages_[idx];
    uint64_t sender_message_abs_ackno = sender_message.seqno.unwrap(isn_, next_abs_seqno_);
    if (sender_message_abs_ackno<=abs_ackno){
      sequence_numbers_in_flight_ -= sender_message.sequence_length();
      idx += 1;
    }
    else{
      break;
    }
  }
  if (idx){
    time_elapsed_ = 0;
    retransmission_timeout_ = initial_RTO_ms_;
    consecutive_retransmissions_ = 0;
    outstanding_messages_.erase( outstanding_messages_.begin(), outstanding_messages_.begin() + idx );
  }

}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  std::cout<<"tick"<<std::endl;
  time_elapsed_ += ms_since_last_tick;
  if (time_elapsed_ >= retransmission_timeout_) {
    preparing_send_messages_.push_back(outstanding_messages_[0]);
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