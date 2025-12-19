#ifndef RUDRAKSH_MATH_H
#define RUDRAKSH_MATH_H

#include <stdint.h>
#include "rudraksh_params.h"

// ==========================================================
// 1. NTT 專用常數
// ==========================================================
#define RUDRAKSH_ZETA 202

// ==========================================================
// 2. 資料結構
// ==========================================================

// 多項式結構
typedef struct {
    int16_t coeffs[RUDRAKSH_N];
} poly;

// 多項式向量 (矩陣運算用)
typedef struct {
    poly vec[RUDRAKSH_K];
} polyvec;

// 多項式矩陣 (RUDRAKSH_K x RUDRAKSH_K)
typedef struct {
    poly matrix[RUDRAKSH_K][RUDRAKSH_K];
} polymat;

// ==========================================================
// 3. 全域變數宣告
// ==========================================================
extern const int16_t zetas[RUDRAKSH_N];

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
    // add new
void poly_zero(poly *p); 

// NTT(poly) 運算
    // 加法
void poly_add(poly *r, const poly *a, const poly *b);          // r = a + b
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b); // Vector r = Vector a + Vector b
void poly_sub(poly *r, const poly *a, const poly *b);
    //乘法
void poly_basemul_acc(poly *r, const poly *a, const poly *b);
void poly_matrix_trans_vec_mul(polyvec *b, const polymat *A, const polyvec *s); // matrix (轉置) - vector 乘法
void poly_matrix_vec_mul(polyvec *b, const polymat *A, const polyvec *s);        // matrix - vector 乘法
void poly_vector_vector_mul(poly *c, const polyvec *b, const polyvec *s); // vector - vector 乘法


// ==========================================================
// 5. 資料轉換與壓縮 (Member A)
// ==========================================================

// 訊息 <-> 多項式 (B=2)
void arrange_msg(poly *m, const uint8_t msg[16]);
void original_msg(uint8_t msg[16], const poly *m);

// R_2^B (R4) -> Rq
void poly_encode(poly *r, const poly *m);
void poly_decode(poly *m, const poly *noisy_poly);



// 向量 U 壓縮 (10 bits)
void poly_compress_u(uint8_t *r, const poly *a);
void poly_decompress_u(poly *r, const uint8_t *a);

// 密文 V 壓縮 (3 bits)
void poly_compress_v(uint8_t *r, const poly *a);
void poly_decompress_v(poly *r, const uint8_t *a);

// ==========================================================
// 6. 取樣函式 (Member B)
// ==========================================================

// void poly_uniform(poly *p, const uint8_t *seed, uint16_t nonce);
void polyvec_cbd_eta(polyvec *s,polyvec *e, const uint8_t *key); // 生成 s or e with eta=1
void poly_cbd_eta(poly *e, const uint8_t *key, const uint8_t nonce); // 生成 e'' with eta=2
void poly_matrixA_generator(polymat *a, const uint8_t *seed); // 生成矩陣 A (ascon xof)

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
// 可能要加入多項式矩陣乘法和加法的函式宣告

#endif // RUDRAKSH_MATH_H