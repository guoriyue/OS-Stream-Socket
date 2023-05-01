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

  if ( send_messages_.empty() ) {
    return nullopt;
  }
  TCPSenderMessage sender_message = send_messages_.front();
  send_messages_.pop_front();
  // if ( !timing ) {
  //   timing = true;
  //   time_elapsed_ = 0;
  // }
  return make_optional<TCPSenderMessage>( sender_message );
}

void TCPSender::push( Reader& outbound_stream )
{
  std::cout << "push receiver_window_size_ " << receiver_window_size_ << std::endl;
  std::cout << "push receiver_window_space_ " << receiver_window_space_ << std::endl;
  if ( !syn ) {
    // SYN sent after first push
    syn = true;
    TCPSenderMessage sender_message;
    sender_message.SYN = true;
    sender_message.seqno = Wrap32( 0 ).wrap( next_abs_seqno_, isn_ );
    next_abs_seqno_++;
    sequence_numbers_in_flight_++;
    // std::cout << "syn push sequence_numbers_in_flight_ " << sequence_numbers_in_flight_ << std::endl;
    receiver_window_space_--;

    std::cout << "true syn  outbound_stream.bytes_buffered()" << outbound_stream.bytes_buffered() << std::endl;
    std::cout << "true syn receiver_window_space_" << receiver_window_space_ << std::endl;
    std::cout << "true syn receiver_window_size_" << receiver_window_size_ << std::endl;
    std::cout << "true syn outbound_stream.is_finished()" << outbound_stream.is_finished() << std::endl;
    if ( outbound_stream.is_finished() ) { // && receiver_window_space_){
      sender_message.FIN = true;
      fin = true;
      next_abs_seqno_++;
      sequence_numbers_in_flight_++;
      // receiver_window_space_--;
    }

    send_messages_.push_back( sender_message );

    outstanding_messages_.push_back( sender_message );
    return;
  }

  if ( fin ) {
    return;
  }

  if ( !syn ) {
    return;
  }

  // SYN has not been acked
  if ( !outstanding_messages_.empty() && outstanding_messages_[0].SYN ) {
    return;
  }

  if ( outbound_stream.bytes_buffered() ) {

    // if (receiver_window_size_==0){
    //   // When filling window, treat a '0' window size as equal to '1' but don't back off RTO" failed.
    //   TCPSenderMessage sender_message;
    //   size_t read_size=1;
    //   std::string read_out;
    //   read( outbound_stream, read_size, read_out );
    //   sender_message.payload = Buffer( read_out );

    //   sender_message.seqno = Wrap32( 0 ).wrap( next_abs_seqno_, isn_ );
    //   next_abs_seqno_ += sender_message.sequence_length();
    //   // std::cout << "before + sequence_numbers_in_flight_ " << sequence_numbers_in_flight_ << std::endl;
    //   // std::cout << "before + sender_message.sequence_length() " << sender_message.sequence_length() <<
    //   std::endl;

    //   sequence_numbers_in_flight_ += sender_message.sequence_length();
    //   // std::cout << "push sequence_numbers_in_flight_ " << sequence_numbers_in_flight_ << std::endl;
    //   if (receiver_window_space_)
    //   {
    //     receiver_window_space_--;
    //   }
    //   // receiver_window_space_ -= sender_message.sequence_length();

    //   send_messages_.push_back( sender_message );

    //   outstanding_messages_.push_back( sender_message );

    //   return;
    // }
    while ( ( receiver_window_space_ && outbound_stream.bytes_buffered() )
            || ( receiver_window_size_ == 0 && receiver_window_space_ == 0 ) ) {
      std::cout << "receiver_window_space_ " << receiver_window_space_ << std::endl;
      std::cout << "outbound_stream.bytes_buffered() " << outbound_stream.bytes_buffered() << std::endl;

      size_t read_size;
      if ( outbound_stream.bytes_buffered() < receiver_window_space_ ) {
        read_size = outbound_stream.bytes_buffered();
      } else {
        read_size = receiver_window_space_;
      }
      if ( read_size > TCPConfig::MAX_PAYLOAD_SIZE ) {
        read_size = TCPConfig::MAX_PAYLOAD_SIZE; // 1000
      }
      if ( receiver_window_size_ == 0 && receiver_window_space_ == 0 ) {
        receiver_window_size_ = 1;
        receiver_window_space_ = 1;
        read_size = 1;
        free_window = true;
      }
      TCPSenderMessage sender_message;

      // push_back(const value_type& __x)
      // error: passing ‘const std::vector<TCPSenderMessage>’ as ‘this’ argument discards qualifiers [-fpermissive]
      std::string read_out;
      read( outbound_stream, read_size, read_out );
      sender_message.payload = Buffer( read_out );
      std::cout << "read_out " << read_out << std::endl;
      std::cout << "outbound_stream.is_finished() " << outbound_stream.is_finished() << std::endl;
      if ( outbound_stream.is_finished() && receiver_window_space_ > read_size ) {
        sender_message.FIN = true;
        fin = true;
      }

      sender_message.seqno = Wrap32( 0 ).wrap( next_abs_seqno_, isn_ );
      next_abs_seqno_ += sender_message.sequence_length();
      // std::cout << "before + sequence_numbers_in_flight_ " << sequence_numbers_in_flight_ << std::endl;
      // std::cout << "before + sender_message.sequence_length() " << sender_message.sequence_length() << std::endl;

      sequence_numbers_in_flight_ += sender_message.sequence_length();
      // std::cout << "push sequence_numbers_in_flight_ " << sequence_numbers_in_flight_ << std::endl;

      receiver_window_space_ -= sender_message.sequence_length();

      send_messages_.push_back( sender_message );

      outstanding_messages_.push_back( sender_message );
    }
  } else {

    std::cout << "buffered none " << std::endl;
    if ( outbound_stream.is_finished() ) {
      if ( receiver_window_space_ || ( receiver_window_space_ == 0 && receiver_window_size_ == 0 ) ) {
        std::cout << "outbound_stream.is_finished() " << outbound_stream.is_finished() << std::endl;
        TCPSenderMessage sender_message;
        sender_message.seqno = Wrap32( 0 ).wrap( next_abs_seqno_, isn_ );
        sender_message.FIN = true;
        fin = true;
        next_abs_seqno_++;
        sequence_numbers_in_flight_++;
        receiver_window_space_--;
        send_messages_.push_back( sender_message );

        outstanding_messages_.push_back( sender_message );
      }
    }
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // generate and send a zero-length message with the sequence number set correctly
  TCPSenderMessage sender_message;
  sender_message.seqno = Wrap32( 0 ).wrap( next_abs_seqno_, isn_ );
  return sender_message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( !msg.ackno ) {
    return;
  }
  uint64_t abs_ackno = msg.ackno.value().unwrap( isn_, next_abs_seqno_ );
  std::cout << "!!!receive msg.window_size" << msg.window_size << std::endl;

  bool valid_ack = false;

  // std::cout << "abs_ackno " << abs_ackno << std::endl;
  // std::cout << "next_abs_seqno_ " << next_abs_seqno_ << std::endl;
  // std::cout << "outstanding_messages_[0].seqno.unwrap(isn_, next_abs_seqno_) "
  //           << outstanding_messages_[0].seqno.unwrap( isn_, next_abs_seqno_ ) << std::endl;
  if ( abs_ackno <= next_abs_seqno_ ) {
    if ( outstanding_messages_.empty() ) {
      valid_ack = true;
    } else {
      if ( abs_ackno >= outstanding_messages_[0].seqno.unwrap( isn_, next_abs_seqno_ ) ) {
        // + outstanding_messages_[0].sequence_length() ) {
        valid_ack = true;
      }
    }
  }

  if ( !valid_ack ) {
    return;
  }

  receiver_window_size_ = msg.window_size;
  receiver_window_space_ = msg.window_size;
  free_window = false;

  long unsigned int idx = 0;
  while ( idx < outstanding_messages_.size() ) {
    TCPSenderMessage sender_message = outstanding_messages_[idx];
    uint64_t sender_message_abs_ackno = sender_message.seqno.unwrap( isn_, next_abs_seqno_ );
    // if ( sender_message_abs_ackno + sender_message.sequence_length() <= abs_ackno ) {
    if ( sender_message_abs_ackno + sender_message.sequence_length() <= abs_ackno ) {
      std::cout << "before - sequence_numbers_in_flight_ " << sequence_numbers_in_flight_ << std::endl;
      std::cout << "before - sender_message.sequence_length() " << sender_message.sequence_length() << std::endl;

      sequence_numbers_in_flight_ -= sender_message.sequence_length();
      std::cout << "after - sequence_numbers_in_flight_ " << sequence_numbers_in_flight_ << std::endl;

      idx += 1;
    } else {
      break;
    }
  }
  if ( idx ) {
    time_elapsed_ = 0;
    retransmission_timeout_ = initial_RTO_ms_;
    consecutive_retransmissions_ = 0;
    outstanding_messages_.erase( outstanding_messages_.begin(), outstanding_messages_.begin() + idx );
  }
  if ( receiver_window_space_ >= sequence_numbers_in_flight_ ) {
    receiver_window_space_ = receiver_window_space_ - sequence_numbers_in_flight_;
  } else {
    receiver_window_space_ = 0;
  }
  std::cout << "!!!receiver_window_space_ after receive" << receiver_window_space_ << std::endl;

  // // receiver_window_space_ = msg.window_size - sequence_numbers_in_flight_;
  //   if (!outstanding_messages_.empty()) {
  //         receiver_window_space_ = msg.window_size - sequence_numbers_in_flight_;
  //         // (sequence_numbers_in_flight_ + outstanding_messages_[0].seqno.unwrap( isn_, next_abs_seqno_ )  -
  //         abs_ackno );

  //         // static_cast<uint16_t>(
  //         //     abs_ackno + static_cast<uint64_t>(window_size) -
  //         //     outstanding_messages_[0].seqno.unwrap( isn_, next_abs_seqno_ ) - sequence_numbers_in_flight_);
  //     }

  // if ( !sequence_numbers_in_flight_ )
  //   timing = false;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  if ( outstanding_messages_.empty() ) {
    return;
  }

  // if ( !timing ) {
  //   return;
  // }

  time_elapsed_ += ms_since_last_tick;
  std::cout << "time_elapsed_ " << time_elapsed_ << std::endl;
  std::cout << "ms_since_last_tick " << ms_since_last_tick << std::endl;
  std::cout << "retransmission_timeout_ " << retransmission_timeout_ << std::endl;
  if ( time_elapsed_ >= retransmission_timeout_ ) {
    send_messages_.push_back( outstanding_messages_[0] );
    std::cout << "receiver_window_size_ " << receiver_window_size_ << std::endl;
    if ( ( receiver_window_size_ != 0 || outstanding_messages_[0].SYN ) && !free_window ) {
      consecutive_retransmissions_ += 1;
      retransmission_timeout_ *= 2;
    }
    std::cout << "consecutive_retransmissions_ " << consecutive_retransmissions_ << std::endl;
    std::cout << "after retransmission_timeout_ " << retransmission_timeout_ << std::endl;
    time_elapsed_ = 0;
  }
}