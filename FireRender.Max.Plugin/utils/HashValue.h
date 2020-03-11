/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/


/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* CRC32 based hash
*
* Parts of this code based on the work of Stephan Brumme (CRC32)
* Slicing-by-16 contributed by Bulat Ziganshin
* Tableless bytewise CRC contributed by Hagai Gold
* see http://create.stephan-brumme.com/disclaimer.html
*********************************************************************************************************************************/

#pragma once

#include <stdint.h>
#include <stddef.h>

#define CRC32_USE_LOOKUP_TABLE_BYTE
#define CRC32_USE_LOOKUP_TABLE_SLICING_BY_8
#define CRC32_USE_LOOKUP_TABLE_SLICING_BY_4
#define CRC32_USE_LOOKUP_TABLE_SLICING_BY_16

/// Slicing-By-16
#ifdef CRC32_USE_LOOKUP_TABLE_SLICING_BY_16
#define MaxSlice 16
#elif defined(CRC32_USE_LOOKUP_TABLE_SLICING_BY_8)
#define MaxSlice 8
#elif defined(CRC32_USE_LOOKUP_TABLE_SLICING_BY_4)
#define MaxSlice 4
#elif defined(CRC32_USE_LOOKUP_TABLE_BYTE)
#define MaxSlice 1
#else
#define NO_LUT // don't use Crc32Lookup at all
#endif

// define endianess and some integer data types
#if defined(_MSC_VER) || defined(__MINGW32__)
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER    __LITTLE_ENDIAN

#include <xmmintrin.h>
#ifdef __MINGW32__
#define PREFETCH(location) __builtin_prefetch(location)
#else
#define PREFETCH(location) _mm_prefetch(location, _MM_HINT_T0)
#endif
#else
// defines __BYTE_ORDER as __LITTLE_ENDIAN or __BIG_ENDIAN
#include <sys/param.h>

#ifdef __GNUC__
#define PREFETCH(location) __builtin_prefetch(location)
#else
#define PREFETCH(location) ;
#endif
#endif


class HashValue
{
private:
	// crc32_fast selects the fastest algorithm depending on flags (CRC32_USE_LOOKUP_...)
	/// compute CRC32 using the fastest algorithm for large datasets on modern CPUs
	uint32_t crc32_fast(const void* data, size_t length, uint32_t previousCrc32 = 0);

	/// compute CRC32 (bitwise algorithm)
	uint32_t crc32_bitwise(const void* data, size_t length, uint32_t previousCrc32 = 0);
	/// compute CRC32 (half-byte algoritm)
	uint32_t crc32_halfbyte(const void* data, size_t length, uint32_t previousCrc32 = 0);

#ifdef CRC32_USE_LOOKUP_TABLE_BYTE
	/// compute CRC32 (standard algorithm)
	uint32_t crc32_1byte(const void* data, size_t length, uint32_t previousCrc32 = 0);
#endif

	/// compute CRC32 (byte algorithm) without lookup tables
	uint32_t crc32_1byte_tableless(const void* data, size_t length, uint32_t previousCrc32 = 0);
	/// compute CRC32 (byte algorithm) without lookup tables
	uint32_t crc32_1byte_tableless2(const void* data, size_t length, uint32_t previousCrc32 = 0);

#ifdef CRC32_USE_LOOKUP_TABLE_SLICING_BY_4
	/// compute CRC32 (Slicing-by-4 algorithm)
	uint32_t crc32_4bytes(const void* data, size_t length, uint32_t previousCrc32 = 0);
#endif

#ifdef CRC32_USE_LOOKUP_TABLE_SLICING_BY_8
	/// compute CRC32 (Slicing-by-8 algorithm)
	uint32_t crc32_8bytes(const void* data, size_t length, uint32_t previousCrc32 = 0);
	/// compute CRC32 (Slicing-by-8 algorithm), unroll inner loop 4 times
	uint32_t crc32_4x8bytes(const void* data, size_t length, uint32_t previousCrc32 = 0);
#endif

#ifdef CRC32_USE_LOOKUP_TABLE_SLICING_BY_16
	/// compute CRC32 (Slicing-by-16 algorithm)
	uint32_t crc32_16bytes(const void* data, size_t length, uint32_t previousCrc32 = 0);
	/// compute CRC32 (Slicing-by-16 algorithm, prefetch upcoming data blocks)
	uint32_t crc32_16bytes_prefetch(const void* data, size_t length, uint32_t previousCrc32 = 0, size_t prefetchAhead = 256);
#endif

	/// swap endianess
	inline uint32_t swap(uint32_t x)
	{
#if defined(__GNUC__) || defined(__clang__)
		return __builtin_bswap32(x);
#else
		return (x >> 24) |
			((x >> 8) & 0x0000FF00) |
			((x << 8) & 0x00FF0000) |
			(x << 24);
#endif
	}

private:
	const uint32_t Polynomial = 0xEDB88320;

#ifndef NO_LUT
	/// look-up table, already declared above
	static const uint32_t Crc32Lookup[MaxSlice][256];
#endif

	uint32_t crc = 0;

public:
	HashValue(uint32_t v = 0)
	: crc(v)
	{
	}
	
	HashValue(const HashValue &other)
	: crc(other.crc)
	{
	}

	template <class T>
	HashValue& operator<<(const T& v)
	{
		crc = crc32_fast(&v, sizeof(v), crc);
		return *this;
	}

	inline HashValue& operator = (const HashValue& other)
	{
		crc = other.crc;
		return *this;
	}

	inline operator uint32_t() const
	{
		return crc;
	}
};
