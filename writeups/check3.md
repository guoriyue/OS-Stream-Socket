Checkpoint 3 Writeup
====================

My name: Mingfei Guo

My SUNet ID: mfguo

I collaborated with: None

I would like to thank/reward these classmates for their help: Everyone on Ed

This checkpoint took me about 20 hours to do. I did not attend the lab session.

Program Structure and Design of the TCPSender:

New Variables:

std::deque<TCPSenderMessage> send_messages_ = {};
- This is the queue of messages that is considered as already sent. But it
will be really sent when maybe_send() is called. It's just used as a buffer
to transfer messages to maybe_send(), we don't use it to check message status
or update ack information.

std::deque<TCPSenderMessage> outstanding_messages_ = {};
- This is the queue of messages that is considered as already sent and not
yet acknowledged. We use it to update ack information.

uint64_t next_abs_seqno_ = 0;
- This is the next absolute sequence number to be sent. Updated with the sender
thinks it's sending a message, which means pushing a message to the send_messages_
and outstanding_messages_ queue.

uint64_t abs_ackno_ = 0;
- This is the absolute ack number that the sender has received. Updated when fully
acknowledge a new message.

uint64_t sequence_numbers_in_flight_ = 0;
- This is the number of sequence numbers that are in flight. Updated when sending
a new message or receiving a new ack.

uint64_t consecutive_retransmissions_ = 0;
- This is used to record the number of consecutive retransmissions. Set to 0 when
receiving a new ack.

uint64_t retransmission_timeout_ = initial_RTO_ms_;
- The exponential backoff timeout.

uint16_t receiver_window_size_ = 1; // receiver window size int16
uint16_t receiver_window_space_ = 1;
- The receiver window size and the receiver window space. The receiver window size
is the actual window size that the receiver sends to the sender, won't be changed.
The receiver window space is the window size minus the sequence number in flight,
representing the free space left in the receiver window. Both are initialized to 1.

size_t time_elapsed_ = 0;
- The time elapsed since the last ack is received, or the retransmission_timeout_
is updated. Actually when the last ack is received, the retransmission_timeout_ is
updated at the same time. We can conclude that the time_elapsed_ is the time elapsed
since the last retransmission_timeout_ is updated.

bool syn = false;
- Whether the sender has sent a SYN message.

bool fin = false;
- Whether the sender has sent a FIN message.

bool free_window = false;
- Whether the receiver window size and the receiver window space are both 0 and
the receiver give a free window size of 1.

New Functions:

void update_status( TCPSenderMessage sender_message );
- Used to update variables related with sending. For example, outstanding_messages_,
send_messages_, consecutive_retransmissions_, next_abs_seqno_ and sequence_numbers_in_flight_.

Implementation Challenges:

Edge cases.

1. Sometimes the receiver window space is 0, but the sender should still be able to send.
We think this is related with initializing the receiver window space and the receiver 
window space to be 1. For example, SYN + FIN. After sending SYN, the receiver window space
is 0, but the sender should still be able to send the FIN. This is a special case, so we
handle it separately in the code when syn is first set to true.

2. Similar with point 1, sometimes the window size might be changed to be smaller than the
sequence number in flight. To avoid getting a negative window space (which is actually a
very large number because of uint), we set the window space to be 0 when the new window 
size is smaller than the sequence number in flight.

3. The receiver needs to give a free window size of 1 when the actual window size is 0.
To handle this, we set both the receiver window size and the receiver window space to 1
when both of them are 0. In this case, we shouldn't perform back off, so we add a label
free_window to distinguish this case from the case that the real receiver window size is 1.

4. In receiver function, we need to first check whether the ackno is valid, and then try
to receive packets that can be fully acknowledged with the received ackno. However, when
we check whether the ackno is valid, we just check if the ackno is greater than the
sequence number of the first packet in flight. But when we try to receive packets, we
need to make sure that the ackno is greater than the sequence number of the first packet
in flight plus the length of the first packet in flight. This is because the definition
of valid and the definition of fully acknowledged are different. We still need to update
other information like the receiver window size even if the first packet in flight is not
fully acknowledged. However, this different makes the code a little bit confusing.

Also, another tricky thing is that we have to use deque here, instead of vector. Because
when we push back to a vector, we get error: passing ‘const std::vector<TCPSenderMessage>’ 
as ‘this’ argument discards qualifiers [-fpermissive]. However, we're not defining the
vector as const, so we actually don't know why this error happens. To solve this we have to
change TCPSenderMessage to TCPSenderMessage*, which is quite annoying. Then we tried to use
deque as an alternative, and it works. But we define both vector and deque the same, and 
both push_back of vector require const value_type& __x. So we don't know why this solves
the problem.

Remaining Bugs:
None

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
