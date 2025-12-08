#ifndef RUDRAKSH_RANDOM_H
#define RUDRAKSH_RANDOM_H

#include <stdint.h>
#include <stddef.h>
#include "ascon/ascon.h"
#define RUDRAFKSH_STATE ascon_state_t

#define RUDRAKSH_PRF_IN_BYTES 18   // 16 bytes seed + 2 byte nonce

// ==========================================================
// 1. Ascon 狀態結構
// ==========================================================

// 不確定輸出長度，所以需要state參數
void rudraksh_prf_init(RUDRAFKSH_STATE *s, uint8_t *key, uint8_t *nonce_i, uint8_t *nonce_j);
void rudraksh_prf_put(RUDRAFKSH_STATE *s,uint8_t *out );


void rudraksh_hash(uint8_t *output, const uint8_t *input, size_t inlen);


// ==========================================================
// 2. random bytes 產生器
// ==========================================================
void rudraksh_randombytes(uint8_t *x, size_t xlen);

#endif