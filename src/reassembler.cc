#include "reassembler.hh"
#include <iostream>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  if ( buffer.empty() && flag.empty() ) {
    // initialization
    flag.insert(flag.end(), output.available_capacity(), false);
    buffer.insert(buffer.end(), output.available_capacity(), '\0');
  }

  uint64_t start_index = max( base_index, first_index );
  uint64_t end_index = min( base_index + output.available_capacity(), first_index + data.size() );
  uint64_t len = end_index - start_index;

  if ( first_index > base_index ) {
    for ( uint64_t i = 0; i < len; i++ ) {
      if ( flag[i + first_index - base_index] == false ) {
        flag[i + first_index - base_index] = true;
        buffer[i + first_index - base_index] = data[i];
        unassembled_size++;
      }
    }
  } else {
    // first_index < base_index, need to truncate
    if ( first_index + data.size() > base_index ) {
      for ( uint64_t i = 0; i < len; i++ ) {
        if ( flag[i] == false ) {
          flag[i] = true;
          buffer[i] = data[i + base_index - first_index];
          unassembled_size++;
        }
      }
    }
  }

  string tmp = "";
  long unsigned int cnt = 0;
  while ( cnt < flag.size() && flag[cnt] ) {
    tmp += buffer[cnt];
    cnt ++;
  }

  if (cnt) {
    flag.erase(flag.begin(), flag.begin() + cnt);
    flag.insert(flag.end(), cnt, false);
    buffer.erase(buffer.begin(), buffer.begin() + cnt);
    buffer.insert(buffer.end(), cnt, '\0');
  }

  if ( tmp.length() > 0 ) {
    output.push( tmp );
    unassembled_size -= tmp.length();
    base_index += tmp.length();
  }

  if ( is_last_substring ) {
    // is_last_substring maybe cycles ahead of finishing assembling
    eof = true;
  }

  if ( unassembled_size == 0 && eof ) {
    output.close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return unassembled_size;
}
