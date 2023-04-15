#include "reassembler.hh"
#include <iostream>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  if (buffer.empty() && flag.empty()) {
    // initialization
    for (uint64_t i = 0; i < output.available_capacity(); i++) {
      buffer.push_back('\0');
      flag.push_back(false);
    }
  }

  uint64_t start_index = max(base_index, first_index);
  uint64_t end_index = min(base_index + output.available_capacity(), first_index + data.size());
  uint64_t len = end_index - start_index;
  

  if (first_index > base_index) {
    for (uint64_t i = 0; i < len; i++) {
      if (flag[i + first_index - base_index] == false) {
        flag[i + first_index - base_index] = true;
        buffer[i + first_index - base_index] = data[i];
        unassembled_size++;
      }
    }
  } else {
    // first_index < base_index, need to truncate
    if (first_index + data.size() > base_index) {
      for (uint64_t i = 0; i < len; i++) {
        if (flag[i] == false) {
          flag[i] = true;
          buffer[i] = data[i + base_index - first_index];
          unassembled_size++;
        }
      }
    }
  }

  string tmp = "";

  while (!flag.empty() && flag[0]) {
    tmp += buffer[0];
    flag.pop_front();
    flag.push_back(false);
    buffer.pop_front();
    buffer.push_back('\0');
  }

  if (tmp.length() > 0) {
    output.push(tmp);
    unassembled_size -= tmp.length();
    base_index += tmp.length();
  } 

  if (is_last_substring) {
    // is_last_substring maybe cycles ahead of finishing assembling
    eof = true;
  }

  if (unassembled_size == 0 && eof) {
    output.close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return unassembled_size;
}
