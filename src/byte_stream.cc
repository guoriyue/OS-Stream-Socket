#include "byte_stream.hh"
#include <stdexcept>
#include <vector>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), stream() {}

void Writer::push( string data )
{
  // push to buffer and update bytes_write, be careful about the available_capacity
  uint64_t length_write = min( data.length(), capacity_ - stream.size() );
  for ( uint64_t i = 0; i < length_write; i++ ) {
    stream.push_back( data[i] );
  }
  bytes_write += length_write;
}

void Writer::close()
{
  // set close_ flag
  close_ = true;
}

void Writer::set_error()
{
  // set error_ flag
  error_ = true;
}

bool Writer::is_closed() const
{
  // get close_ flag
  return close_;
}

uint64_t Writer::available_capacity() const
{
  // the capacity left
  return capacity_ - stream.size();
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_write;
}

string_view Reader::peek() const
{
  // returning the entire buffer
  return string_view { stream.data(), stream.size() };
}

bool Reader::is_finished() const
{
  // Is the stream finished (closed and fully popped)?
  return close_ && ( bytes_write == bytes_read );
}

bool Reader::has_error() const
{
  return error_;
}

void Reader::pop( uint64_t len )
{
  // pop len bytes from the buffer, set error_ to true if required to pop more bytes than the buffer size
  if ( len > stream.size() ) {
    error_ = true;
    return;
  }
  stream.erase( stream.begin(), stream.begin() + len );
  bytes_read += len;
}

uint64_t Reader::bytes_buffered() const
{
  // Number of bytes currently buffered (pushed and not popped)
  return bytes_write - bytes_read;
}

uint64_t Reader::bytes_popped() const
{
  return bytes_read;
}
