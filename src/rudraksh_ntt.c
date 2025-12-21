#include <stdint.h>
#include <string.h>
#include "rudraksh_math.h"
#include "rudraksh_params.h"

// 這是我們在 step 2 生成的數據，透過 extern 引用
extern const int16_t zetas[RUDRAKSH_N];


// =========================================================
// 1. base
// =========================================================
/**
 * 基礎模乘法 (Modular Multiplication)
 * 計算 (a * b) % Q
 * 注意：這裡使用 int32_t 來防止溢位 (7681 * 7681 < 2^31)
 */
// 模乘法
int16_t fqmul(int16_t a, int16_t b) {
    return (int32_t)a * b % RUDRAKSH_Q;
}

/**
 * 基礎模加法 (Modular Addition)
 * 計算 (a + b) % Q
 */
int16_t fqadd(int16_t a, int16_t b) {
    int16_t res = a + b;
    if (res >= RUDRAKSH_Q) res -= RUDRAKSH_Q;
    return res;
}

/**
 * 基礎模減法 (Modular Subtraction)
 * 計算 (a - b) % Q
 */
int16_t fqsub(int16_t a, int16_t b) {
    int16_t res = a - b;
    if (res < 0) res += RUDRAKSH_Q;
    return res;
}

// 多項式歸零
void poly_zero(poly *p) {
    memset(p->coeffs, 0, sizeof(p->coeffs));
}

void polyvec_zero(polyvec *pv) {
    // 同樣地，可以直接對整個向量結構進行 memset
    memset(pv, 0, sizeof(polyvec));
}
// =========================================================
// 2.NTT / INTT
// =========================================================
#define INV_2 3841
// 需使用 Python 生成的正確表
// static const int16_t zetas[64] = { /* ... */ };
// static const int16_t zetas_inv[64] = { /* ... */ };

// 論文 Algorithm 1 的化約函數 (保持不變)
static int16_t rudraksh_reduce(int32_t c) {
    int32_t c0 = c & 0x1FFF;
    int32_t c1 = (c >> 13) & 0xF;
    int32_t c2 = (c >> 17) & 0xF;
    int32_t c3 = (c >> 21) & 0xF;
    int32_t c4 = (c >> 25) & 0x1;
    int32_t temp0 = c4 + c3;
    int32_t temp1 = temp0 + c2;
    int32_t temp2 = temp1 + c1;
    int32_t temp3 = (temp2 << 1) - temp0;
    int32_t temp4 = (temp3 << 4) - temp1;
    int32_t temp5 = (temp4 << 4) - temp2;
    int32_t temp6 = temp5 + c0;
    int32_t res = (-(c4 << 12)) + temp6;
    while (res < 0) res += RUDRAKSH_Q;
    while (res >= RUDRAKSH_Q) res -= RUDRAKSH_Q;
    return (int16_t)res;
}

// 正向 NTT: Cooley-Tukey DIF (輸入自然順序 -> 輸出位元反轉)
void poly_ntt(poly *p) {
    int t = 32, k = 1;
    for (int m = 1; m < 64; m <<= 1) {
        for (int i = 0; i < m; i++) {
            int16_t zeta = zetas[k++]; // zetas 必須按位元反轉順序預生成
            for (int j = i * 2 * t; j < i * 2 * t + t; j++) {
                int16_t u = p->coeffs[j];
                int16_t v = p->coeffs[j + t];
                p->coeffs[j] = rudraksh_reduce(u + v);
                // 負循環關鍵：先減後乘
                p->coeffs[j + t] = rudraksh_reduce((int32_t)(u - v + RUDRAKSH_Q) * zeta);
            }
        }
        t >>= 1;
    }
}

// 反向 INTT: Gentleman-Sande DIT (輸入位元反轉 -> 輸出自然順序)
void poly_invntt(poly *p) {
    int t = 1;
    for (int m = 32; m >= 1; m >>= 1) {
        int start_k = m; 
        for (int i = 0; i < m; i++) {
            // 注意：這裡使用反向因子，且順序與正向對稱
            int16_t zeta_inv = zetas_inv[start_k++]; 
            for (int j = i * 2 * t; j < i * 2 * t + t; j++) {
                int16_t u = p->coeffs[j];
                // 負循環關鍵：先乘後加減
                int16_t v = rudraksh_reduce((int32_t)p->coeffs[j + t] * zeta_inv);
                
                int16_t res_u = rudraksh_reduce(u + v);
                int16_t res_v = rudraksh_reduce(u - v + RUDRAKSH_Q);
                
                // 論文第 16 頁：每一層除以 2 (INV_2 = 3841)
                p->coeffs[j] = rudraksh_reduce((int32_t)res_u * INV_2);
                p->coeffs[j + t] = rudraksh_reduce((int32_t)res_v * INV_2);
            }
        }
        t <<= 1;
    }
}


