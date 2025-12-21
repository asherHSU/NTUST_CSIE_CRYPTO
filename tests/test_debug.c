#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

# include "../src/rudraksh_params.h"
# include "../src/rudraksh_crypto.h"
# include "../src/rudraksh_math.h"
# include "../src/rudraksh_random.h"

// ==========================================================
// 輔助巨集與函式
// ==========================================================
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED   "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RESET "\033[0m"

void print_failure(const char *msg) {
    printf("[%sFAIL%s] %s\n", COLOR_RED, COLOR_RESET, msg);
}

void print_pass(const char *msg) {
    printf("[%sPASS%s] %s\n", COLOR_GREEN, COLOR_RESET, msg);
}

void print_info(const char *msg) {
    printf("[%sINFO%s] %s\n", COLOR_YELLOW, COLOR_RESET, msg);
}

// 產生一個測試用的 polyvec (非隨機，便於重現)
void generate_test_polyvec(polyvec *v, int16_t start_val) {
    for(int i=0; i<RUDRAKSH_K; i++) {
        for(int j=0; j<RUDRAKSH_N; j++) {
            v->vec[i].coeffs[j] = (start_val + i*RUDRAKSH_N + j) % RUDRAKSH_Q;
        }
    }
}

// ==========================================================
// 測試 1: 減法邏輯 (檢查負數處理)
// ==========================================================
void debug_subtraction() {
    printf("\n=== Debug 1: Subtraction Logic (poly_sub) ===\n");
    poly a, b, r;
    
    // Case 1: 一般減法
    // 100 - 50 = 50
    memset(&a, 0, sizeof(poly)); memset(&b, 0, sizeof(poly));
    a.coeffs[0] = 100;
    b.coeffs[0] = 50;
    poly_sub(&r, &a, &b); // 假設你有這個 wrapper，或者直接測試 fqsub
    
    if (r.coeffs[0] != 50) {
        print_failure("100 - 50 != 50");
        printf("   Actual: %d\n", r.coeffs[0]);
    } else {
        print_pass("Positive subtraction OK");
    }

    // Case 2: 負數回繞 (重點！)
    // 0 - 1 mod 7681 應該是 7680
    a.coeffs[0] = 0;
    b.coeffs[0] = 1;
    poly_sub(&r, &a, &b);

    if (r.coeffs[0] == 7680) {
        print_pass("Negative wrap-around (0 - 1 = q - 1) OK");
    } else {
        print_failure("Negative wrap-around failed!");
        printf("   Expected: 7680 (0x1E00)\n");
        printf("   Actual:   %d (0x%04X)\n", r.coeffs[0], (uint16_t)r.coeffs[0]);
        print_info("Hint: If actual is -1 or 65535, your fqsub is not adding q back.");
    }
}

// ==========================================================
// 測試 2: NTT 域與 InvNTT 因子
// ==========================================================
void debug_ntt_domain() {
    printf("\n=== Debug 2: NTT/InvNTT Domain Roundtrip ===\n");
    polyvec v, v_orig;
    
    // 設定一個簡單數值：全為 1
    for(int i=0; i<RUDRAKSH_K; i++)
        for(int j=0; j<RUDRAKSH_N; j++)
            v.vec[i].coeffs[j] = 1;
    
    v_orig = v;

    // 執行 NTT -> InvNTT
    polyvec_ntt(&v);
    polyvec_invntt_tomont(&v);

    // 檢查結果
    // 理論上 InvNTT_tomont 的結果通常會帶有 Montgomery 因子 (R 或 R^-1)
    // 或者如果你的實作包含 map back，應該變回 1。
    
    int16_t result = v.vec[0].coeffs[0];
    printf("   Input: 1 -> NTT -> InvNTT -> Output: %d\n", result);

    if (result == 1) {
        print_pass("InvNTT returns standard domain integer (Excellent!)");
        print_info("You can mix InvNTT output with CBD errors directly.");
    } else {
        print_info("InvNTT output is NOT 1. This means it's in Montgomery domain.");
        printf("   Montgomery Factor appears to be: %d\n", result);
        print_failure("Potential Bug: You cannot add CBD error (integer) to this value directly!");
        print_info("Fix: Multiply CBD error by this factor, or convert InvNTT output to normal int.");
    }
}

// ==========================================================
// 測試 3: 矩陣 A 的確定性 (Determinism)
// ==========================================================
void debug_matrix_consistency() {
    printf("\n=== Debug 3: Matrix A Determinism ===\n");
    
    uint8_t seed[RUDRAKSH_len_K];
    memset(seed, 0xAB, RUDRAKSH_len_K); // 隨便一個種子

    polymat A1, A2;
    
    poly_matrixA_generator(&A1, seed);
    poly_matrixA_generator(&A2, seed);

    if (memcmp(&A1, &A2, sizeof(polymat)) == 0) {
        print_pass("Matrix A generation is deterministic (Good).");
    } else {
        print_failure("Matrix A generation is NOT deterministic!");
        print_info("Hint: Check if your XOF state is being reset properly.");
    }

    // 檢查數值是否看起來正常 (非全零)
    int zeros = 0;
    for(int i=0; i<10; i++) if(A1.matrix[0][0].coeffs[i] == 0) zeros++;
    if (zeros > 8) {
        print_failure("Matrix A seems to be empty (all zeros). Check XOF output.");
    }
}

// ==========================================================
// 測試 4: 序列化與反序列化 (Packing)
// ==========================================================
void debug_serialization() {
    printf("\n=== Debug 4: Serialization (polyvec_tobytes/frombytes) ===\n");
    
    polyvec v_in, v_out;
    // 13 bits (936 bytes for K=9, N=64)
    uint8_t buffer[936]; 

    // 產生測試資料 (邊界值測試)
    // 0, 1, q-1, q/2 ...
    generate_test_polyvec(&v_in, 0); 
    v_in.vec[0].coeffs[0] = 0;
    v_in.vec[0].coeffs[1] = RUDRAKSH_Q - 1;
    v_in.vec[0].coeffs[2] = 4095; // Max 12-bit
    v_in.vec[0].coeffs[3] = 7000; // Large 13-bit

    // Pack
    polyvec_tobytes_13bit(buffer, &v_in);
    
    // Clear output
    memset(&v_out, 0, sizeof(polyvec));

    // Unpack
    polyvec_frombytes_13bit(&v_out, buffer);

    // Compare
    int fail = 0;
    for(int i=0; i<RUDRAKSH_K; i++) {
        for(int j=0; j<RUDRAKSH_N; j++) {
            if (v_in.vec[i].coeffs[j] != v_out.vec[i].coeffs[j]) {
                if(fail == 0) {
                    printf("   Mismatch at vec[%d].coeffs[%d]: In=%d, Out=%d\n", 
                           i, j, v_in.vec[i].coeffs[j], v_out.vec[i].coeffs[j]);
                }
                fail = 1;
            }
        }
    }

    if (!fail) {
        print_pass("Serialization 13-bit roundtrip OK");
    } else {
        print_failure("Serialization failed. Check bit-packing logic.");
    }
}

// ==========================================================
// 測試 5: 壓縮 (Compression) 精確度
// ==========================================================
void debug_compression() {
    printf("\n=== Debug 5: Compression Loss Analysis ===\n");
    
    polyvec u, u_decomp;
    // u 使用 10 bits 壓縮
    uint8_t buf_u[720]; // 9 * 64 * 10 / 8

    generate_test_polyvec(&u, 100);

    // Compress
    polyvec_compress_u(buf_u, &u);
    // Decompress
    polyvec_decompress_u(&u_decomp, buf_u);

    // 檢查誤差
    // 誤差應該 <= (q / 2^10) / 2  大約是 7681 / 1024 / 2 ~= 3.7
    // 我們寬容一點設為 5 或 10
    int max_diff = 0;
    for(int i=0; i<RUDRAKSH_K; i++) {
        for(int j=0; j<RUDRAKSH_N; j++) {
            int diff = abs(u.vec[i].coeffs[j] - u_decomp.vec[i].coeffs[j]);
            if (diff > max_diff) max_diff = diff;
        }
    }

    printf("   Max compression error (u): %d\n", max_diff);

    // 理論值檢查 (依據 Rudraksh 參數 q=7681, d=10)
    // 容許誤差約為 q / 2^(d+1) 
    int allowed = (RUDRAKSH_Q + (1<<10)) >> 10; // 近似值
    if (max_diff <= allowed + 2) { // +2 for rounding variance
        print_pass("Compression error is within expected range.");
    } else {
        print_failure("Compression error is too large! Check compress/decompress logic.");
    }
}

// ==========================================================
// 輔助：產生單一測試多項式
// ==========================================================
void generate_test_poly(poly *p, int16_t offset) {
    for (int i = 0; i < RUDRAKSH_N; i++) {
        // 產生 0 ~ q-1 之間的數值
        p->coeffs[i] = (offset + i * 123) % RUDRAKSH_Q; 
    }
}

// ==========================================================
// 測試 6: Encode/Decode (針對 B=2 優化版)
// ==========================================================
void debug_encode_decode() {
    printf("\n=== Debug 6: Encode/Decode Signal Strength (B=2) ===\n");
    poly m_raw, m_encoded, m_decoded;
    uint8_t msg_in[RUDRAKSH_len_K];
    uint8_t msg_out[RUDRAKSH_len_K];

    // 我們手動構造一個多項式，包含 B=2 的所有可能值: 0, 1, 2, 3
    // 這樣可以測試所有區間的映射是否正確
    memset(&m_raw, 0, sizeof(poly));
    m_raw.coeffs[0] = 0; // 00
    m_raw.coeffs[1] = 1; // 01
    m_raw.coeffs[2] = 2; // 10
    m_raw.coeffs[3] = 3; // 11

    // 2. Encode
    // 預期映射 (Factor = 1920):
    // 0 -> 0
    // 1 -> 1920
    // 2 -> 3840
    // 3 -> 5760
    poly_encode(&m_encoded, &m_raw);

    // 檢查各點映射
    int16_t val0 = m_encoded.coeffs[0];
    int16_t val1 = m_encoded.coeffs[1];
    int16_t val2 = m_encoded.coeffs[2];
    int16_t val3 = m_encoded.coeffs[3];

    printf("   Input 0 maps to: %d (Target: 0)\n", val0);
    printf("   Input 1 maps to: %d (Target: 1920)\n", val1);
    printf("   Input 2 maps to: %d (Target: 3840)\n", val2);
    printf("   Input 3 maps to: %d (Target: 5760)\n", val3);

    // 驗證範圍 (容許誤差 +/- 10)
    int pass_scale = 1;
    if (abs(val1 - 1920) > 10) pass_scale = 0;
    if (abs(val2 - 3840) > 10) pass_scale = 0;
    if (abs(val3 - 5760) > 10) pass_scale = 0;

    if (pass_scale) {
        print_pass("Scaling for B=2 (0, 1920, 3840, 5760) is correct.");
    } else {
        print_failure("Scaling incorrect! Check poly_encode factor.");
    }

    // 3. Decode (無雜訊)
    poly_decode(&m_decoded, &m_encoded);

    // 檢查還原值
    if (m_decoded.coeffs[0] == 0 && m_decoded.coeffs[1] == 1 && 
        m_decoded.coeffs[2] == 2 && m_decoded.coeffs[3] == 3) {
        print_pass("Decode logic correctly recovers 0, 1, 2, 3.");
    } else {
        print_failure("Decode logic failed!");
        printf("   Recovered: %d, %d, %d, %d\n", 
               m_decoded.coeffs[0], m_decoded.coeffs[1], 
               m_decoded.coeffs[2], m_decoded.coeffs[3]);
    }

    // 4. Decode (加一點雜訊測試 Robustness)
    // 模擬誤差 e''，加個 200 (還在容許範圍內)
    m_encoded.coeffs[1] += 200; // 1920 + 200 = 2120 -> 應該還是 1
    m_encoded.coeffs[2] -= 200; // 3840 - 200 = 3640 -> 應該還是 2
    
    poly_decode(&m_decoded, &m_encoded);
    
    if (m_decoded.coeffs[1] == 1 && m_decoded.coeffs[2] == 2) {
        print_pass("Decode is robust against small noise (+/- 200).");
    } else {
        print_failure("Decode failed with noise!");
    }
}

// ==========================================================
// 測試 7: Compress V (3-bit 壓縮誤差測試)
// ==========================================================
void debug_compress_v() {
    printf("\n=== Debug 7: Compress V (3-bit) Accuracy ===\n");
    
    poly v_in, v_out;
    // v 使用 3 bits 壓縮, N=64 -> 24 bytes
    uint8_t buffer[24]; 

    // 產生測試資料
    generate_test_poly(&v_in, 500);

    // 1. 壓縮
    poly_compress_v(buffer, &v_in);

    // 2. 解壓縮
    poly_decompress_v(&v_out, buffer);

    // 3. 計算誤差
    // 理論最大誤差 Bound = (q / 2^3) / 2 = (7681 / 8) / 2 ≈ 480
    int max_diff = 0;
    for(int i=0; i<RUDRAKSH_N; i++) {
        int diff = abs(v_in.coeffs[i] - v_out.coeffs[i]);
        if (diff > max_diff) max_diff = diff;
    }

    printf("   Max compression error (v): %d\n", max_diff);
    printf("   Theoretical max error:     ~480\n");

    // 設定一個合理的容許值 (考慮邊界狀況，設為 485)
    if (max_diff <= 485) {
        print_pass("V compression error is within theoretical bounds.");
    } else {
        print_failure("V compression error is TOO HIGH!");
        printf("   This will cause decryption failures.\n");
        printf("   Check: poly_compress_v (3-bit logic)\n");
    }
}

// 測試：INTT( NTT(a) * NTT(b) ) == a * b mod (x^64 + 1)
void debug_ntt_multiplication() {
    poly a, b, c_ntt, c_ref;
    // 設定 a = 1, b = x (即 coeffs[1] = 1)
    poly_zero(&a); a.coeffs[0] = 1;
    poly_zero(&b); b.coeffs[1] = 1;

    // 1. 使用你的 NTT 進行點乘
    poly_ntt(&a);
    poly_ntt(&b);
    poly_zero(&c_ntt);
    poly_basemul_acc(&c_ntt, &a, &b); // 點乘
    poly_invntt(&c_ntt);

    // 2. 預期結果：a * b = x，所以 coeffs[1] 應為 1，其餘為 0
    if (c_ntt.coeffs[1] != 1) {
        printf("NTT 同態測試失敗！預期 coeffs[1]=1, 得到 %d\n", c_ntt.coeffs[1]);
        // 如果得到的是其他位置，說明 bit-reversal 索引錯了
        // 如果得到的是亂碼，說明蝶形運算結構（DIF/DIT）與 zetas 表順序不對
    }
}

int main() {
    printf("==========================================\n");
    printf("   Rudraksh Debug Suite (Hardware/Math)   \n");
    printf("==========================================\n");

    debug_subtraction();
    debug_ntt_domain();
    debug_matrix_consistency();
    debug_serialization();
    debug_compression();

    debug_encode_decode();
    debug_compress_v();

    printf("\n==========================================\n");
    printf("Analysis Guide:\n");
    printf("1. If Debug 1 fails: Your decryption math is broken (negative numbers).\n");
    printf("2. If Debug 2 output != 1: You have a Domain Mismatch. Add Montgomery factor to Error term.\n");
    printf("3. If Debug 4 fails: Your KeyGen/Encapsulate key transfer is broken.\n");
    printf("4. If Debug 5 error is huge: Your Cipher is #@/~1df$e5\n");

    debug_ntt_multiplication();
}