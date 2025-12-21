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


}

// 輔助函式：將多項式設為純常數 (c + 0x + 0x^2...)
void poly_set_constant(poly *p, int16_t val) {
    poly_zero(p);
    p->coeffs[0] = val;
}

// 輔助函式：檢查多項式是否等於某個常數
int poly_check_constant(const poly *p, int16_t val) {
    if (p->coeffs[0] != val) return 0;
    for (int i = 1; i < RUDRAKSH_N; i++) {
        if (p->coeffs[i] != 0) return 0;
    }
    return 1;
}

void test_schoolbook_arithmetic() {
    polyvec va, vb;
    polymat ma;
    poly a, b, r;
    
    printf("=== Starting Schoolbook Domain Arithmetic Tests ===\n");

    // -----------------------------------------------------
    // 1. 基本多項式乘法測試 (x * x^63 = x^64 = -1)
    // 這是驗證 Schoolbook 獨有的 x^n + 1 約減邏輯
    // -----------------------------------------------------
    printf("[Test 1] Polynomial Reduction (x * x^63 = -1): ");
    poly_zero(&a); a.coeffs[1] = 1;  // a = x
    poly_zero(&b); b.coeffs[63] = 1; // b = x^63
    poly_zero(&r);
    poly_basemul_acc_serial(&r, &a, &b);
    // 預期結果 r = -1, 在 mod Q 下為 7680
    assert(r.coeffs[0] == RUDRAKSH_Q - 1); 
    for(int i=1; i<RUDRAKSH_N; i++) assert(r.coeffs[i] == 0);
    printf("PASSED\n");

    // -----------------------------------------------------
    // 2. 向量內積測試 (Vector-Vector Dot Product)
    // -----------------------------------------------------
    printf("[Test 2] Vector-Vector Dot Product: ");
    // va = [2, 2, ..., 2], vb = [3, 3, ..., 3] (皆為常數多項式)
    // 內積結果 = sum_{i=0}^{K-1} (2 * 3) = 9 * 6 = 54
    for(int i=0; i<RUDRAKSH_K; i++) {
        poly_set_constant(&va.vec[i], 2);
        poly_set_constant(&vb.vec[i], 3);
    }
    poly res_poly;
    poly_vector_vector_mul(&res_poly, &va, &vb);
    assert(poly_check_constant(&res_poly, 54));
    printf("PASSED\n");

    // -----------------------------------------------------
    // 3. 矩陣乘法與轉置測試
    // -----------------------------------------------------
    printf("[Test 3] Matrix-Vector vs Transpose: ");
    
    // 初始化矩陣 ma 為 0
    for(int i=0; i<RUDRAKSH_K; i++)
        for(int j=0; j<RUDRAKSH_K; j++)
            poly_zero(&ma.matrix[i][j]);
    
    // 設定 ma[0][1] = 2 (第 0 列, 第 1 行)
    // 設定 ma[0][0] = 1 (第 0 列, 第 0 行)
    poly_set_constant(&ma.matrix[0][0], 1);
    poly_set_constant(&ma.matrix[0][1], 2);

    // 構造向量 s = [1, 1, 1...]
    polyvec s;
    for(int i=0; i<RUDRAKSH_K; i++) poly_set_constant(&s.vec[i], 1);

    // (A) 標準矩陣向量乘法 b = A * s
    // b[0] = A[0][0]*s[0] + A[0][1]*s[1] = 1*1 + 2*1 = 3
    polyvec b_norm;
    poly_matrix_vec_mul(&b_norm, &ma, &s);
    assert(poly_check_constant(&b_norm.vec[0], 3));
    assert(poly_check_constant(&b_norm.vec[1], 0));

    // (B) 轉置矩陣向量乘法 b = A^T * s
    // b[0] = A[0][0]*s[0] + A[1][0]*s[1]... = 1*1 + 0 = 1
    // b[1] = A[0][1]*s[0] + A[1][1]*s[1]... = 2*1 + 0 = 2
    polyvec b_trans;
    poly_matrix_trans_vec_mul(&b_trans, &ma, &s);
    
    if (poly_check_constant(&b_trans.vec[0], 1) && poly_check_constant(&b_trans.vec[1], 2)) {
        printf("PASSED\n");
    } else {
        printf("FAILED (Transpose logic error!)\n");
        printf("  b_trans[0][0] got %d, expected 1\n", b_trans.vec[0].coeffs[0]);
        printf("  b_trans[1][0] got %d, expected 2\n", b_trans.vec[1].coeffs[0]);
    }

    // -----------------------------------------------------
    // 4. 溢位與大數模運算測試
    // -----------------------------------------------------
    printf("[Test 4] High Value Modulo Check: ");
    // (Q-1) + (Q-1) = 2Q-2 = 7680 + 7680 = 15360. 15360 % 7681 = 7679
    poly_set_constant(&a, RUDRAKSH_Q - 1);
    poly_add(&r, &a, &a);
    assert(poly_check_constant(&r, RUDRAKSH_Q - 2));

    // (Q-1) * (Q-1) 應等於 1 (因為 -1 * -1 = 1)
    // 這裡直接測試係數乘法
    int16_t mul_res = fqmul(RUDRAKSH_Q - 1, RUDRAKSH_Q - 1);
    assert(mul_res == 1);
    printf("PASSED\n");

    printf("\nAll Schoolbook Domain Arithmetic Tests PASSED!\n");
}

int main()
{
    printf("\n=============================================\n");
    printf("   Math Tests\n");
    printf("=============================================\n");

    test_ntt_arithmetic();

    printf("\n=============================================\n");
    printf("   End of Tests\n");
    printf("=============================================\n");
}