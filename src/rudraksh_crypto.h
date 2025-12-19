#ifndef RUDRAKSH_CRYPTO_H
#define RUDRAKSH_CRYPTO_H

#include <stdint.h> // for uint8_t, uint16_t, etc.
#include <stddef.h> // for size_t
#include "rudraksh_params.h"
#include "rudraksh_math.h" // for poly ,polyvec

// ==========================================================
// Data Type
// ==========================================================
// ==========================================
// 1. External / Serialized (API 用，緊湊的 Bytes)
// ==========================================
typedef struct {
    uint8_t bytes[CRYPTO_PUBLICKEYBYTES];
} public_key_bitstream;

typedef struct {
    uint8_t bytes[CRYPTO_SECRETKEYBYTES];
} secret_key_bitstream;

typedef struct {
    uint8_t bytes[CRYPTO_CIPHERTEXTBYTES];
} cipher_text;

typedef struct {
    uint8_t bytes[RUDRAKSH_len_K];
} shared_secret;

// ==========================================
// 2. Internal / Unpacked (運算用，int16_t)
// ==========================================
typedef struct {
    polyvec b;                         // 係數為 int16_t
    uint8_t seed_A[RUDRAKSH_len_K];    // 儲存生成矩陣用的種子
} public_key;

typedef struct {
    polyvec s;                         // 係數為 int16_t
} secret_key;

// ==========================================================
// 1. Public Key Encryption (PKE) 
// ==========================================================
// PKE KeyGen
void rudraksh_pke_keygen(public_key *pk, secret_key *sk);
// PKE Encryption
void rudraksh_pke_encrypt(public_key *pk, poly *m, uint8_t *r, cipher_text *c);
// PKE Decryption
void rudraksh_pke_decrypt(cipher_text *c, secret_key *sk, poly *m);

// ==========================================================
// 2. Key Encapsulation Mechanism (KEM) 
// ==========================================================
// KEM KeyGen
void rudraksh_kem_keygen(public_key_bitstream *pkb, secret_key_bitstream *skb);
// KEM Encapsulation
void rudraksh_kem_encapsulate(public_key_bitstream *pkb, cipher_text *c, shared_secret *K);
// KEM Decapsulation
void rudraksh_kem_decapsulate(secret_key_bitstream *skb, cipher_text *c, shared_secret *K);    



// // ==========================================================
// // 3. Digital Signature APIs
// // ==========================================================
// // Signature KeyGen
// void rudraksh_sig_keygen(uint8_t *public_key, uint8_t *secret_key);
// // Signature Generation
// void rudraksh_sig_sign(uint8_t *signature, const uint8_t *message, size_t message_len, const uint8_t *secret_key);
// // Signature Verification
// int rudraksh_sig_verify(const uint8_t *signature, const uint8_t *message, size_t message_len, const uint8_t *public_key);


#endif