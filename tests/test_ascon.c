#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../src/rudraksh_ascon.h"

// 輔助函式：列印 Hex 字串
void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s", label);
    for(size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

int main() {
    printf("==========================================\n");
    printf("=== Rudraksh ASCON Full API Unit Test ====\n");
    printf("==========================================\n\n");

    // ---------------------------------------------------------
    // 1. 測試 rudraksh_xof
    // ---------------------------------------------------------
    printf("[1] Testing rudraksh_xof (32 bytes -> Any bytes)...\n");
    {
        uint8_t seed[32] = {0}; // 全 0 種子
        uint8_t out1[32], out2[32];

        rudraksh_xof(out1, 32, seed);
        print_hex("  Output 1: ", out1, 16); // 只印前16 bytes示意

        // Determinism check
        rudraksh_xof(out2, 32, seed);
        if (memcmp(out1, out2, 32) == 0) {
            printf("  >> Determinism Check: PASSED\n");
        } else {
            printf("  >> Determinism Check: FAILED\n");
        }

        // Avalanche check (Seed + 1)
        seed[0] ^= 0x01;
        rudraksh_xof(out2, 32, seed);
        if (memcmp(out1, out2, 32) != 0) {
            printf("  >> Avalanche Check  : PASSED\n");
        } else {
            printf("  >> Avalanche Check  : FAILED (Output didn't change)\n");
        }
    }
    printf("\n");

    // ---------------------------------------------------------
    // 2. 測試 rudraksh_hash_H
    // ---------------------------------------------------------
    printf("[2] Testing rudraksh_hash_H (Any bytes -> 32 bytes)...\n");
    {
        const char *msg = "Hello Rudraksh";
        size_t msg_len = strlen(msg);
        uint8_t out1[RUDRAKSH_HASH_H_OUT_BYTES];
        uint8_t out2[RUDRAKSH_HASH_H_OUT_BYTES];

        rudraksh_hash_H(out1, (const uint8_t*)msg, msg_len);
        print_hex("  Hash H Out: ", out1, RUDRAKSH_HASH_H_OUT_BYTES);

        // Determinism check
        rudraksh_hash_H(out2, (const uint8_t*)msg, msg_len);
        if (memcmp(out1, out2, RUDRAKSH_HASH_H_OUT_BYTES) == 0) {
            printf("  >> Determinism Check: PASSED\n");
        } else {
            printf("  >> Determinism Check: FAILED\n");
        }

        // Avalanche check (Change input message slightly)
        char msg_mod[50];
        strcpy(msg_mod, msg);
        msg_mod[0] = 'h'; // 'H' -> 'h'
        rudraksh_hash_H(out2, (const uint8_t*)msg_mod, msg_len);
        
        if (memcmp(out1, out2, RUDRAKSH_HASH_H_OUT_BYTES) != 0) {
            printf("  >> Avalanche Check  : PASSED\n");
        } else {
            printf("  >> Avalanche Check  : FAILED\n");
        }
    }
    printf("\n");

    // ---------------------------------------------------------
    // 3. 測試 rudraksh_hash_G
    // ---------------------------------------------------------
    printf("[3] Testing rudraksh_hash_G (Any bytes -> 64 bytes)...\n");
    {
        const char *msg = "Test Message G";
        size_t msg_len = strlen(msg);
        uint8_t out1[RUDRAKSH_HASH_G_OUT_BYTES];
        uint8_t out2[RUDRAKSH_HASH_G_OUT_BYTES];

        rudraksh_hash_G(out1, (const uint8_t*)msg, msg_len);
        print_hex("  Hash G Out (first 16b): ", out1, 16);

        // Determinism check
        rudraksh_hash_G(out2, (const uint8_t*)msg, msg_len);
        if (memcmp(out1, out2, RUDRAKSH_HASH_G_OUT_BYTES) == 0) {
            printf("  >> Determinism Check: PASSED\n");
        } else {
            printf("  >> Determinism Check: FAILED\n");
        }

        // Avalanche check
        char msg_mod[50];
        strcpy(msg_mod, msg);
        msg_mod[msg_len-1] ^= 0xFF; // Flip last byte
        rudraksh_hash_G(out2, (const uint8_t*)msg_mod, msg_len);

        if (memcmp(out1, out2, RUDRAKSH_HASH_G_OUT_BYTES) != 0) {
            printf("  >> Avalanche Check  : PASSED\n");
        } else {
            printf("  >> Avalanche Check  : FAILED\n");
        }
    }
    printf("\n");

    // ---------------------------------------------------------
    // 4. 測試 rudraksh_prf
    // ---------------------------------------------------------
    printf("[4] Testing rudraksh_prf (Key[16]+Nonce[1] -> Any bytes)...\n");
    {
        uint8_t key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                           0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
        uint8_t nonce[1] = {0xAA};
        uint8_t out1[32], out2[32];

        rudraksh_prf(out1, 32, key, nonce);
        print_hex("  PRF Output: ", out1, 16);

        // Determinism check
        rudraksh_prf(out2, 32, key, nonce);
        if (memcmp(out1, out2, 32) == 0) {
            printf("  >> Determinism Check: PASSED\n");
        } else {
            printf("  >> Determinism Check: FAILED\n");
        }

        // Key Sensitivity Check (Change Key)
        key[0] ^= 0xFF; 
        rudraksh_prf(out2, 32, key, nonce);
        if (memcmp(out1, out2, 32) != 0) {
            printf("  >> Key Sensitivity  : PASSED\n");
        } else {
            printf("  >> Key Sensitivity  : FAILED\n");
        }
        key[0] ^= 0xFF; // Restore key

        // Nonce Sensitivity Check (Change Nonce)
        nonce[0] ^= 0xFF;
        rudraksh_prf(out2, 32, key, nonce);
        if (memcmp(out1, out2, 32) != 0) {
            printf("  >> Nonce Sensitivity: PASSED\n");
        } else {
            printf("  >> Nonce Sensitivity: FAILED\n");
        }
    }

    printf("\n=== All Tests Finished ===\n");
    return 0;
}