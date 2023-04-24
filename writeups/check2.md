Checkpoint 2 Writeup
====================

My name: Mingfei Guo

My SUNet ID: mfguo

I collaborated with: None

I would like to thank/reward these classmates for their help: Thanks to everyone on Ed.

This lab took me about 12 hours to do. I did attend the lab session.

Program Structure and Design of the TCPReceiver and wrap/unwrap routines:

Wrap32::wrap:

We simply use the operator+ to add the seqno and isn. The type
is implicitly converted to Wrap32. 

Wrap32::unwrap:

We add an internal function absolute_delta to compute the absolute
delta of two Wrap32 numbers. We need to compute (seqno - isn) to 
get the absolute seqno. I will add more details about this in the
next section.

After we get the delta, we add or subtract multiplies of 2^32 to
get two possible results that are closest to the checkpoint value.
We then compare the two results and return the one that is closer
to the checkpoint value using the absolute_delta function.

TCPReceiver

Added fields:
Wrap32 isn = Wrap32(0);
- The initial seqno of the receiver, in Wrap32 format.
bool fin = false;
- Whether the FIN has been received, received doesn't mean assembled.
bool syn = false;
- Whether the SYN has been received.

TCPReceiver::receive:

In this function, we first check or initialize SYN. We only start
receiving data after SYN is received. Then we get the absolute seqno
using wrap/unwrap. To avoid pushing invalid data, we check if the
absolute seqno (along with the length of the payload) overlaps
with the window. If it does, we push the payload into the window.
Otherwise, we ignore the payload and return.

One thing to note is that we set fin to false as long as we receive
a FIN that is valid. However, since FIN may arrive in the middle
of the stream, we need to check if the writer is closed or not
to determine if FIN is assembled or not. This is very important
when we need to compute the last ackowledged absolute seqno.

TCPReceiver::send:

We return ackno and window_size. Don't forget to check is_closed
to get the correct ackno.

Also, when convert window_size from uint64_t to uint16_t, we need
to check if it is larger than 65535. If it is, we just return 65535.
This is because we only care about the window size in the range of
0 - 65535. It's good if we get a larger window size, but we don't
care about how large it actually is. We just need to send that
the window_size is the maximum value we have so return 65535 should
be fine.

Implementation Challenges:

Some of the challenges are already discussed in the design section.
Here are some other detailed problems I encountered:

wrap/unwrap:

When computing the absolute seqno (seqno - isn) in unwrap, initially,
I use the absolute_delta function. However, this doesn't make sense.
If we get a negative number, we should add 2^32 to wrap it in the
positive range, instead of get the absolute value.

Also, when trying to get 2^32 using bit operations, using 1 << 32 may
cause overflow. We should use 1ll << 32 instead.

TCPReceiver:

We need to determine if FIN is assembled or not. To do this, we can
check if the writer is closed because the writer would be closed immediately
after we assemble the FIN.

Remaining Bugs:
None.

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

- Optional: I made an extra test I think will be helpful in catching bugs: [describe where to find]
