//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_BIT_STREAM_H
#define NJ_CORE_BIT_STREAM_H

#include "core/njtype.h"

// Provide methods to read bits value from a bytes array.
struct nj_bit_stream_t {
  nju8* data;
  njsp index;
};

bool nj_bs_init(nj_bit_stream_t* bs, nju8* data);

// Get number from |num_of_bits| without moving the bit pointer.
nju64 nj_bs_read_lsb(nj_bit_stream_t* bs, njsp num_of_bits);
nju64 nj_bs_read_msb(nj_bit_stream_t* bs, njsp num_of_bits);

void nj_bs_skip(nj_bit_stream_t* bs, njsp num_of_bits);

// Same as |Read*| but also skip |num_of_bits|.
nju64 nj_bs_consume_lsb(nj_bit_stream_t* bs, njsp num_of_bits);
nju64 nj_bs_consume_msb(nj_bit_stream_t* bs, njsp num_of_bits);

#endif // NJ_CORE_BIT_STREAM_H
