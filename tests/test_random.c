#include "../src/rudraksh_random.h"
#include "../src/rudraksh_params.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// 輔助函式：列印 Hex 字串
void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s", label);
    for(size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

int main()
{
    printf("==========================================\n");
    printf("=== Rudraksh ASCON Full API Unit Test ====\n");
    printf("==========================================\n\n");


    // ---------------------------------------------------------
    // 1. 測試 rudraksh_hash
    // ---------------------------------------------------------
    printf("[3] Testing rudraksh_hash ...\n");
    {
        const char *msg = "Test Message ";
        size_t msg_len = strlen(msg);
        uint8_t out1[RUDRAKSH_len_K];
        uint8_t out2[RUDRAKSH_len_K];

        rudraksh_hash(out1, (const uint8_t*)msg, msg_len,RUDRAKSH_len_K);
        print_hex("  Hash G Out (first 16b): ", out1, 16);

        // Determinism check
        rudraksh_hash(out2, (const uint8_t*)msg, msg_len,RUDRAKSH_len_K);
        if (memcmp(out1, out2, RUDRAKSH_len_K) == 0) {
            printf("  >> Determinism Check: PASSED\n");
        } else {
            printf("  >> Determinism Check: FAILED\n");
        }

        // Avalanche check
        char msg_mod[50];
        strcpy(msg_mod, msg);
        msg_mod[msg_len-1] ^= 0xFF; // Flip last byte
        rudraksh_hash(out2, (const uint8_t*)msg_mod, msg_len,RUDRAKSH_len_K);

        if (memcmp(out1, out2, RUDRAKSH_len_K) != 0) {
            printf("  >> Avalanche Check  : PASSED\n");
        } else {
            printf("  >> Avalanche Check  : FAILED\n");
        }
    }
    printf("\n");

    // ---------------------------------------------------------
// 2. 測試 Rudraksh PRF (包含 MatrixA 與 CBD 版本)
// ---------------------------------------------------------

// --- [4.1] Testing rudraksh_prf_init_matrixA ---
printf("[4.1] Testing MatrixA PRF (Key[16]+Nonce[i,j] -> 16 bytes)...\n");
{
    uint8_t key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                       0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    uint8_t nonce_i = 0x00;
    uint8_t nonce_j = 0x01;
    uint8_t out1[16], out2[16];

    RUDRAFKSH_STATE state1;
    uint8_t out1_h[8], out1_l[8];
    
    // 初始化並取得 16 bytes 輸出 (兩次 8 bytes)
    rudraksh_prf_init_matrixA(&state1, key, &nonce_i, &nonce_j);
    rudraksh_prf_put(&state1, out1_h);
    rudraksh_prf_put(&state1, out1_l);
    memcpy(out1, out1_h, 8);
    memcpy(out1 + 8, out1_l, 8);
    print_hex("  MatrixA PRF Output: ", out1, 16);

    // 1. 決定性檢查 (Determinism Check)
    RUDRAFKSH_STATE state2;
    uint8_t out2_h[8], out2_l[8];
    rudraksh_prf_init_matrixA(&state2, key, &nonce_i, &nonce_j);
    rudraksh_prf_put(&state2, out2_h);
    rudraksh_prf_put(&state2, out2_l);
    memcpy(out2, out2_h, 8);
    memcpy(out2 + 8, out2_l, 8);

    if (memcmp(out1, out2, 16) == 0) {
        printf("  >> Determinism Check: PASSED\n");
    } else {
        printf("  >> Determinism Check: FAILED\n");
    }

    // 2. 金鑰靈敏度 (Key Sensitivity)
    key[0] ^= 0xFF; 
    rudraksh_prf_init_matrixA(&state2, key, &nonce_i, &nonce_j);
    rudraksh_prf_put(&state2, out2_h);
    rudraksh_prf_put(&state2, out2_l);
    memcpy(out2, out2_h, 8);
    memcpy(out2 + 8, out2_l, 8);
    if (memcmp(out1, out2, 16) != 0) {
        printf("  >> Key Sensitivity  : PASSED\n");
    } else {
        printf("  >> Key Sensitivity  : FAILED\n");
    }
    key[0] ^= 0xFF; // 還原

    // 3. Nonce 靈敏度 (Nonce Sensitivity - i)
    uint8_t nonce_i_alt = nonce_i ^ 0xFF;
    rudraksh_prf_init_matrixA(&state2, key, &nonce_i_alt, &nonce_j);
    rudraksh_prf_put(&state2, out2_h);
    rudraksh_prf_put(&state2, out2_l);
    memcpy(out2, out2_h, 8);
    memcpy(out2 + 8, out2_l, 8);
    if (memcmp(out1, out2, 16) != 0) {
        printf("  >> Nonce-i Sensitivity: PASSED\n");
    } else {
        printf("  >> Nonce-i Sensitivity: FAILED\n");
    }
}

printf("\n");

// --- [4.2] Testing rudraksh_prf_init_cbd ---
printf("[4.2] Testing CBD PRF (Key[16]+Nonce[1] -> 16 bytes)...\n");
{
    uint8_t key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                       0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    uint8_t nonce = 0x05;
    uint8_t out1[16], out2[16];

    RUDRAFKSH_STATE state1;
    uint8_t out1_h[8], out1_l[8];
    
    // 初始化
    rudraksh_prf_init_cbd(&state1, key, &nonce);
    rudraksh_prf_put(&state1, out1_h);
    rudraksh_prf_put(&state1, out1_l);
    memcpy(out1, out1_h, 8);
    memcpy(out1 + 8, out1_l, 8);
    print_hex("  CBD PRF Output    : ", out1, 16);

    // 1. 決定性檢查
    RUDRAFKSH_STATE state2;
    uint8_t out2_h[8], out2_l[8];
    rudraksh_prf_init_cbd(&state2, key, &nonce);
    rudraksh_prf_put(&state2, out2_h);
    rudraksh_prf_put(&state2, out2_l);
    memcpy(out2, out2_h, 8);
    memcpy(out2 + 8, out2_l, 8);

    if (memcmp(out1, out2, 16) == 0) {
        printf("  >> Determinism Check: PASSED\n");
    } else {
        printf("  >> Determinism Check: FAILED\n");
    }

    // 2. Nonce 靈敏度
    uint8_t nonce_alt = nonce ^ 0x01;
    rudraksh_prf_init_cbd(&state2, key, &nonce_alt);
    rudraksh_prf_put(&state2, out2_h);
    rudraksh_prf_put(&state2, out2_l);
    memcpy(out2, out2_h, 8);
    memcpy(out2 + 8, out2_l, 8);
    if (memcmp(out1, out2, 16) != 0) {
        printf("  >> Nonce Sensitivity : PASSED\n");
    } else {
        printf("  >> Nonce Sensitivity : FAILED\n");
    }
}

// ---------------------------------------------------------
// 3. Random Bytes Test
// ---------------------------------------------------------
#define test_bytes  1024 * 1024 // 測試 1 MB 的隨機數據

printf("\n==========================================\n");
printf("=== Random Bytes Test (%d bytes): ===\n", test_bytes);
printf("==========================================\n\n");

int bit_count[2] = {0, 0};
uint8_t *buffer = malloc(test_bytes); // 使用 malloc 避免 Stack Overflow
if (buffer == NULL) {
    printf("Memory Allocation Failed!\n");
    return -1;
}

rudraksh_randombytes(buffer, test_bytes);
for (int i = 0; i < test_bytes; i++) {
    for (int b = 0; b < 8; b++) {
        int bit = (buffer[i] >> b) & 0x01;
        bit_count[bit]++;
    }
}
printf("  Total Bits: %d\n", test_bytes * 8);
printf("  0 Bits    : %d (%.2f%%)\n", bit_count[0], (bit_count[0] * 100.0) / (test_bytes * 8));
printf("  1 Bits    : %d (%.2f%%)\n", bit_count[1], (bit_count[1] * 100.0) / (test_bytes * 8));

free(buffer);
printf("\n=============================================\n");
printf("   End of Tests\n");
printf("=============================================\n");
}