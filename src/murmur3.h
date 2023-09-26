#ifndef _MURMUR3_H_
#define _MURMUR3_H_

/* MurmurHash3 was written by Austin Appleby, and is placed in the public
   domain. The author hereby disclaims copyright to this source code.
*/

static inline uint32_t murmur3_rotl32(uint32_t x, int_fast8_t r) {
  return (x << r) | (x >> (32 - r));
}

static inline uint32_t murmur3_mix_k1(uint32_t k1) {
  k1 *= 0xcc9e2d51;
  k1 = murmur3_rotl32(k1, 15);
  k1 *= 0x1b873593;
  return k1;
}

static inline uint32_t murmur3_mix_h1(uint32_t h1, uint32_t k1) {
  h1 ^= k1;
  h1 = murmur3_rotl32(h1, 13);
  h1 = h1 * 5 + 0xe6546b64;
  return h1;
}

static inline uint32_t murmur3_finalize(uint32_t h1, uint32_t length) {
  h1 ^= length;
  h1 ^= h1 >> 16;
  h1 *= 0x85ebca6b;
  h1 ^= h1 >> 13;
  h1 *= 0xc2b2ae35;
  h1 ^= h1 >> 16;
  return h1;
}

static inline uint32_t murmur3_hash_coll(uint32_t hash, uint32_t n) {
  uint32_t k1 = murmur3_mix_k1(hash);
  uint32_t h1 = murmur3_mix_h1(0, k1);
  return murmur3_finalize(h1, n);
}

static inline uint32_t murmur3_hash_int(uint32_t input) {
  if (input == 0) return 0;

  uint32_t k1 = murmur3_mix_k1(input);
  uint32_t h1 = murmur3_mix_h1(0, k1);

  return murmur3_finalize(h1, 4);
}

static inline uint32_t murmur3_hash_long(uint64_t input) {
  if (input == 0) return 0;
  
  uint32_t k1 = murmur3_mix_k1((uint32_t)input);
  uint32_t h1 = murmur3_mix_h1(0, k1);
  
  k1 = murmur3_mix_k1((uint32_t)(input >> 32));
  h1 = murmur3_mix_h1(h1, k1);
  
  return murmur3_finalize(h1, 8);
}

static inline uint32_t murmur3_hash_string(const char * str, size_t size) {
  int hashcode = 0, m = 1;
  for (int i = (int)size - 1; i >= 0; i--) {
    hashcode += str[i] * m;
    m *= 31;
  }
  return murmur3_hash_int(hashcode);
}

#endif
