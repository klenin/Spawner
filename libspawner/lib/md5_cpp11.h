/*
  Copyright (c) 2015, The SObjectizer Project
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  - Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  - Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  - The name of the author may not be used to endorse or promote
  products derived from this software without specific prior written
  permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier: BSL-1.0
//
//
// MD5 (RFC 1321) algorithm:
// Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
// rights reserved.
// 
// License to copy and use this software is granted provided that it
// is identified as the "RSA Data Security, Inc. MD5 Message-Digest
// Algorithm" in all material mentioning or referencing this software
// or this function.
//
// License is also granted to make and use derivative works provided
// that such works are identified as "derived from the RSA Data
// Security, Inc. MD5 Message-Digest Algorithm" in all material
// mentioning or referencing the derived work.
//
// RSA Data Security, Inc. makes no representations concerning either
// the merchantability of this software or the suitability of this
// software for any particular purpose. It is provided "as is"
// without express or implied warranty of any kind.
//
// These notices must be retained in any copies of any part of this
// documentation and/or software.
//

#pragma once

#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <cstring>

namespace md5_cpp11
{

const std::size_t digest_length = 16;

using digest = std::array< std::uint8_t, digest_length >;

namespace details
{

template< class T >
inline std::uint8_t
as_uint8( T what ) { return static_cast< std::uint8_t >( what ); }

template< class T >
inline std::uint32_t
as_uint32( T what ) { return static_cast< std::uint32_t >( what ); }

template< class T >
const std::uint8_t *
as_uint8_ptr( const T * what ) { return reinterpret_cast< const std::uint8_t * >( what ); }

inline void
encode(
  std::uint8_t * output,
  const std::uint32_t * input,
  std::size_t len )
  {
    for (std::size_t i = 0, j = 0; j < len; i++, j += 4) 
    {
      output[j]   = as_uint8(input[i] & 0xff);
      output[j+1] = as_uint8((input[i] >> 8) & 0xff);
      output[j+2] = as_uint8((input[i] >> 16) & 0xff);
      output[j+3] = as_uint8((input[i] >> 24) & 0xff);
    }
  }

inline void
decode(
  std::uint32_t * output,
  const std::uint8_t * input,
  std::size_t len)
  {
    for (std::size_t i = 0, j = 0; j < len; i++, j += 4)
      output[i] =
          as_uint32(input[j]) |
          (as_uint32(input[j+1]) << 8) |
          (as_uint32(input[j+2]) << 16) |
          (as_uint32(input[j+3]) << 24);
  }

/* Constants for MD5Transform routine. */
const std::uint32_t S11 = 7;
const std::uint32_t S12 = 12;
const std::uint32_t S13 = 17;
const std::uint32_t S14 = 22;
const std::uint32_t S21 = 5;
const std::uint32_t S22 = 9;
const std::uint32_t S23 = 14;
const std::uint32_t S24 = 20;
const std::uint32_t S31 = 4;
const std::uint32_t S32 = 11;
const std::uint32_t S33 = 16;
const std::uint32_t S34 = 23;
const std::uint32_t S41 = 6;
const std::uint32_t S42 = 10;
const std::uint32_t S43 = 15;
const std::uint32_t S44 = 21;

/* F, G, H and I are basic MD5 functions. */
inline std::uint32_t
F( const std::uint32_t x, const std::uint32_t y, const std::uint32_t z )
{ return (x & y) | ((~x) & z); }

inline std::uint32_t
G( const std::uint32_t x, const std::uint32_t y, const std::uint32_t z )
{ return (x & z) | (y & (~z)); }

inline std::uint32_t
H( const std::uint32_t x, const std::uint32_t y, const std::uint32_t z )
{ return (x ^ y ^ z); }

inline std::uint32_t
I( const std::uint32_t x, const std::uint32_t y, const std::uint32_t z )
{ return y ^ (x | (~z)); }


/* ROTATE_LEFT rotates x left n bits. */
inline std::uint32_t
rotate_left( const std::uint32_t x, unsigned int n )
{ return (x << n) | (x >> (32-n)); }

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
   Rotation is separate from addition to prevent recomputation. */
inline void
FF( std::uint32_t & a,
  const std::uint32_t b, const std::uint32_t c, const std::uint32_t d,
  const std::uint32_t x, const std::uint32_t s, const std::uint32_t ac )
  {
    a += F(b, c, d) + x + ac;
    a = rotate_left(a, s);
    a += b;
  }

inline void
GG( std::uint32_t & a,
  const std::uint32_t b, const std::uint32_t c, const std::uint32_t d,
  const std::uint32_t x, const std::uint32_t s, const std::uint32_t ac )
  {
    a += G(b, c, d) + x + ac;
    a = rotate_left(a, s);
    a += b;
  }

inline void
HH( std::uint32_t & a,
  const std::uint32_t b, const std::uint32_t c, const std::uint32_t d,
  const std::uint32_t x, const std::uint32_t s, const std::uint32_t ac )
  {
    a += H(b, c, d) + x + ac;
    a = rotate_left(a, s);
    a += b;
  }

inline void
II( std::uint32_t & a,
  const std::uint32_t b, const std::uint32_t c, const std::uint32_t d,
  const std::uint32_t x, const std::uint32_t s, const std::uint32_t ac )
  {
    a += I(b, c, d) + x + ac;
    a = rotate_left(a, s);
    a += b;
  }

