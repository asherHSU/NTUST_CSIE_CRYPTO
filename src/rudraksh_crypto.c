# include "rudraksh_crypto.h" //實作

# include "rudraksh_params.h"
# include "rudraksh_math.h" 
# include "rudraksh_random.h" // hash & randombytes

// ==========================================================
// 1. Public Key Encryption (PKE) 
// ==========================================================
// PKE KeyGen
void rudraksh_pke_keygen(public_key *pk, secret_key *sk)
{
    uint8_t seed_A[RUDRAKSH_len_K];
    uint8_t seed_se[RUDRAKSH_len_K];
    polymat A;
    
}
// PKE Encryption
void rudraksh_pke_encrypt(public_key *pk, poly *m, uint8_t *r, cipher_text *c)
{

}
// PKE Decryption
void rudraksh_pke_decrypt(cipher_text *c, secret_key *sk, poly *m)
{

}

// ==========================================================
// 2. Key Encapsulation Mechanism (KEM) 
// ==========================================================
// KEM KeyGen
void rudraksh_kem_keygen(public_key_bitstream *pkb, secret_key_bitstream *skb)
{

}
// KEM Encapsulation
void rudraksh_kem_encapsulate(public_key_bitstream *pkb, cipher_text *c, shared_secret *K)
{

}
// KEM Decapsulation
void rudraksh_kem_decapsulate(secret_key_bitstream *skb, cipher_text *c, shared_secret *K)
{

}
