Checkpoint 1 Writeup
====================

My name: Mingfei Guo

My SUNet ID: mfguo

I collaborated with: None

I would like to thank/reward these classmates for their help: Thanks to everyone on Ed.

This lab took me about 10 hours to do. I did not attend the lab session.

Program Structure and Design of the Reassembler:

I add 5 private variables in Reassembler.

uint64_t base_index = 0;
- The base index of the stream, starting with 0.
uint64_t unassembled_size = 0;
- The size of unassembled bytes.
std::deque<char> buffer = {};
- The buffer is used to save input substrings.
std::deque<bool> flag = {};
- Corresponding flags for each character in the buffer.
  False means unassembled.
bool eof = false;
- True if already get the last substr is_last_substring.

To maintain continuity, I need to pop the front item and 
push a default value at the end. This ensures the buffer's 
beginning is always base_index (one important invariant). 
Since deque prvides O(1) complexity for pop_front and
push_back, I choose to use deque. Also, I update base_index
after assembled substrings can be pushed to the Writer.

When I get the is_last_substring signal, it's possible 
that I haven't assembled its previous substrings. Therefore, 
I add a bool value called "eof" to indicate that I already 
know the end. Since I might have assembled the stream partially 
before the end, this bool guarantees that after getting the true 
end, I must have finished assembling when the unassembled_size is 0.

If first_index > base_index, it's just a non-continuous substring.
I iterate through the substring and if there's a hole in the flag 
(which means a false unassembled flag value), I put the substring 
in the correct place in the buffer and update the flag to true.
If first_index < base_index, it's possible that the substring 
can be discarded. I check the substring's end to see if I should 
truncate or discard the whole substring. If truncation is needed, 
I iterate through the substring and add the truncated part. In
this case, the offset should be added to the substring since
it is truncated.

Implementation Challenges:
I think it's tricky to update base_index and move forward the
assembling deque. I need to keep in mind the invariant that the 
beginning of buffer represents the base_index. To put the substring 
in correct place in the buffer, I need to distinguish between two 
situations. For first_index > base_index, the offset belongs to 
buffer and flag while for first_index < base_index, the offset 
belongs to substring data. The code might looks more elegant if
we use a single formula to represent these two cases. For example,
we can use flag[i + max(first_index - base_index, 0)] and
data[i + max(base_index - first_index, 0)]. However, I think
this makes the code difficult to understand. Therefore, I choose
to iterate through these two cases separately to keep the code clear.



Remaining Bugs:
None.

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
