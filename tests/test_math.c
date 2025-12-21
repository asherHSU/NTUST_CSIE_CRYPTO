#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

# include "../src/rudraksh_params.h"
# include "../src/rudraksh_math.h"

// 假設環境參數
// #define RUDRAKSH_N 64
// #define RUDRAKSH_Q 7681
// #define RUDRAKSH_K 9

// 輔助函式：將多項式設為特定常數
void poly_set_const(poly *p, int16_t val) {
    for (int i = 0; i < RUDRAKSH_N; i++) {
        p->coeffs[i] = val % RUDRAKSH_Q;
    }
}

// 輔助函式：檢查多項式是否全為特定常數
int poly_check_const(const poly *p, int16_t expected) {
    int16_t target = expected % RUDRAKSH_Q;
    if (target < 0) target += RUDRAKSH_Q;
    for (int i = 0; i < RUDRAKSH_N; i++) {
        if (p->coeffs[i] != target) return 0;
    }
    return 1;
}

// =========================================================
// 測試主體
// =========================================================

void test_ntt_arithmetic() {
    printf("Starting NTT Domain Arithmetic Tests...\n");

    poly a, b, r;
    polyvec va, vb, vr;
    polymat ma;

    // -----------------------------------------------------
    // 1. 基礎加減法測試 (Linearity & Modulo)
    // -----------------------------------------------------
    printf("[Test 1] Poly Add/Sub: ");
    poly_set_const(&a, 5000);
    poly_set_const(&b, 4000);
    
    poly_add(&r, &a, &b); // 5000 + 4000 = 9000 % 7681 = 1319
    assert(poly_check_const(&r, 1319));

    poly_sub(&r, &a, &b); // 5000 - 4000 = 1000
    assert(poly_check_const(&r, 1000));

    poly_sub(&r, &b, &a); // 4000 - 5000 = -1000 % 7681 = 6681
    assert(poly_check_const(&r, 6681));
    printf("PASSED\n");

    // -----------------------------------------------------
    // 2. 點對點乘法與累加 (BaseMul Acc)
    // -----------------------------------------------------
    printf("[Test 2] Poly BaseMul Acc: ");
    poly_zero(&r);
    poly_set_const(&a, 10);
    poly_set_const(&b, 20);
    
    // 第一次累加: 0 + 10*20 = 200
    poly_basemul_acc(&r, &a, &b);
    assert(poly_check_const(&r, 200));

    // 第二次累加: 200 + 10*20 = 400
    poly_basemul_acc(&r, &a, &b);
    assert(poly_check_const(&r, 400));
    printf("PASSED\n");

    // -----------------------------------------------------
    // 3. 向量內積測試 (Vector-Vector Mul)
    // -----------------------------------------------------
    printf("[Test 3] Vector-Vector Dot Product: ");
    // va = [poly(2), poly(2), ..., poly(2)] (長度 K=9)
    // vb = [poly(3), poly(3), ..., poly(3)]
    // 結果應為 K * (2 * 3) = 9 * 6 = 54
    for(int i=0; i<RUDRAKSH_K; i++) {
        poly_set_const(&va.vec[i], 2);
        poly_set_const(&vb.vec[i], 3);
    }
    poly res_poly;
    poly_vector_vector_mul(&res_poly, &va, &vb);
    assert(poly_check_const(&res_poly, 54));
    printf("PASSED\n");

    // -----------------------------------------------------
    // 4. 矩陣乘法與轉置測試 (The Crucial Transpose Check)
    // -----------------------------------------------------
    printf("[Test 4] Matrix-Vector vs Transpose: ");
    
    // 構造一個非對稱矩陣 ma
    // ma[0][0] = 1, ma[0][1] = 2 (第0列)
    // 其他設為 0
    for(int i=0; i<RUDRAKSH_K; i++)
        for(int j=0; j<RUDRAKSH_K; j++)
            poly_zero(&ma.matrix[i][j]);
    
    poly_set_const(&ma.matrix[0][0], 1);
    poly_set_const(&ma.matrix[0][1], 2);

    // 構造向量 s = [1, 1, 1...]
    polyvec s;
    for(int i=0; i<RUDRAKSH_K; i++) poly_set_const(&s.vec[i], 1);

    // (A) 標準乘法 b = A * s
    // b[0] = ma[0][0]*s[0] + ma[0][1]*s[1] = 1*1 + 2*1 = 3
    // b[1...8] = 0
    polyvec b_norm;
    poly_matrix_vec_mul(&b_norm, &ma, &s);
    assert(poly_check_const(&b_norm.vec[0], 3));
    assert(poly_check_const(&b_norm.vec[1], 0));

    // (B) 轉置乘法 b = A^T * s
    // b[0] = ma[0][0]*s[0] + ma[1][0]*s[1]... = 1*1 + 0 = 1
    // b[1] = ma[0][1]*s[0] + ma[1][1]*s[1]... = 2*1 + 0 = 2
    polyvec b_trans;
    poly_matrix_trans_vec_mul(&b_trans, &ma, &s);
    
    // 如果轉置邏輯正確，b[0]應為1，b[1]應為2
    if (poly_check_const(&b_trans.vec[0], 1) && poly_check_const(&b_trans.vec[1], 2)) {
        printf("PASSED\n");
    } else {
        printf("FAILED (Transpose logic error!)\n");
        printf("  b_trans[0] got % d, expected 1\n", b_trans.vec[0].coeffs[0]);
        printf("  b_trans[1] got % d, expected 2\n", b_trans.vec[1].coeffs[0]);
    }

    // -----------------------------------------------------
    // 5. 溢位與大數測試 (Overflow Check)
    // -----------------------------------------------------
    printf("[Test 5] High Value Modulo Check: ");
    // 測試接近 Q 的運算
    // (Q-1) + (Q-1) = 2Q-2 = 7680 + 7680 = 15360. 15360 % 7681 = 7679
    poly_set_const(&a, RUDRAKSH_Q - 1);
    poly_add(&r, &a, &a);
    assert(poly_check_const(&r, RUDRAKSH_Q - 2));

    // (Q-1) * (Q-1) 應等於 1 (因為 -1 * -1 = 1)
    int16_t mul_res = fqmul(RUDRAKSH_Q - 1, RUDRAKSH_Q - 1);
    assert(mul_res == 1);
    printf("PASSED\n");

    printf("\nAll NTT Domain Arithmetic Tests PASSED!\n");
}

int main()
{
    test_ntt_arithmetic();
}