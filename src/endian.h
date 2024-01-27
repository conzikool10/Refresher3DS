/*
 * Copyright (c) 2011-2013 by naehrwert
 * Copyright (c) 2024 by Beyley Thomas
 * This file is released under the GPLv2.
 */

#pragma once

#include <stdint.h>

#define BOOL int
#define TRUE 1
#define FALSE 0

// Align.
#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))

// Bits <-> bytes conversion.
#define BITS2BYTES(x) ((x) / 8)
#define BYTES2BITS(x) ((x) * 8)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LITTLE_ENDIAN 1
#else
#undef LITTLE_ENDIAN
#endif

// Endian swap for u16.
#ifndef LITTLE_ENDIAN
#define _ES16(val)                                  \
    ((uint16_t)(((((uint16_t)val) & 0xff00) >> 8) | \
                ((((uint16_t)val) & 0x00ff) << 8)))
#else
#define _ES16(val) (val)
#endif

#ifndef LITTLE_ENDIAN
// Endian swap for u32.
#define _ES32(val)                                       \
    ((uint32_t)(((((uint32_t)val) & 0xff000000) >> 24) | \
                ((((uint32_t)val) & 0x00ff0000) >> 8) |  \
                ((((uint32_t)val) & 0x0000ff00) << 8) |  \
                ((((uint32_t)val) & 0x000000ff) << 24)))
#else
#define _ES32(val) (val)
#endif

#ifndef LITTLE_ENDIAN
// Endian swap for u64.
#define _ES64(val)                                                  \
    ((uint64_t)(((((uint64_t)val) & 0xff00000000000000ull) >> 56) | \
                ((((uint64_t)val) & 0x00ff000000000000ull) >> 40) | \
                ((((uint64_t)val) & 0x0000ff0000000000ull) >> 24) | \
                ((((uint64_t)val) & 0x000000ff00000000ull) >> 8) |  \
                ((((uint64_t)val) & 0x00000000ff000000ull) << 8) |  \
                ((((uint64_t)val) & 0x0000000000ff0000ull) << 24) | \
                ((((uint64_t)val) & 0x000000000000ff00ull) << 40) | \
                ((((uint64_t)val) & 0x00000000000000ffull) << 56)))
#else
#define _ES64(val) (val)
#endif

#ifdef __cplusplus
}
#endif