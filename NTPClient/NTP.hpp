#pragma once

/// read a big-endian 16-bit int from a byte buffer
constexpr uint16_t read_be16(byte* d)
{
  uint16_t result = uint16_t{ d[0] } << 8;
  result |= d[1];

  return result;
}

/// read a big-endian 32-bit int from a byte buffer
constexpr uint32_t read_be32(byte* d)
{
  uint32_t result = uint32_t{ read_be16(d) } << 16;
  d += 2;
  result |= uint32_t{ read_be16(d) };

  return result;
}
