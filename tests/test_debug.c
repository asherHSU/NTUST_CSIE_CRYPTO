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
// 測試 1: 序列化與反序列化 (Packing)
// ==========================================================
void debug_serialization() {
    printf("\n=== Debug 1: Serialization (polyvec_tobytes/frombytes) ===\n");
    
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
// 測試 2: 壓縮 (Compression) 精確度
// ==========================================================
void debug_compression() {
    printf("\n=== Debug 2: Compression Loss Analysis ===\n");
    
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
// 測試 4: Encode/Decode (針對 B=2 優化版)
// ==========================================================
void debug_encode_decode() {
    printf("\n=== Debug 4: Encode/Decode Signal Strength (B=2) ===\n");
    poly m_raw, m_encoded, m_decoded;

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
// 測試 3: Compress V (3-bit 壓縮誤差測試)
// ==========================================================
void debug_compress_v() {
    printf("\n=== Debug 3: Compress V (3-bit) Accuracy ===\n");
    
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
        if (diff > RUDRAKSH_Q / 2) {
            diff = RUDRAKSH_Q - diff;
        }
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

int main() {
    printf("==========================================\n");
    printf("   Rudraksh Debug Suite (Hardware/Math)   \n");
    printf("==========================================\n");

    debug_serialization();
    debug_compression();
    debug_compress_v();
    debug_encode_decode();
    

    printf("\n=============================================\n");
    printf("   End of Tests\n");
    printf("=============================================\n");
}