#include <stdint.h> // for uint8_t, uint16_t, etc.
#include <stddef.h> // for size_t
#include "rudraksh_crypto.h"
#include "rudraksh_params.h"
#include "rudraksh_math.h"
#include "rudraksh_random.h"

// ==========================================================
// 1. Public Key Encryption (PKE) APIs
// ==========================================================
// PKE KeyGen
void rudraksh_pke_keygen(uint8_t *public_key, uint8_t *secret_key)
{

    // ==========================================
    // 1. 初始化變數
    // ==========================================
    // (1)種子與隨機數 (Seeds)
    //      根據論文，我們需要兩個 32 bytes (256-bit) 的種子：
    //      1. seed_A : 用於生成公共矩陣 A
    //      2. seed_se: 用於生成秘密 s 和誤差 e
    //      通常我們會呼叫一次 RNG 產生 64 bytes，然後切成兩半
    
    uint8_t seedbuf[2 * RUDRAKSH_len_K]; 
    const uint8_t *publicseed = seedbuf;                       // 指向 seed_A
    const uint8_t *noiseseed = seedbuf + RUDRAKSH_len_K;    // 指向 seed_se

    // (2) 多項式結構 (Lattice Structures)
    //      這些變數佔用 Stack 空間較大，若在嵌入式系統需注意 Stack Overflow
    //      K = 9, N = 64 (for KEM-poly64)
    
    polymat A;   // 公共矩陣 A (K x K 個多項式) -> 對應論文的 Â
    polyvec s;   // 秘密向量 s (K 個多項式)     -> 對應論文的 ŝ
    polyvec e;   // 誤差向量 e (K 個多項式)     -> 對應論文的 ê
    polyvec b;   // 公鑰向量 b (K 個多項式)     -> 對應論文的 b = As + e

    // ==========================================
    // 2. 亂數生成 -> (seed, A, s, e)
    // ==========================================
    // step 1 : 生成 seed 
    rudraksh_randombytes(seedbuf,2 * RUDRAKSH_len_K);
    // step 2 : 生成 A
    
    // step 3 : 生成 s, e

    // ==========================================
    // 3. 矩陣計算 
    // ==========================================
    // step 4 : s, e to ntt
    // step 5 : b = As + e

    // ==========================================
    // 4. 公私鑰轉換 poly -> bit stream
    // ==========================================
}
// PKE Encryption
void rudraksh_pke_encrypt(uint8_t *ciphertext, const uint8_t *message, const uint8_t *public_key)
{

}
// PKE Decryption
void rudraksh_pke_decrypt(uint8_t *message, const uint8_t *ciphertext, const uint8_t *secret_key)
{

}

// ==========================================================
// 2. Key Encapsulation Mechanism (KEM) APIs
// ==========================================================
// KEM KeyGen
void rudraksh_kem_keygen(uint8_t *public_key, uint8_t *secret_key);
// KEM Encapsulation
void rudraksh_kem_encapsulate(uint8_t *ciphertext, uint8_t *shared_secret, const uint8_t *public_key);
// KEM Decapsulation
void rudraksh_kem_decapsulate(uint8_t *shared_secret, const uint8_t *ciphertext, const uint8_t *secret_key);    
