#include "wrapping_integers.hh"
#include "math.h"
#include <iostream>
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Convert absolute seqno → seqno
  // Use operator+ in Wrap32, implicitly convert to uint32_t
  return zero_point + n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Convert seqno → absolute seqno
  // Convert to uint64_t
  uint64_t seqno = uint64_t( this->raw_value_ );
  uint64_t isn = uint64_t( zero_point.raw_value_ );
  auto absolute_delta = []( uint64_t a, uint64_t b ) {
    if ( a < b ) {
      return b - a;
    } else {
      return a - b;
    }
  };

  uint64_t tmp = 0;
  // Can't use absolute_delta here!
  if ( seqno < isn ) {
    tmp = ( 1ll << 32 ) - ( isn - seqno );
  } else {
    tmp = seqno - isn;
  }

  uint64_t last_tmp = tmp;

  if ( tmp >= checkpoint ) {
    uint32_t mod = ( tmp - checkpoint ) % ( 1ll << 32 );
    tmp = checkpoint + mod;
    last_tmp = tmp - ( 1ll << 32 );
  } else {
    uint32_t mod = ( checkpoint - tmp ) % ( 1ll << 32 );
    tmp = checkpoint - mod;
    last_tmp = tmp + ( 1ll << 32 );
  }

  if ( absolute_delta( checkpoint, last_tmp ) < absolute_delta( checkpoint, tmp ) ) {
    return last_tmp;
  } else {
    return tmp;
  }
}
