Checkpoint 0 Writeup
====================

My name: Mingfei Guo

My SUNet ID: mfguo

I collaborated with: None

I would like to credit/thank these classmates for their help: Thank everyone on Ed.

This lab took me about 6 hours to do. I did not attend the lab session.

My secret code from section 2.1 was: 403338

Design choices:

I use a vector to implement stream buffer because it suppports constant time insert at the end, so it would be efficient to push buffer. Also, the elements of a vector are stored contiguously, so we can return all the bytes of the whole buffer conveniently in the peek function.

Deque provides constant time for both insert at the end and pop at the front, which makes it more efficient that vector. However, the elements of a deque are not stored contiguously, so it may not be possible to return a view of the whole buffer. Therefore, I choose to use vector.

Besides this, I also add two uint64_t variable, bytes_read and bytes_write to record the number of bytes we pushed and popped, and update them in Reader::pop and Writer::push respectively. Thus, I can implement bytes_pushed, bytes_popped and bytes_buffered easily. Moreover, I also create two bool flags, error_ and close_, to save current state. I update error_ flag in Writer::set_error and Reader::pop since it might throw an error if it is required to pop more bytes than the buffer size.

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
