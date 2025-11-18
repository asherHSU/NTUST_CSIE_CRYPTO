#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define Q 7681
#define N 64
#define ROOT_ORDER 128 // 2 * N

// 模冪運算 (Modular Exponentiation): (base^exp) % mod
int intmod_pow(int base, int exp, int mod) {
    int res = 1;
    base = base % mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = (res * base) % mod;
        base = (base * base) % mod;
        exp /= 2;
    }
    return res;
}

// 檢查是否為 Primitive 128-th root of unity
bool is_primitive_root(int w, int order, int mod) {
    // 1. w^order must be 1
    if (intmod_pow(w, order, mod) != 1) return false;
    // 2. w^(order/2) must NOT be 1 (it should be -1, i.e., mod-1)
    if (intmod_pow(w, order / 2, mod) == 1) return false;
    return true;
}

int main() {
    printf("Searching for primitive %d-th root of unity modulo %d...\n", ROOT_ORDER, Q);
    
    int zeta = 0;
    // 從 2 開始暴力搜尋
    for (int i = 2; i < Q; i++) {
        if (is_primitive_root(i, ROOT_ORDER, Q)) {
            zeta = i;
            break;
        }
    }

    if (zeta != 0) {
        printf("Found zeta: %d\n", zeta);
        printf("Verification: %d^%d %% %d = %d\n", zeta, ROOT_ORDER, Q, intmod_pow(zeta, ROOT_ORDER, Q));
        printf("Verification: %d^%d %% %d = %d (Should be %d)\n", zeta, ROOT_ORDER/2, Q, intmod_pow(zeta, ROOT_ORDER/2, Q), Q-1);
        
        // 產生 C 語言 header 格式
        printf("\n// Copy this into rudraksh_ntt.h\n");
        printf("#define RUDRAKSH_ZETA %d\n", zeta);
    } else {
        printf("Error: No root found. Check parameters.\n");
    }

    return 0;
}