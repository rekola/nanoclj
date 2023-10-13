#ifndef _NANOCLJ_HASH_H_
#define _NANOCLJ_HASH_H_

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

static inline uint32_t hash_combine(uint32_t seed, uint32_t hash) {
  // see https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
  seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  return seed;
}

static inline uint32_t get_string_hashcode(const char * str, size_t size) {
  int hashcode = 0, m = 1;
  for (int i = (int)size - 1; i >= 0; i--) {
    hashcode += str[i] * m;
    m *= 31;
  }
  return hashcode;
}

static inline uint32_t murmur3_hash_qualified_string(const char * ns, size_t ns_size, const char * name, size_t name_size) {
  return murmur3_hash_int(hash_combine(get_string_hashcode(name, name_size),
				       get_string_hashcode(ns, ns_size)));
}

#endif
