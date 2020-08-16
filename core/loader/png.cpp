//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/loader/png.h"

#include "core/allocator.h"
#include "core/bit_stream.h"
#include "core/dynamic_array.h"
#include "core/file_utils.h"
#include "core/linear_allocator.h"
#include "core/log.h"
#include "core/os.h"

#include <stdlib.h>

#if NJ_OS_WIN()
#  define bswap32(x) _byteswap_ulong(x)
#elif NJ_OS_LINUX()
#  include <byteswap.h>
#  define bswap32(x) bswap_32(x)
#endif

#define FOURCC(cc) (cc[0] | cc[1] << 8 | cc[2] << 16 | cc[3] << 24)

// From 0 - 15
static const int gc_max_code_len = 16;
static const int gc_max_code = 286 + 30;
static const int gc_png_sig_len = 8;

static const nju8 gc_png_signature[gc_png_sig_len] = {137, 80, 78, 71, 13, 10, 26, 10};

static const int gc_len_bases[] = {3,  4,  5,  6,   7,   8,   9,   10,  11, 13,
                                   15, 17, 19, 23,  27,  31,  35,  43,  51, 59,
                                   67, 83, 99, 115, 131, 163, 195, 227, 258};

static const int gc_len_extra_bits[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                                        1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
                                        4, 4, 4, 4, 5, 5, 5, 5, 0};