inline void
transform(
  std::uint32_t state[4],
  const std::uint8_t block[64] )
  {
    std::uint32_t a = state[0];
    std::uint32_t b = state[1];
    std::uint32_t c = state[2];
    std::uint32_t d = state[3];
    std::uint32_t x[16];

    decode(x, block, 64);

    /* Round 1 */
    FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
    FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
    FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
    FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
    FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
    FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
    FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
    FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
    FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
    FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
    FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
    FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
    FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
    FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
    FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
    FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

    /* Round 2 */
    GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
    GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
    GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
    GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
    GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
    GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
    GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
    GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
    GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
    GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
    GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
    GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
    GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
    GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
    GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
    GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
    HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
    HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
    HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
    HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
    HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
    HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
    HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
    HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
    HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
    HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
    HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
    HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
    HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
    HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
    HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
    HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

    /* Round 4 */
    II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
    II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
    II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
    II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
    II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
    II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
    II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
    II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
    II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
    II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
    II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
    II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
    II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
    II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
    II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
    II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    /* Zeroize sensitive information. */
    std::fill( std::begin(x), std::end(x), 0);
  }

inline std::uint8_t
char_to_byte( std::string::value_type v )
  {
    if (v >= '0' && v <= '9')
      return as_uint8(v - '0');
    else if (v >= 'a' && v <= 'f')
      return as_uint8(v - 'a' + 10);
    else if (v >= 'A' && v <= 'F')
      return as_uint8(v - 'A' + 10);
    else
      throw std::invalid_argument( "non-hex symbol found in MD5 value" );
  };

} /* namespace details */

class builder
  {
  public :
    inline builder()
      {
        reset();
      }
    inline ~builder()
      {
        /* Zeroize sensitive information. */
        reset();
      }

    inline builder &
    update( const std::uint8_t * what, std::size_t length )
      {
        using namespace details;

        /* Compute number of bytes mod 64 */
        std::size_t index = (count_[0] >> 3) & 0x3F;

        /* Update number of bits */
        const auto bits = as_uint32(length << 3);
        count_[0] += bits;
        if( count_[0] < bits )
          count_[1]++;
        count_[1] += as_uint32(length >> 29);

        const std::size_t part_len = 64 - index;

        /* Transform as many times as possible. */
        std::size_t i = 0;
        if( length >= part_len ) 
        {
          std::copy( what, what + part_len, buffer_ + index );
          transform( state_, buffer_ );

          for (i = part_len; i + 63 < length; i += 64)
            transform( state_, what + i );

          index = 0;
        }

        /* Buffer remaining input */
        std::copy( what + i, what + length, buffer_ + index );

        return *this;
      }

    digest
    finalize()
      {
        using namespace details;

        static const std::uint8_t PADDING[64] = 
        {
          0x80, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0
        };
        std::uint8_t bits[8];

        /* Save number of bits */
        encode( bits, count_, 8 );

        /* Pad out to 56 mod 64. */
        const std::size_t index = (count_[0] >> 3) & 0x3f;
        const std::size_t padLen = (index < 56) ? (56 - index) : (120 - index);
        update( PADDING, padLen );

        /* Append length (before padding) */
        update( bits, 8 );

        /* Store state in digest */
        digest result;
        result.fill( 0 );
        encode( &result.front(), state_, 16 );

        /* Zeroize sensitive information. */
        reset();

        return result;
      }

  private :
    std::uint32_t state_[4];   // state (ABCD)
    std::uint32_t count_[2];   // number of bits, modulo 2^64 (lsb first)
    std::uint8_t buffer_[64];  // input buffer

    void
    reset()
      {
        count_[0] = count_[1] = 0;
        state_[0] = 0x67452301;
        state_[1] = 0xefcdab89;
        state_[2] = 0x98badcfe;
        state_[3] = 0x10325476;

        std::fill( std::begin(buffer_), std::end(buffer_), 0 );
      }
  };

inline std::string
to_hex_string( const digest & what )
  {
    static const char digits[] = "0123456789abcdef";

    std::string result;
    result.reserve( digest_length * 2);

    for( const auto c : what )
    {
      result += digits[(c >> 4) & 0xF];
      result += digits[c & 0xF];
    }

    return result;
  }

inline digest
from_hex_string( const std::string & what )
  {
    using namespace details;

    if( what.size() != digest_length * 2 )
      throw std::invalid_argument( "hex representation of MD5 value "
          "must be exactly 32 bytes long" );

    digest result;
    for( std::size_t i = 0, j = 0; i < digest_length; i += 1, j += 2 )
      result[ i ] = (char_to_byte(what[j]) << 4) | (char_to_byte(what[j+1]));

    return result;
  }

inline digest
make_digest( const std::uint8_t * what, std::size_t length )
  {
    return builder{}.update( what, length ).finalize();
  }

template< class T >
inline digest
make_digest( const T * begin, const T * end )
  {
    const std::uint8_t * const start = details::as_uint8_ptr( begin );
    const std::uint8_t * const finish = details::as_uint8_ptr( end );
    const auto length = static_cast< std::size_t >( finish - start );

    return make_digest( start, length );
  }

inline digest
make_digest( const char * what, std::size_t length )
  {
    return builder{}.update(
        details::as_uint8_ptr( what ), length ).finalize();
  }

inline digest
make_digest( const char * what )
  {
    return builder{}.update(
        details::as_uint8_ptr( what ), std::strlen( what ) ).finalize();
  }

template< class T, typename... P >
inline digest
make_digest( const std::basic_string< T, P... > & what )
  {
    return make_digest( what.data(), what.data() + what.size() );
  }

template< class T, typename... P >
inline digest
make_digest( const std::vector< T, P... > & what )
  {
    return make_digest( what.data(), what.data() + what.size() );
  }

template< class T, std::size_t N >
inline digest
make_digest( const std::array< T, N > & what )
  {
    return make_digest( what.data(), what.data() + what.size() );
  }

} /* namespace md5_cpp11 */

