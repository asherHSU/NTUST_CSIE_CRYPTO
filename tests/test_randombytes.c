#include "../src/rudraksh_random.h"
#include "../src/rudraksh_params.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main()
{
    // 計算 0 / 1 的比例
    int bit_count[2] = {0, 0};
    #define test_bytes  1024 * 1024 // 測試 1 MB 的隨機數據
    uint8_t buffer[test_bytes];
    rudraksh_randombytes(buffer, test_bytes);
    for (int i = 0; i < test_bytes; i++) {
        for (int b = 0; b < 8; b++) {
            int bit = (buffer[i] >> b) & 0x01;
            bit_count[bit]++;
        }
    }
    printf("Random Bytes Test (%d bytes):\n", test_bytes);
    printf("  Total Bits: %d\n", test_bytes * 8);
    printf("  0 Bits    : %d (%.2f%%)\n", bit_count[0], (bit_count[0] * 100.0) / (test_bytes * 8));
    printf("  1 Bits    : %d (%.2f%%)\n", bit_count[1], (bit_count[1] * 100.0) / (test_bytes * 8));
    return 0;
}