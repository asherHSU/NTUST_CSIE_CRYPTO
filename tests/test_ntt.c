#include <stdio.h>
#include "../src/rudraksh_params.h"
#include "../src/rudraksh_math.h"

void print_poly(const char* msg, poly *p) {
    printf("%s: [", msg);
    for(int i=0; i<8; i++) printf("%d, ", p->coeffs[i]); // 只印前8個
    printf("...]\n");
}

int main() {
    printf("=== Rudraksh NTT Unit Test ===\n");
    printf("N=%d, Q=%d, Zeta=%d\n", RUDRAKSH_N, RUDRAKSH_Q, RUDRAKSH_ZETA);

    poly a;
    
    // 初始化一個簡單的多項式 a(x) = 1 + 1x + 1x^2 ... (全為1)
    for(int i=0; i<RUDRAKSH_N; i++) a.coeffs[i] = 1;
    
    print_poly("Input", &a);

    // 執行 NTT
    poly_ntt(&a);

    print_poly("Output (NTT)", &a);
    
    // 簡單驗證：
    // 對於全 1 的輸入，NTT 的結果第一個係數應該是 N (或者 N mod Q)
    // 其他係數應該是 0 (這是 DFT 的性質)
    printf("\nVerification Check:\n");
    printf("Coeff[0] should be %d. Actual: %d\n", RUDRAKSH_N, a.coeffs[0]);
    printf("Coeff[1] should be 0.  Actual: %d\n", a.coeffs[1]);

    if (a.coeffs[0] == RUDRAKSH_N && a.coeffs[1] == 0) {
        printf(">> NTT Test PASSED! (Basic Property Check)\n");
    } else {
        printf(">> NTT Test FAILED!\n");
    }

    // 在 main() 函數的最後面加入這段：

    printf("\n=== Inverse NTT Test ===\n");
    // 接續剛剛的結果，現在 a 處於 NTT 域
    poly_invntt_tomont(&a);
    
    print_poly("Output (INTT)", &a);

    // 驗證是否變回全 1
    int fail = 0;
    for(int i=0; i<RUDRAKSH_N; i++) {
        if (a.coeffs[i] != 1) {
            fail = 1;
            printf("Error at index %d: expected 1, got %d\n", i, a.coeffs[i]);
            break;
        }
    }

    if (!fail) {
        printf(">> Round-Trip Test (INTT(NTT(x)) == x) PASSED!\n");
    } else {
        printf(">> Round-Trip Test FAILED!\n");
    }

    // =========================================================
    // Part 3: Vector-Vector Multiplication Test (c = b^T * s)
    // =========================================================
    printf("\n=== Vector-Vector Mul Test (Point-wise & Accumulate) ===\n");
    
    polyvec b, s;
    poly c;
    
    // 1. 初始化測試資料：將所有係數設為 1
    // 我們假設 b 和 s 已經是在 NTT 域中的資料
    for(int i = 0; i < RUDRAKSH_K; i++) {
        for(int j = 0; j < RUDRAKSH_N; j++) {
            b.vec[i].coeffs[j] = 1;
            s.vec[i].coeffs[j] = 1;
        }
    }

    // 2. 執行向量內積 (Inner Product)
    // c = b[0]*s[0] + b[1]*s[1] + ... + b[K-1]*s[K-1]
    poly_vector_vector_mul(&c, &b, &s);

    // 3. 驗證結果
    // 因為 b[i]*s[i] = 1*1 = 1
    // 總共有 K 個元素相加，所以結果應該等於 K (矩陣維度)
    // 注意：這裡假設模數 Q 遠大於 K (7681 > 9)，所以不會發生溢位
    fail = 0;
    int expected_val = RUDRAKSH_K; 
    
    for(int i = 0; i < RUDRAKSH_N; i++) {
        if (c.coeffs[i] != expected_val) {
            fail = 1;
            printf("VecMul Error at coeff %d: expected %d, got %d\n", 
                   i, expected_val, c.coeffs[i]);
            break;
        }
    }

    if (!fail) {
        printf(">> Vector-Vector Mul Test PASSED! (All coeffs are %d)\n", expected_val);
    } else {
        printf(">> Vector-Vector Mul Test FAILED!\n");
    }

    // =========================================================
    // Part 4: Matrix-Vector Multiplication Test (res = A^T * s)
    // =========================================================
    printf("\n=== Matrix-Vector Mul Test (b = A^T * s) ===\n");

    polymat A;
    polyvec res_vec;

    // 1. 初始化矩陣 A：將所有係數設為 1
    // s 向量我們沿用上一個測試的設定 (全為 1)
    for(int i = 0; i < RUDRAKSH_K; i++) {
        for(int j = 0; j < RUDRAKSH_K; j++) {
            for(int k = 0; k < RUDRAKSH_N; k++) {
                A.matrix[i][j].coeffs[k] = 1;
            }
        }
    }

    // 2. 執行矩陣向量乘法
    // res_vec[i] = sum( A[j][i] * s[j] )  <-- 注意這是轉置乘法
    // 由於我們輸入全都是 1，轉置與否結果數值是一樣的
    poly_matrix_trans_vec_mul(&res_vec, &A, &s);

    // 3. 驗證結果
    // 每個結果向量的多項式，都是 K 個 (1*1) 的累加
    // 所以 res_vec 中的每一個多項式的每一個係數都應該是 K
    fail = 0;
    for(int i = 0; i < RUDRAKSH_K; i++) {
        for(int j = 0; j < RUDRAKSH_N; j++) {
            if (res_vec.vec[i].coeffs[j] != expected_val) {
                fail = 1;
                printf("MatMul Error at vec[%d].coeff[%d]: expected %d, got %d\n", 
                       i, j, expected_val, res_vec.vec[i].coeffs[j]);
                goto mat_test_end; // 跳出多層迴圈
            }
        }
    }

    mat_test_end:
    if (!fail) {
        printf(">> Matrix-Vector Mul Test PASSED! (All vec coeffs are %d)\n", expected_val);
    } else {
        printf(">> Matrix-Vector Mul Test FAILED!\n");
    }
    return 0;
}