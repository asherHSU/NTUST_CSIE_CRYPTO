#ifndef RUDRAKSH_CRYPTO_H
#define RUDRAKSH_CRYPTO_H

#include <stdint.h> // for uint8_t, uint16_t, etc.
#include <stddef.h> // for size_t

// ==========================================================
// 1. Public Key Encryption (PKE) APIs
// ==========================================================
// PKE KeyGen
void rudraksh_pke_keygen(uint8_t *public_key, uint8_t *secret_key);
// PKE Encryption
void rudraksh_pke_encrypt(uint8_t *ciphertext, const uint8_t *message, const uint8_t *public_key);
// PKE Decryption
void rudraksh_pke_decrypt(uint8_t *message, const uint8_t *ciphertext, const uint8_t *secret_key);

// ==========================================================
// 2. Key Encapsulation Mechanism (KEM) APIs
// ==========================================================
// KEM KeyGen
void rudraksh_kem_keygen(uint8_t *public_key, uint8_t *secret_key);
// KEM Encapsulation
void rudraksh_kem_encapsulate(uint8_t *ciphertext, uint8_t *shared_secret, const uint8_t *public_key);
// KEM Decapsulation
void rudraksh_kem_decapsulate(uint8_t *shared_secret, const uint8_t *ciphertext, const uint8_t *secret_key);    


// ==========================================================
// 3. Digital Signature APIs
// ==========================================================
// Signature KeyGen
void rudraksh_sig_keygen(uint8_t *public_key, uint8_t *secret_key);
// Signature Generation
void rudraksh_sig_sign(uint8_t *signature, const uint8_t *message, size_t message_len, const uint8_t *secret_key);
// Signature Verification
int rudraksh_sig_verify(const uint8_t *signature, const uint8_t *message, size_t message_len, const uint8_t *public_key);


#endif