static const int gc_dist_bases[] = {
    1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
    33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
    1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

static const int gc_dist_extra_bits[] = {0, 0, 0,  0,  1,  1,  2,  2,  3,  3,
                                         4, 4, 5,  5,  6,  6,  7,  7,  8,  8,
                                         9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

// Structure saves codes of specific length.
// |codes| contains codes of that length.
// |min| is the smallest codes of that length.
// |count| is the number of codes.
struct codes_for_len_t {
  nju16 codes[gc_max_code];
  nju16 min;
  nju16 count;
};

// |cfl| is a pointer to array of where the index is the length and value is the
// codes for that length.
struct alphabet_t {
  codes_for_len_t* cfl;
  nju8 min_len;
  nju8 max_len;
};

// Build an alphabet based on lengths of  codes from 0 to |num| - 1.
static void build_alphabet(nju8* lens, int num, alphabet_t* alphabet) {
  nju8 len_counts[gc_max_code_len] = {};
  for (int i = 0; i < num; ++i) {
    if (lens[i]) {
      alphabet->cfl[lens[i]].codes[len_counts[lens[i]]] = i;
      len_counts[lens[i]]++;
    }
  }
  for (int i = 0; i < gc_max_code_len; ++i) {
    if (len_counts[i]) {
      alphabet->min_len = i;
      break;
    }
  }
  for (nju8 i = gc_max_code_len - 1; i > 0; ++i) {
    if (len_counts[i]) {
      alphabet->max_len = i;
      break;
    }
  }
  int smallest_code = 0;
  int last_non_zero_count = 0;
  for (nju8 i = 0; i < 16; ++i) {
    if (len_counts[i]) {
      smallest_code = (smallest_code + last_non_zero_count) << 1;
      alphabet->cfl[i].min = smallest_code;
      alphabet->cfl[i].count = len_counts[i];
      last_non_zero_count = len_counts[i];
    }
  }
}

static int decode(nj_bit_stream_t* bs, const alphabet_t* c_alphabet) {
  int code = nj_bs_consume_msb(bs, c_alphabet->min_len);
  for (nju8 i = c_alphabet->min_len; i <= c_alphabet->max_len; ++i) {
    int delta_to_min = code - c_alphabet->cfl[i].min;
    if (delta_to_min < c_alphabet->cfl[i].count) {
      return c_alphabet->cfl[i].codes[delta_to_min];
    }
    code = code << 1 | nj_bs_consume_msb(bs, 1);
  }
  NJ_LOGF("Can't decode");
  return -1;
}

static void decode_len_and_dist(int len_code, nj_bit_stream_t* bs, const alphabet_t* c_dist_alphabet, nju8** o_deflated) {
  int len_idx = len_code % 257;
  int len_base = gc_len_bases[len_idx];
  int len_extra_bits = gc_len_extra_bits[len_idx];
  int len = len_base + nj_bs_consume_lsb(bs, len_extra_bits);
  int dist_idx = c_dist_alphabet ? decode(bs, c_dist_alphabet) : nj_bs_consume_lsb(bs, 5);
  int dist_base = gc_dist_bases[dist_idx];
  int dist_extra_bits = gc_dist_extra_bits[dist_idx];
  int dist = dist_base + nj_bs_consume_lsb(bs, dist_extra_bits);
  nju8* copy = *o_deflated - dist;
  for (int i = 0; i < len; ++i)
    *(*o_deflated)++ = *copy++;
}

static int paeth(int a, int b, int c) {
  int p = a + b - c;
  int pa = abs(p - a);
  int pb = abs(p - b);
  int pc = abs(p - c);
  if (pa <= pb && pa <= pc)
    return a;
  else if (pb <= pc)
    return b;
  return c;
}

bool nj_png_init(nj_png_t* png, const nj_os_char* path, nj_allocator_t* allocator) {
  png->allocator = allocator;

  nj_scoped_la_allocator_t<> temp_allocator("png_temp_allocator");
  temp_allocator.init();
  nj_dynamic_array_t<nju8> data = nj_read_whole_file(&temp_allocator, path, NULL);
  NJ_CHECKF_RETURN_VAL(!memcmp(data.p, &gc_png_signature[0], gc_png_sig_len), false, "Invalid PNG signature");
  for (int i = gc_png_sig_len; i < nj_da_len(&data);) {
    int data_len = bswap32(*((int*)(data.p + i)));
    i += 4;
    const nju8* chunk_it = data.p + i;
    const int chunk_type = *((int*)(data.p + i));
    i += 4;
    nju8* p = data.p + i;
    i += data_len;
    // nju8* cRC = it;
    i += 4;
    switch (chunk_type) {
    case FOURCC("IHDR"): {
      NJ_CHECKF_RETURN_VAL(data_len == 13, false, "Invalid IHDR length");
      png->width = bswap32(*(int*)p);
      p += 4;
      png->height = bswap32(*(int*)p);
      p += 4;
      png->bit_depth = *p++;
      nju8 color_type = *p++;
      switch (color_type) {
      case 0:
        png->bit_per_pixel = 1;
        break;
      case 2:
      case 3:
        png->bit_per_pixel = 3;
        break;
      case 4:
        png->bit_per_pixel = 2;
      case 6:
        png->bit_per_pixel = 2;
        break;
      default:
        NJ_LOGF_RETURN_VAL(false, "Invalid color type");
      }
      nju8 compression_method = *p++;
      NJ_CHECKF_RETURN_VAL(!compression_method, false, "Invalid compression method");
      nju8 filter_method = *p++;
      NJ_CHECKF_RETURN_VAL(!filter_method, false, "Invalid filter method");
      const nju8 interlace_method = *p++;
      NJ_CHECKF_RETURN_VAL(!interlace_method, false, "Invalid interlace method");
      break;
    }
    case FOURCC("PLTE"):
      break;
    case FOURCC("IDAT"): {
      nj_bit_stream_t bs;
      nj_bs_init(&bs, p);
      // 2 bytes of zlib header.
      nju32 zlib_compress_method = nj_bs_consume_lsb(&bs, 4);
      NJ_CHECKF_RETURN_VAL(zlib_compress_method == 8, false, "Invalid zlib compression method");
      nju32 zlib_compress_info = nj_bs_consume_lsb(&bs, 4);
      NJ_CHECKF_RETURN_VAL((p[0] * 256 + p[1]) % 31 == 0, false, "Invalid FCHECK bits");
      nj_bs_skip(&bs, 5);
      nju8 fdict = nj_bs_consume_lsb(&bs, 1);
      nju8 flevel = nj_bs_consume_lsb(&bs, 2);

      // 3 header bits
      const nju8 bfinal = nj_bs_consume_lsb(&bs, 1);
      const nju8 ctype = nj_bs_consume_lsb(&bs, 2);
      nju8* deflated_data = (nju8*)temp_allocator.alloc((png->width + 1) * png->height * png->bit_depth);
      nju8* deflated_p = deflated_data;
      if (ctype == 1) {
        // Fixed Huffman.
        for (;;) {
          int code;
          code = nj_bs_consume_msb(&bs, 7);
          if (code >= 0 && code <= 23) {
            code += 256;
            if (code == 256)
              break;
          } else {
            code = code << 1 | nj_bs_consume_msb(&bs, 1);
            if (code >= 48 && code <= 191) {
              *deflated_p++ = code - 48;
              continue;
            } else if (code >= 192 && code <= 199) {
              code += 88;
            } else {
              code = code << 1 | nj_bs_consume_msb(&bs, 1);
              NJ_CHECKF_RETURN_VAL(code >= 400 && code <= 511, false, "Can't decode fixed Huffman");
              *deflated_p++ = code - 256;
              continue;
            }
          }
          decode_len_and_dist(code, &bs, NULL, &deflated_p);
        }
      } else if (ctype == 2) {
        // Dynamic Huffman.
        int hlit = nj_bs_consume_lsb(&bs, 5) + 257;
        int hdist = nj_bs_consume_lsb(&bs, 5) + 1;
        int hclen = nj_bs_consume_lsb(&bs, 4) + 4;
        nju8 len_of_len[19] = {};
        const int c_len_alphabet[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
        for (int j = 0; j < hclen; ++j) {
          len_of_len[c_len_alphabet[j]] = nj_bs_consume_lsb(&bs, 3);
        }
        codes_for_len_t code_lens_cfl[8];
        alphabet_t code_len_alphabet = {code_lens_cfl, 0, 0};
        build_alphabet(len_of_len, 19, &code_len_alphabet);
        int index = 0;
        nju8 lit_and_dist_lens[gc_max_code];
        while (index < hlit + hdist) {
          int code_len = decode(&bs, &code_len_alphabet);
          if (code_len < 16) {
            lit_and_dist_lens[index++] = code_len;
          } else {
            nju8 repeat_num;
            nju8 repeated_val = 0;
            if (code_len == 16) {
              repeat_num = nj_bs_consume_lsb(&bs, 2) + 3;
              repeated_val = lit_and_dist_lens[index - 1];
            } else if (code_len == 17) {
              repeat_num = nj_bs_consume_lsb(&bs, 3) + 3;
            } else if (code_len == 18) {
              repeat_num = nj_bs_consume_lsb(&bs, 7) + 11;
            }
            memset(lit_and_dist_lens + index, repeated_val, repeat_num);
            index += repeat_num;
          }
          NJ_CHECKF_RETURN_VAL(index <= hlit + hdist, false, "Can't decode literal and length alphabet, overflowed");
        }
        NJ_CHECKF_RETURN_VAL(lit_and_dist_lens[256], false, "Symbol 256 can't have length of 0");
        codes_for_len_t lit_or_len_cfl[gc_max_code_len];
        alphabet_t lit_or_len_alphabet = {lit_or_len_cfl, 0, 0};
        build_alphabet(lit_and_dist_lens, hlit, &lit_or_len_alphabet);
        codes_for_len_t dist_cfl[gc_max_code_len];
        alphabet_t dist_alphabet = {dist_cfl, 0, 0};
        build_alphabet(lit_and_dist_lens + hlit, hdist, &dist_alphabet);
        for (;;) {
          int lit_or_len_code = decode(&bs, &lit_or_len_alphabet);
          if (lit_or_len_code == 256)
            break;
          if (lit_or_len_code < 256) {
            *deflated_p++ = lit_or_len_code;
            continue;
          }
          decode_len_and_dist(lit_or_len_code, &bs, &dist_alphabet, &deflated_p);
        }
      }
      png->data = (nju8*)png->allocator->alloc(png->width * png->height * 4);
      int bytes_per_deflated_row = 4 * png->width + 1;
      int bytes_per_data_row = 4 * png->width;
      for (int r = 0; r < png->height; ++r) {
        const nju8 filter_method = deflated_data[r * bytes_per_deflated_row];
        const int data_offset = r * bytes_per_data_row;
        const int deflated_offset = r * bytes_per_deflated_row + 1;
        nju8* a = &png->data[r * bytes_per_data_row];
        nju8* b = NULL;
        if (r)
          b = &png->data[(r - 1) * bytes_per_data_row];
        nju8* c = NULL;
        if (r)
          c = &png->data[(r - 1) * bytes_per_data_row];
        switch (filter_method) {
        case 0: {
          memcpy(png->data + data_offset, deflated_data + deflated_offset, bytes_per_data_row);
        } break;
        case 1: {
          for (int j = 0; j < 4; ++j)
            png->data[data_offset + j] = deflated_data[deflated_offset + j];
          for (int j = 4; j < bytes_per_data_row; ++j)
            png->data[data_offset + j] =
                deflated_data[deflated_offset + j] + *a++;
        } break;
        case 2: {
          if (!r) {
            for (int j = 0; j < bytes_per_data_row; ++j)
              png->data[data_offset + j] = deflated_data[deflated_offset + j];
          } else {
            for (int j = 0; j < bytes_per_data_row; ++j)
              png->data[data_offset + j] = deflated_data[deflated_offset + j] + *b++;
          }
        } break;
        case 3: {
          if (!r) {
            for (int j = 0; j < 4; ++j)
              png->data[data_offset + j] = deflated_data[deflated_offset + j];
            for (int j = 4; j < bytes_per_data_row; ++j)
              png->data[data_offset + j] = deflated_data[deflated_offset + j] + *a++ / 2;
          } else {
            for (int j = 0; j < 4; ++j)
              png->data[data_offset + j] = deflated_data[deflated_offset + j] + *b++ / 2;
            for (int j = 4; j < bytes_per_data_row; ++j)
              png->data[data_offset + j] = deflated_data[deflated_offset + j] + ((int)(*a++) + (int)(*b++)) / 2;
          }
        } break;
        case 4: {
          if (!r) {
            for (int j = 0; j < bytes_per_data_row; ++j)
              png->data[data_offset + j] = deflated_data[deflated_offset + j];
          } else {
            for (int j = 0; j < 4; ++j)
              png->data[data_offset + j] = deflated_data[deflated_offset + j] + *b++;
            for (int j = 4; j < bytes_per_data_row; ++j)
              png->data[data_offset + j] = deflated_data[deflated_offset + j] + paeth(*(a++), *(b++), *(c++));
          }
        } break;
        default:
          NJ_LOGF_RETURN_VAL(false, "Invalid filter method");
        }
      }
    } break;
    case FOURCC("IEND"):
      break;
    }
  }
  return true;
}

void nj_png_destroy(nj_png_t* png) {
  png->allocator->free(png->data);
}
