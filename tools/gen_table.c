#include <stdio.h>
#include <stdint.h>

// 參數來源: rudraksh_params.h / rudraksh_ntt.h
#define Q 7681
#define N 64
#define ZETA 202 // 這是您剛剛算出來的

int main() {
    int32_t f = 1; // 使用明確的 32-bit 整數避免平台差異

    printf("/* \n");
    printf(" * Twiddle Factors for Rudraksh (KEM-poly64)\n");
    printf(" * Modulus Q = %d, Zeta = %d, Size N = %d\n", Q, ZETA, N);
    printf(" * Order: Natural Order (powers of zeta: 0, 1, 2, ..., N-1)\n");
    printf(" */\n\n");
    
    printf("#include \"rudraksh_ntt.h\"\n\n");
    printf("const int16_t zetas[RUDRAKSH_N] = {\n");

    for(int i = 0; i < N; i++) {
        // 每行印 8 個數字，保持排版整潔
        if (i % 8 == 0) printf("    ");
        
        printf("%5d", f);
        
        // 最後一個數字後不加逗號，其他都要加
        if (i != N - 1) printf(",");
        
        // 換行控制
        if (i % 8 == 7) printf("\n");
        else printf(" ");
        
        // 計算下一個冪次: f = (f * ZETA) % Q
        // 注意: 這裡使用 int32_t 計算，7681 * 202 = 1,551,562，不會溢位
        f = (f * ZETA) % Q;
    }
    printf("};\n");
    
    return 0;
}