// =========================================================
// 3.NTT 域點乘
// =========================================================

// 多項式點對點乘法 (Point-wise Multiplication)
// r = a * b (在 NTT 域中)
void poly_basemul_acc(poly *r, const poly *a, const poly *b) {
    for (int i = 0; i < RUDRAKSH_N; i++) {
        // r[i] = r[i] + (a[i] * b[i])
        int16_t product = fqmul(a->coeffs[i], b->coeffs[i]);
        r->coeffs[i] = fqadd(r->coeffs[i], product);
    }
}

// 非 NTT 域多項式乘法累加 (Schoolbook Multiplication)
void poly_basemul_acc_serial(poly *r, const poly *a, const poly *b) {
    int32_t c[2 * RUDRAKSH_N] = {0}; // 用於存放中間結果，長度需要 2N

    // 1. 執行標準卷積
    for (int i = 0; i < RUDRAKSH_N; i++) {
        for (int j = 0; j < RUDRAKSH_N; j++) {
            c[i + j] = fqadd(c[i + j], fqmul(a->coeffs[i], b->coeffs[j]));
        }
    }

    // 2. 根據 x^n + 1 = 0 進行約減 (Reduction)
    // 原理：x^n = -1, x^{n+1} = -x, 依此類推
    for (int i = 0; i < RUDRAKSH_N; i++) {
        // r[i] = r[i] + (低次項c[i] - 高次項c[i+n])
        int16_t reduced = fqsub(c[i], c[i + RUDRAKSH_N]);
        r->coeffs[i] = fqadd(r->coeffs[i], reduced);
    }
}

void poly_matrix_trans_vec_mul(polyvec *b, const polymat *A, const polyvec *s) {
    // 1. 初始化結果向量 b 為 0
    for (int i = 0; i < RUDRAKSH_K; i++) {
        poly_zero(&b->vec[i]);
    }

    // 2. 矩陣運算 (注意 A 的索引是 [j][i] 而非 [i][j]，因為是 A^T)
    // b[i] = sum( A[j][i] * s[j] ) for j in 0..K-1
    for (int i = 0; i < RUDRAKSH_K; i++) {
        for (int j = 0; j < RUDRAKSH_K; j++) {
            // 將 A[j][i] 與 s[j] 相乘並累加到 b[i]
            poly_basemul_acc_serial(&b->vec[i], &A->matrix[j][i], &s->vec[j]);
        }
    }
}

// 計算 b = A * s (標準矩陣向量乘法)
void poly_matrix_vec_mul(polyvec *b, const polymat *A, const polyvec *s) {
    for (int i = 0; i < RUDRAKSH_K; i++) {
        poly_zero(&b->vec[i]); // 初始化為 0
        for (int j = 0; j < RUDRAKSH_K; j++) {
            // 注意：這裡是 A[i][j]，代表第 i 列第 j 行
            poly_basemul_acc_serial(&b->vec[i], &A->matrix[i][j], &s->vec[j]);
        }
        // 根據實作，這裡可能需要做 montgomery reduction 或保留在 montgomery domain
        // 假設 poly_basemul_acc 已經處理好累加
    }
}

void poly_vector_vector_mul(poly *c, const polyvec *b, const polyvec *s) {
    // 1. 初始化結果多項式 c 為 0
    poly_zero(c);

    // 2. 內積運算
    // c = sum( b[i] * s[i] )
    for (int i = 0; i < RUDRAKSH_K; i++) {
        // 將 b[i] 與 s[i] 相乘並累加到 c
        poly_basemul_acc_serial(c, &b->vec[i], &s->vec[i]);
    }
}

// =========================================================
// 3. poly(vec) 加法/減法
// =========================================================

// 單一多項式加法: r = a + b
void poly_add(poly *r, const poly *a, const poly *b) {
    for (int i = 0; i < RUDRAKSH_N; i++) {
        r->coeffs[i] = fqadd(a->coeffs[i], b->coeffs[i]);
    }
}

// 多項式向量加法: r = a + b
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b) {
    for (int i = 0; i < RUDRAKSH_K; i++) {
        poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
    }
}

// 多項式向量減法: r = a - b
void poly_sub(poly *r, const poly *a, const poly *b) {
    for (int i = 0; i < RUDRAKSH_N; i++) {
        r->coeffs[i] = fqsub(a->coeffs[i], b->coeffs[i]);
    }
}

//