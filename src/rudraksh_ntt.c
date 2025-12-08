#include <stdint.h>
#include "rudraksh_math.h"
#include "rudraksh_params.h"

// 這是我們在 step 2 生成的數據，透過 extern 引用
extern const int16_t zetas[RUDRAKSH_N];

/**
 * 基礎模乘法 (Modular Multiplication)
 * 計算 (a * b) % Q
 * 注意：這裡使用 int32_t 來防止溢位 (7681 * 7681 < 2^31)
 */
int16_t fqmul(int16_t a, int16_t b) {
    int32_t res = (int32_t)a * b;
    return (int16_t)(res % RUDRAKSH_Q);
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

/**
 * 位元反轉 (Bit Reversal) 用於 N=64
 * 用於 Cooley-Tukey NTT 的輸入重排
 * 例如: index 1 (000001) -> index 32 (100000)
 */
void bitrev_vector(int16_t* poly_coeffs) {
    int i, j;
    int16_t temp;
    
    j = 0;
    for (i = 1; i < RUDRAKSH_N; i++) {
        int bit = RUDRAKSH_N >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        
        // 交換
        if (i < j) {
            temp = poly_coeffs[i];
            poly_coeffs[i] = poly_coeffs[j];
            poly_coeffs[j] = temp;
        }
    }
}
/**
 * Forward NTT (Number Theoretic Transform)
 * 演算法: Iterative Cooley-Tukey (Decimation-in-Time)
 * 複雜度: O(N log N)
 */
void poly_ntt(poly *p) {
    // 1. 先進行位元反轉重排 (Bit-reversal permutation)
    bitrev_vector(p->coeffs);

    int len, start, j;
    int16_t zeta;
    
    // 2. 蝴蝶運算 (Butterfly Operations)
    // 層數 loop: len = 2, 4, 8, 16, 32, 64
    for (len = 2; len <= RUDRAKSH_N; len <<= 1) {
        
        int half_len = len >> 1;
        // 旋轉因子 zeta 步進: 因為我們有 N 個 zeta，但每一層只用到部分
        // 在標準 CT 演算法中，每一層使用的 zeta 間隔不同
        int step = RUDRAKSH_N / len; 

        // 區塊 loop
        for (start = 0; start < RUDRAKSH_N; start += len) {
            
            // 每一層的第一個 butterfly 使用 zeta^0 = 1
            // 但為了對應我們產生的 natural order table，我們需要特殊的索引邏輯
            // 這裡簡化邏輯：直接針對每個蝴蝶計算對應的 zeta
            // (注意：這不是最高效的查表法，但是最容易理解且正確的寫法)
            
            for (j = 0; j < half_len; j++) {
                // 取得正確的旋轉因子
                // 在這一層，我們需要 zetas[step * j]
                // 注意：我們的 zetas 表是 natural order
                zeta = zetas[j * step];

                // 蝴蝶運算核心 (CT Butterfly)
                // U = A
                // V = B * zeta
                // New A = U + V
                // New B = U - V
                
                int16_t u = p->coeffs[start + j];
                int16_t v = fqmul(p->coeffs[start + j + half_len], zeta);

                p->coeffs[start + j]            = fqadd(u, v);
                p->coeffs[start + j + half_len] = fqsub(u, v);
            }
        }
    }
}

/**
 * 模反元素 (Modular Inverse) 用於計算 N^-1
 * 利用費馬小定理: a^(Q-2) = a^-1 mod Q
 */
int16_t fqinv(int16_t a) {
    int32_t base = a;
    int32_t exp = RUDRAKSH_Q - 2;
    int32_t res = 1;
    
    while (exp > 0) {
        if (exp & 1) res = (res * base) % RUDRAKSH_Q;
        base = (base * base) % RUDRAKSH_Q;
        exp >>= 1;
    }
    return (int16_t)res;
}

/**
 * Inverse NTT (INTT) - 修正版
 * 演算法: Inverse Iterative Cooley-Tukey (Symmetric to Forward NTT)
 * 邏輯: Bit-Reverse -> Butterfly(using zeta^-1) -> Scale by N^-1
 */
void poly_invntt_tomont(poly *p) {
    // [修正 1] Bit-reversal 必須在開始時做，跟 Forward NTT 一樣
    bitrev_vector(p->coeffs);

    int len, start, j;
    int16_t zeta, inv_zeta;
    
    // [修正 2] 使用跟 Forward NTT 完全一樣的迴圈結構 (Bottom-up)
    for (len = 2; len <= RUDRAKSH_N; len <<= 1) {
        int half_len = len >> 1;
        int step = RUDRAKSH_N / len;
        
        for (start = 0; start < RUDRAKSH_N; start += len) {
            for (j = 0; j < half_len; j++) {
                // 取得正向的 zeta
                zeta = zetas[j * step];
                
                // [修正 3] 計算 zeta 的模反元素 (zeta^-1)
                // 在優化版中這應該查表，但為了正確性我們先即時計算
                inv_zeta = fqinv(zeta);

                // [修正 4] 使用標準 CT 蝴蝶運算，但是乘上 inv_zeta
                // U = A
                // V = B * zeta^-1
                int16_t u = p->coeffs[start + j];
                int16_t v = fqmul(p->coeffs[start + j + half_len], inv_zeta);

                p->coeffs[start + j]            = fqadd(u, v);
                p->coeffs[start + j + half_len] = fqsub(u, v);
            }
        }
    }

    // [修正 5] 最後縮放：除以 N (乘以 N^-1 mod Q)
    // 64^-1 mod 7681 = 7561 (也就是 -120)
    int16_t n_inv = fqinv(RUDRAKSH_N); 
    
    for(int i=0; i<RUDRAKSH_N; i++) {
        p->coeffs[i] = fqmul(p->coeffs[i], n_inv);
    }
    
    // [修正 6] 移除最後的 bitrev，因為我們一開始就做過了，
    // Cooley-Tukey 的輸出本身就是 Natural Order。
}