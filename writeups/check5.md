Checkpoint 5 Writeup
====================

My name: Mingfei Guo

My SUNet ID: mfguo

I collaborated with: None

I would like to thank/reward these classmates for their help: Everyone on Ed

This checkpoint took me about 6 hours to do. I did attend the lab session.

Program Structure and Design of the Router:
New class:
- RouteRecord:
    This class is used to record the route entry of the route table. It contains 5
    members: route_prefix_, prefix_length_, next_hop_, interface_num_, mask.
    We would initialize a RouteRecord object with route_prefix_, prefix_length_,
    next_hop_, interface_num_ and calculate the mask in the constructor. The mask
    is determined by the prefix_length_.

New fields in Router class:
- std::vector<RouteRecord> route_table_ = {};
    This vector is used to store the route table. Each element in the vector is a
    RouteRecord object.

Implementation Challenges:
1. Compute the mask of the route entry. For prefix_length_ N, the mask should be
    0xffffffff << (32 - N) when N is not 0. Because shift exponent 32 is too 
    large for 32-bit type unsigned int. When N is 0, the mask should be 0, we
    don't need to calculate the mask in this case.
    Initially I first use 1 << (32 - N) - 1 to get exactly N 1s, then shift it
    left by 32 - N bits. However, this expression is not needed because the extra
    1s will be shifted out, so we can just use 0xffffffff << (32 - N) to get the
    mask.

2. Update checksum when TTL is decreased. Whenever the header is changed, we need
    to update the checksum.

3. Loop through interfaces by reference. Otherwise, the interface will be copied
    and the changes on the interface will not be reflected on the original one.
    That's why the same datagram would be popped from the queue multiple times
    when we call the maybe_receive function. maybe_receive on a copy will return 
    the copy of a datagram but not flush it from the underlying class attribute.

Remaining Bugs:
None.

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
