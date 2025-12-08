#include "rudraksh_random.h"

// ==========================================================
// Linux/macOS 系統的亂數產生器實作
// ==========================================================
// #include <stdio.h>  // 為了 fopen, fread, fclose
// #include <stdlib.h> // 為了 exit

// void rudraksh_randombytes(uint8_t *x, size_t xlen) {
//     // 1. 打開系統的亂數裝置
//     FILE *f = fopen("/dev/urandom", "rb");
    
//     if (f == NULL) {
//         fprintf(stderr, "Fatal error: Cannot open /dev/urandom\n");
//         exit(1);
//     }

//     // 2. 讀取 xlen 個 bytes
//     size_t result = fread(x, 1, xlen, f);
    
//     if (result != xlen) {
//         fprintf(stderr, "Fatal error: Failed to read random bytes\n");
//         fclose(f);
//         exit(1);
//     }

//     // 3. 關閉檔案
//     fclose(f);
// }


// ==========================================================
// Windows 系統的亂數產生器實作 (使用 CryptGenRandom)
// ==========================================================
#include <windows.h>
#include <wincrypt.h>

void rudraksh_randombytes(uint8_t *x, size_t xlen) {
    HCRYPTPROV hCryptProv;
    
    // 取得加密 context
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        exit(1);
    }
    
    // 生成亂數
    if (!CryptGenRandom(hCryptProv, (DWORD)xlen, x)) {
        exit(1);
    }
    
    CryptReleaseContext(hCryptProv, 0);
}