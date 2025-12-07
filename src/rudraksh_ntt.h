#ifndef RUDRAKSH_NTT_H
#define RUDRAKSH_NTT_H

#include <stdint.h>
#include "rudraksh_params.h"

// 1. NTT 專用常數
#define RUDRAKSH_ZETA 202

// 2. 資料結構
typedef struct {
    int16_t coeffs[KYBER_N];
} poly;

// 多項式向量 (矩陣運算用)
typedef struct {
    poly vec[KYBER_K];
} polyvec;

// 3. 全域變數宣告
extern const int16_t zetas[KYBER_N];

// ==========================================================
// 4. 數學核心函式 (Member A)
// ==========================================================

// NTT 轉換
void poly_ntt(poly *p);
void poly_invntt_tomont(poly *p);

// 基礎運算
int16_t fqmul(int16_t a, int16_t b);
int16_t fqadd(int16_t a, int16_t b);
int16_t fqsub(int16_t a, int16_t b);
int16_t fqinv(int16_t a);

// ==========================================================
// 5. 資料轉換與壓縮 (Member A)
// ==========================================================

// 訊息 <-> 多項式 (B=2)
void poly_frommsg(poly *r, const uint8_t msg[16]);
void poly_tomsg(uint8_t msg[16], const poly *a);

// 向量 U 壓縮 (10 bits)
void poly_compress_u(uint8_t *r, const poly *a);
void poly_decompress_u(poly *r, const uint8_t *a);

// 密文 V 壓縮 (3 bits)
void poly_compress_v(uint8_t *r, const poly *a);
void poly_decompress_v(poly *r, const uint8_t *a);

// ==========================================================
// 6. 取樣函式 (Member B)
// ==========================================================
void poly_uniform(poly *p, const uint8_t *seed, uint16_t nonce);
void poly_cbd_eta2(poly *p, const uint8_t *buf);

// 13-bit Serialization (For PK/SK)
void poly_tobytes_13bit(uint8_t *r, const poly *a);
void poly_frombytes_13bit(poly *r, const uint8_t *a);

// Vector Wrappers
void polyvec_tobytes_13bit(uint8_t *r, const polyvec *a);
void polyvec_frombytes_13bit(polyvec *r, const uint8_t *a);
void polyvec_compress_u(uint8_t *r, const polyvec *a);
void polyvec_decompress_u(polyvec *r, const uint8_t *a);
void polyvec_ntt(polyvec *r);
void polyvec_invntt_tomont(polyvec *r);


#endif // RUDRAKSH_NTT_H