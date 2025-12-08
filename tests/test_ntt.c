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

    return 0;
}