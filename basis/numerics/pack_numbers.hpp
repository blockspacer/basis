#pragma once

#include <base/logging.h>

#include <cstdint>

/// \note Take care of byte order (endianess).
/// Usually data pack'd on one architecture
/// will not be usable on one with the opposite endianness.
/// Use fixed-width integers (uint8_t, int16_t, etc.) and size_t.

namespace basis {

// pack mix of (up to 4) uint_8t into one uint32_t
//
// EXAMPLE
//
// uint32_t packed = pack_to_uint32(4u,5u,6u,7u);
// std::cout << (int)unpack_unsigned<0>(packed) << "\n";
// std::cout << (int)unpack_unsigned<1>(packed) << "\n";
// std::cout << (int)unpack_unsigned<3>(packed) << "\n";
// std::cout << (int)unpack_unsigned<3>(packed) << "\n";
//
inline uint32_t pack_to_uint32(
  uint_8t x, uint_8t y, uint_8t z, uint_8t w)
{
  return (static_cast<uint32_t>(w) << 24)
         | (static_cast<uint32_t>(z) << 16)
         | (static_cast<uint32_t>(y) << 8)
         | static_cast<uint32_t>(x);
}

template <int N>
uint8_t unpack_unsigned(uint32_t packed)
{
  // cast to avoid potential warnings for implicit narrowing conversion
  return static_cast<uint8_t>(packed >> (N*8));
}

// pack 4 normalized floats [-1, 1] to uint32_t
//
// EXAMPLE
//
// uint32_t packed = pack_nf_to_uint32_t(0.3f,0.2f,0.45f,0.99f);
// float x = (float(unpack_unsigned<0>(packed)) - 128.0f) * 1.0f/127.0f;
// float y = (float(unpack_unsigned<1>(packed)) - 128.0f) * 1.0f/127.0f;
//
inline uint32_t pack_nf_to_uint32_t(
  float x, float y = 0.0f, float z = 0.0f, float w = 0.0f)
{
  DCHECK(x >= -1.0f);
  DCHECK(x <= 1.0f);
  const uint_8t xx = uint_8t(x * 127.0f + 128.0f);

  DCHECK(y >= -1.0f);
  DCHECK(y <= 1.0f);
  const uint_8t yy = uint_8t(y * 127.0f + 128.0f);

  DCHECK(z >= -1.0f);
  DCHECK(z <= 1.0f);
  const uint_8t zz = uint_8t(z * 127.0f + 128.0f);

  DCHECK(w >= -1.0f);
  DCHECK(w <= 1.0f);
  const uint_8t ww = uint_8t(w * 127.0f + 128.0f);

  return pack_to_uint32(xx, yy, zz, ww);
}

// pack 4 unsigned floats [0, 1] to uint32_t
//
// EXAMPLE
//
// uint32_t packed = pack_uf_to_uint32_t(0.3f,0.2f,0.45f,0.99f);
// float x = float(unpack_unsigned<0>(packed)) * 1.0f/255.0f;
// float y = float(unpack_unsigned<1>(packed)) * 1.0f/255.0f;
//
inline uint32_t pack_uf_to_uint32_t(
  float x, float y = 0.0f, float z = 0.0f, float w = 0.0f)
{
  DCHECK(x >= 0.0f);
  DCHECK(x <= 1.0f);
  const uint_8t xx = uint_8t(x * 255.f);

  DCHECK(y >= 0.0f);
  DCHECK(y <= 1.0f);
  const uint_8t yy = uint_8t(y * 255.f);

  DCHECK(z >= 0.0f);
  DCHECK(z <= 1.0f);
  const uint_8t zz = uint_8t(z * 255.f);

  DCHECK(w >= 0.0f);
  DCHECK(w <= 1.0f);
  const uint_8t ww = uint_8t(w * 255.f);

  return pack_to_uint32(xx, yy, zz, ww);
}

} // namespace basis
