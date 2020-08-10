//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/bit_stream.h"

bool nj_bs_init(nj_bit_stream_t* bs, nju8* data) {
  bs->data = data;
  bs->index = 0;
  return true;
}

nju64 nj_bs_read_lsb(nj_bit_stream_t* bs, njsz num_of_bits) {
  nju64 result = 0;
  njsz temp_idx = bs->index;
  for (njsz i = 0; i < num_of_bits; ++i, temp_idx++) {
    njsz bytes_num = temp_idx / 8;
    njsz bit_in_byte = temp_idx % 8;
    result |= ((bs->data[bytes_num] >> bit_in_byte) & 1) << i;
  }
  return result;
}

nju64 nj_bs_read_msb(nj_bit_stream_t* bs, njsz num_of_bits) {
  nju64 result = 0;
  njsz temp_idx = bs->index;
  for (njsz i = 0; i < num_of_bits; ++i, temp_idx++) {
    njsz bytes_num = temp_idx / 8;
    njsz bit_in_byte = temp_idx % 8;
    result |= ((bs->data[bytes_num] >> bit_in_byte) & 1)
              << (num_of_bits - 1 - i);
  }
  return result;
}

void nj_bs_skip(nj_bit_stream_t* bs, njsz num_of_bits) {
  bs->index += num_of_bits;
}

nju64 nj_bs_consume_lsb(nj_bit_stream_t* bs, njsz num_of_bits) {
  nju64 result = nj_bs_read_lsb(bs, num_of_bits);
  nj_bs_skip(bs, num_of_bits);
  return result;
}

nju64 nj_bs_consume_msb(nj_bit_stream_t* bs, njsz num_of_bits) {
  nju64 result = nj_bs_read_msb(bs, num_of_bits);
  nj_bs_skip(bs, num_of_bits);
  return result;
}
