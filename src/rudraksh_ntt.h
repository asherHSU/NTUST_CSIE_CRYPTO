#ifndef RUDRAKSH_NTT_H
#define RUDRAKSH_NTT_H

#include <stdint.h>

// [關鍵修正] 引入專案的全域參數，直接使用裡面的 KYBER_N 和 KYBER_Q
#include "rudraksh_params.h" 

/**
 * Rudraksh Mathematical Core
 * 這裡只定義與 "運算實作" 有關的常數，不重複定義 N 和 Q
 */

// 1. NTT 專用常數 (這是剛剛算出來的)
#define RUDRAKSH_ZETA 202

// 2. 資料結構 (依賴 params.h 中的 KYBER_N)
typedef struct {
    int16_t coeffs[KYBER_N];
} poly;

// 告訴大家 zetas 陣列存在於某個 .c 檔中，大家都可以用
extern const int16_t zetas[KYBER_N];

// 3. 數學函式宣告
void poly_ntt(poly *p);
void poly_invntt_tomont(poly *p);
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b);
// ... 其他運算函式

#endif // RUDRAKSH_NTT_H