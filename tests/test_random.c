#include "../src/rudraksh_random.h"
#include "../src/rudraksh_params.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

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
    // 2. 測試 rudraksh_prf
    // ---------------------------------------------------------
    printf("[4] Testing rudraksh_prf (Key[16]+Nonce[2] -> 8 bytes(one times))...\n");
    {
        uint8_t key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                           0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
        uint8_t nonce[2] = {0x00,0x01}; // i = 0, j = 1
        uint8_t out1[16], out2[16];

        RUDRAFKSH_STATE state1;
        uint8_t out1_1[8];
        uint8_t out1_2[8];
        rudraksh_prf_init(&state1, key, &nonce[0],&nonce[1]);
        rudraksh_prf_put(&state1,out1_1);
        rudraksh_prf_put(&state1,out1_2);
        memcpy(out1, out1_1, 8);
        memcpy(out1+8, out1_2, 8);
        print_hex("  PRF Output: ", out1, 16);

        // Determinism check
        RUDRAFKSH_STATE state2;
        uint8_t out2_1[8];
        uint8_t out2_2[8];
        rudraksh_prf_init(&state2, key, &nonce[0],&nonce[1]);
        rudraksh_prf_put(&state2,out2_1);
        rudraksh_prf_put(&state2,out2_2);
        memcpy(out2, out2_1, 8);
        memcpy(out2+8, out2_2, 8);

        if (memcmp(out1, out2, 16) == 0) {
            printf("  >> Determinism Check: PASSED\n");
        } else {
            printf("  >> Determinism Check: FAILED\n");
        }

        // Key Sensitivity Check (Change Key)
        key[0] ^= 0xFF; 
        RUDRAFKSH_STATE state2_2;
        rudraksh_prf_init(&state2_2, key, &nonce[0],&nonce[1]);
        rudraksh_prf_put(&state2_2,out2_1);
        rudraksh_prf_put(&state2_2,out2_2);
        memcpy(out2, out2_1, 8);
        memcpy(out2+8, out2_2, 8);
        if (memcmp(out1, out2, 16) != 0) {
            printf("  >> Key Sensitivity  : PASSED\n");
        } else {
            printf("  >> Key Sensitivity  : FAILED\n");
        }
        key[0] ^= 0xFF; // Restore key

        // Nonce Sensitivity Check (Change Nonce)
        nonce[0] ^= 0xFF;
        RUDRAFKSH_STATE state2_3;
        rudraksh_prf_init(&state2_3, key, &nonce[0],&nonce[1]);
        rudraksh_prf_put(&state2_3,out2_1);
        rudraksh_prf_put(&state2_3,out2_2);
        memcpy(out2, out2_1, 8);
        memcpy(out2+8, out2_2, 8);
        if (memcmp(out1, out2, 16) != 0) {
            printf("  >> Nonce Sensitivity: PASSED\n");
        } else {
            printf("  >> Nonce Sensitivity: FAILED\n");
        }
    }


    #define test_bytes  1024 * 1024 // 測試 1 MB 的隨機數據

    printf("\n==========================================\n");
    printf("=== Random Bytes Test (%d bytes): ===\n",test_bytes);
    printf("==========================================\n\n");

    // 計算 0 / 1 的比例
    int bit_count[2] = {0, 0};
    uint8_t buffer[test_bytes];
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

    printf("\n=== All Tests Finished ===\n");
    return 0;
}