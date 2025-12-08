#ifndef RUDRAKSH_PARAMS_H
#define RUDRAKSH_PARAMS_H

/**
 * Rudraksh KEM-poly64 Parameters
 * Source: Table 1, Rudraksh: A compact and lightweight post-quantum KEM 
 */

// 1. 多項式環 (Polynomial Ring) 參數
// 論文選用 n=64 以降低記憶體與硬體面積 [cite: 259, 425]
#define KYBER_N 64

// 2. 模數 (Modulus)
// 使用質數 q = 7681，這是一個 NTT 友善質數 (7681 = 120 * 64 + 1) 
#define KYBER_Q 7681

// 3. 模組晶格 (Module Lattice) 維度
// 為了在 n 縮小後維持安全性，矩陣維度 l 提升至 9 
#define KYBER_K 9  // 在 Kyber 代碼中通常用 K 表示矩陣維度 l

// 4. 誤差分佈 (Error Distribution)
// Centered Binomial Distribution 參數 eta = 2 
#define KYBER_ETA 2

// 5. 壓縮參數 (Compression Parameters)
// 用於 ciphertext 的壓縮 
#define KYBER_POLYCOMPRESSEDBYTES 320 // 需根據 q 和壓縮位元數重新計算 (待確認)
#define KYBER_POLYVECCOMPRESSEDBYTES 768 // (待確認)

// 6. 金鑰與密文大小 (Bytes)
// 根據論文 Section 3.6 文字描述 [cite: 491]
#define CRYPTO_PUBLICKEYBYTES  952
#define CRYPTO_SECRETKEYBYTES  1920
#define CRYPTO_CIPHERTEXTBYTES 760

// len_K 定義為 16 bytes (128 bits) 的共享金鑰長度
#define len_K 16

#endif // RUDRAKSH_PARAMS_H