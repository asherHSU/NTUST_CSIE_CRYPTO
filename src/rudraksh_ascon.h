#ifndef RUDRAKSH_ASCON_H
#define RUDRAKSH_ASCON_H

#include <stdint.h>
#include <stddef.h>

#define RUDRAKSH_XOF_IN_BYTES 32
#define RUDRAKSH_HASH_H_OUT_BYTES 32
#define RUDRAKSH_HASH_G_OUT_BYTES 64
#define RUDRAKSH_PRF_IN_BYTES 17


// xof 生成 martix A : 32 bytes -> Any bytes
void rudraksh_xof(uint8_t *output, size_t outlen,const uint8_t *input);

// hash H() : Any bytes -> 32 bytes
void rudraksh_hash_H(uint8_t *output, const uint8_t *input, size_t inlen);

// hash G() : Any bytes -> 64 bytes
void rudraksh_hash_G(uint8_t *output, const uint8_t *input, size_t inlen);

// PRF(xof) : 17 bytes (key[16] + nonce[1]) -> Any bytes (32 / 64)
void rudraksh_prf(uint8_t *output, size_t outlen, const uint8_t *key, const uint8_t *nonce);

#endif