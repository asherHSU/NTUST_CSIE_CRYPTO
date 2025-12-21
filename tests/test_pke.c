#include <stdint.h>
#include <stddef.h>
#include <string.h> // for memcpy, memcmp
#include <stdio.h>

// 包含相關標頭檔
# include "../src/rudraksh_params.h"
# include "../src/rudraksh_crypto.h"
# include "../src/rudraksh_math.h"
# include "../src/rudraksh_random.h"
// 假設已包含必要的 params, math, random headers

// ==========================================================
// 輔助函式：Constant-time Operations
// ==========================================================
/*
// 比較兩個 byte array (Constant-time)
// 回傳值：0 代表相等，1 代表不相等
static int verify(const uint8_t *a, const uint8_t *b, size_t len) {
    uint8_t r = 0;
    for (size_t i = 0; i < len; i++) {
        r |= a[i] ^ b[i];
    }
    // 若 r 為 0 (相等)，回傳 0
    // 若 r != 0 (不相等)，回傳 1
    // 下面的運算確保將任意非零值正規化為 1 (假設 int 為 32-bit 或以上)
    return (-(int)r >> 31) & 1; 
    // 簡單版 (依編譯器優化可能不是 constant-time，但在功能驗證上是正確的):
    // return r != 0;
}

// 條件複製 (Constant-time Conditional Move)
// 若 b == 1，則將 x 複製到 r
// 若 b == 0，則 r 保持不變
// 注意：b 必須嚴格為 0 或 1
static void cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t b) {
    // 產生 mask: 
    // 若 b = 1, mask = 0xFF (-1)
    // 若 b = 0, mask = 0x00 (0)
    uint8_t mask = -b; 
    for (size_t i = 0; i < len; i++) {
        r[i] ^= mask & (x[i] ^ r[i]);
    }
}
*/

// ==========================================================
// 1. Public Key Encryption (PKE) APIs
//    核心運算層：處理 Internal Struct <-> Math
// ==========================================================

// PKE KeyGen: 生成內部使用的結構 (Unpacked)
void rudraksh_pke_test()
{
    poly m_original;   // 原始訊息 (位元值 0, 1, 2, 3)
    poly m_encoded;    // 編碼後的多項式 (用於加密輸入)
    poly m_recovered;  // 解密並 Decode 後的結果
    int errors = 0;

    printf("--- Rudraksh PKE Decoding Success Test ---\n");
    
    // 1. 初始化變數
    uint8_t seedbuf[2 * RUDRAKSH_len_K];
    const uint8_t *seed_A = seedbuf;
    const uint8_t *seed_se = seedbuf + RUDRAKSH_len_K;
    
    polymat A;
    polyvec s, e;
    polyvec b;

    polyvec_zero(&s); 
    polyvec_zero(&e); 
    polyvec_zero(&b); 



    // 2. 亂數生成
    rudraksh_randombytes(seedbuf, 2 * RUDRAKSH_len_K);

    // 保存 seed_A 到內部 PK 結構
    // memcpy(pk->seed_A, seed_A, RUDRAKSH_len_K);

    // 生成矩陣 A 和向量 s, e
    poly_matrixA_generator(&A, seed_A);
    polyvec_cbd_eta(&s, &e, seed_se);

    // polyvec_zero(&e);
    // polyvec_zero(&s);

    // 3. 矩陣運算 (NTT Domain)
    // polyvec_ntt(&s);
    // polyvec_ntt(&e);

    // 計算 b = A * s + e 
    // 先計算 A * s 存入 pk->b
    poly_matrix_vec_mul(&b, &A, &s);
    
    // 再加上 e (In-place addition: b = b + e)
    polyvec_add(&b, &b, &e);

    // 填入 SK
    // sk->s = s;

    // polymat A;
    polyvec s_prime, e_prime, b_prime, u;
    poly e_prime_prime, v;  

    polyvec_zero(&s_prime);
    polyvec_zero(&e_prime);
    polyvec_zero(&b_prime); 
    polyvec_zero(&u); 
    poly_zero(&e_prime_prime);
    poly_zero(&v);

    // 1. 重建矩陣 A (使用內部 PK 存的 seed)
    // poly_matrixA_generator(&A, pk->seed_A);

    // 2. 取樣 (使用隨機數 r)
    uint8_t r[RUDRAKSH_len_K];
    rudraksh_randombytes(r,RUDRAKSH_len_K);
    polyvec_cbd_eta(&s_prime, &e_prime, r);
    poly_cbd_eta(&e_prime_prime, r, 2 * RUDRAKSH_K); // Nonce offset

    // polyvec_zero(&e_prime);
    // polyvec_zero(&s_prime);

    // 3. NTT 運算
    // polyvec_ntt(&s_prime);

    // 4. 計算 u (即 b_prime) = A^T * s' + e'
    poly_matrix_trans_vec_mul(&b_prime, &A, &s_prime);  // b' = A^T * s'
    // polyvec_invntt_tomont(&b_prime);                    // intt(b')
    polyvec_add(&u, &b_prime, &e_prime);                // 加誤差 e'    

    // 5. 計算 v (即 c_m_hat) = b^T * s' + e'' + Encode(m)
    poly_vector_vector_mul(&v, &b, &s_prime);           // cm(v) = b^T * s'
    // poly_invntt(&v);                             // cm = intt(cm)

    for (int i = 0; i < RUDRAKSH_N; i++) 
    {
        // 每個係數循環代表訊息 0, 1, 2, 3
        m_original.coeffs[i] = (i % 4);
    }
    poly_encode(&m_encoded, &m_original);

    // v = v + e''
    poly_add(&v, &v, &e_prime_prime);

    // v = v + Encode(m)
    poly_add(&v, &v, &m_encoded);


    uint8_t u_bytes[720];
    uint8_t v_bytes[40];
    // 6. 壓縮並寫入 External Ciphertext Bytes
    // u 的部分 (K * N * 10 bits) -> bytes
    polyvec_compress_u(u_bytes, &u);

    // v 的部分 (N * 3 bits) -> bytes (接在 u 後面)
    poly_compress_v(v_bytes,&v);

    polyvec u_prime;
    poly v_prime, v_temp;

    // u_prime = u;
    // v_prime = v;

    // 1. 解壓縮 (Unpack Bytes -> Poly)
    // 從 c->bytes 讀取 u
    polyvec_decompress_u(&u_prime, u_bytes);
    // 從 c->bytes 偏移處讀取 v
    poly_decompress_v(&v_prime, v_bytes); //!

    // 2. 運算
    // polyvec_ntt(&u_prime);
    poly_vector_vector_mul(&v_temp, &u_prime, &s); // u^T *
    // poly_invntt(&v_temp);

    // m'' = v - s^T * u
    poly_sub(&v_temp,&v_prime,&v_temp);

    for (int i = 0; i < RUDRAKSH_N; i++) {
        uint16_t orig = (uint16_t)v_temp.coeffs[i];
        if (i < 8) {
            printf("%5d | %8d | %9d | %9d \n", i, orig,1920*(i%4),(1920*(i%4))-orig);
        }
    }

    // 3. Decode
     poly_decode(&m_recovered, &v_temp);

    //check
    // --- 以下為生成的比較部分 ---

    printf("\n[Comparison Result]\n");
    printf("Index | Original | Recovered | Status\n");
    printf("-------------------------------------\n");

    for (int i = 0; i < RUDRAKSH_N; i++) {
        uint16_t orig = (uint16_t)m_original.coeffs[i];
        uint16_t reco = (uint16_t)m_recovered.coeffs[i];
        
        char *status = (orig == reco) ? "OK" : "FAIL";
        if (orig != reco) errors++;

        // 僅列印前 8 個係數作為範例，避免洗版
        if (i < 8) {
            printf("%5d | %8d | %9d | %s\n", i, orig, reco, status);
        }
    }
}

/*
// ==========================================================
// 2. Key Encapsulation Mechanism (KEM) APIs
//    介面層：處理 Pack/Unpack <-> Internal Structs
// ==========================================================

// KEM KeyGen: 輸出序列化的 Bytes
void rudraksh_kem_keygen(public_key_bitstream *pkb, secret_key_bitstream *skb)
{
    // [Internal] 宣告內部運算結構
    public_key pk;
    secret_key sk;

    // 1. 執行核心 KeyGen
    rudraksh_pke_keygen(&pk, &sk);

    // 2. 序列化 Public Key (Pack -> pkb->bytes)
    // b 向量 (13-bit packed)
    polyvec_tobytes_13bit(pkb->bytes, &pk.b);
    // seed_A (直接複製)
    memcpy(pkb->bytes + (CRYPTO_PUBLICKEYBYTES - RUDRAKSH_len_K), pk.seed_A, RUDRAKSH_len_K);

    // 3. 準備 Secret Key 的額外資料
    uint8_t pkh[RUDRAKSH_len_K];
    uint8_t z[RUDRAKSH_len_K];
    
    // 計算 H(pk)
    rudraksh_hash(pkh, pkb->bytes, CRYPTO_PUBLICKEYBYTES);
    // 生成隨機數 z
    rudraksh_randombytes(z, RUDRAKSH_len_K);

    // 4. 序列化 Secret Key (Pack -> skb->bytes)
    // 格式: s || pk || pkh || z
    size_t offset = 0;

    // Pack s
    polyvec_tobytes_13bit(skb->bytes + offset, &sk.s);
    offset += (CRYPTO_SECRETKEYBYTES - CRYPTO_PUBLICKEYBYTES - 2*RUDRAKSH_len_K);

    // Copy pk bytes
    memcpy(skb->bytes + offset, pkb->bytes, CRYPTO_PUBLICKEYBYTES);
    offset += CRYPTO_PUBLICKEYBYTES;

    // Copy pkh
    memcpy(skb->bytes + offset, pkh, RUDRAKSH_len_K);
    offset += RUDRAKSH_len_K;

    // Copy z
    memcpy(skb->bytes + offset, z, RUDRAKSH_len_K);
}

// KEM Encapsulation: 輸入 PK Bytes, 輸出 CT Bytes 和 Shared Secret Bytes
void rudraksh_kem_encapsulate(public_key_bitstream *pkb, cipher_text *c, shared_secret *K)
{
    // [Internal] 宣告內部結構
    public_key pk;
    poly m;
    
    uint8_t msg[RUDRAKSH_len_K];
    uint8_t kr[2 * RUDRAKSH_len_K];
    uint8_t pkh[RUDRAKSH_len_K];

    // 1. 反序列化 Public Key (Unpack -> Internal PK)
    polyvec_frombytes_13bit(&pk.b, pkb->bytes);
    memcpy(pk.seed_A, pkb->bytes + (CRYPTO_PUBLICKEYBYTES - RUDRAKSH_len_K), RUDRAKSH_len_K);

    // 2. 生成隨機訊息 msg
    rudraksh_randombytes(msg, RUDRAKSH_len_K);
    arrange_msg(&m, msg); // 轉換為 poly

    // 3. 計算 pkh
    rudraksh_hash(pkh, pkb->bytes, CRYPTO_PUBLICKEYBYTES);

    // 4. 生成 (K, r)
    // buffer = pkh || msg
    uint8_t buf[2 * RUDRAKSH_len_K];
    memcpy(buf, pkh, RUDRAKSH_len_K);
    memcpy(buf + RUDRAKSH_len_K, msg, RUDRAKSH_len_K);
    rudraksh_hash(kr, buf, 2 * RUDRAKSH_len_K);

    // 5. 輸出 Shared Secret K
    memcpy(K->bytes, kr, RUDRAKSH_len_K);

    // 6. 加密 (PKE Encrypt 會自動處理成 c->bytes)
    // 使用 kr 的後半段作為隨機數 r
    rudraksh_pke_encrypt(&pk, &m, kr + RUDRAKSH_len_K, c);
}

// KEM Decapsulation: 輸入 SK Bytes, CT Bytes, 輸出 Shared Secret Bytes
void rudraksh_kem_decapsulate(secret_key_bitstream *skb, cipher_text *c, shared_secret *K)
{
    // [Internal] 宣告內部結構
    secret_key sk;
    public_key pk;
    poly m_prime;
    
    // 輔助變數
    uint8_t pkh[RUDRAKSH_len_K];
    uint8_t z[RUDRAKSH_len_K];
    uint8_t msg_prime[RUDRAKSH_len_K];
    uint8_t kr_prime[2 * RUDRAKSH_len_K];
    uint8_t k_fail[RUDRAKSH_len_K];
    cipher_text c_star;

    // 1. 反序列化 Secret Key (Unpack -> Internal SK & PK)
    size_t offset = 0;
    
    // Unpack s
    polyvec_frombytes_13bit(&sk.s, skb->bytes);
    offset += (CRYPTO_SECRETKEYBYTES - CRYPTO_PUBLICKEYBYTES - 2*RUDRAKSH_len_K);

    // Unpack pk (從 SK 中還原，用於再加密驗證)
    const uint8_t *pk_bytes_ptr = skb->bytes + offset;
    polyvec_frombytes_13bit(&pk.b, pk_bytes_ptr);
    memcpy(pk.seed_A, pk_bytes_ptr + (CRYPTO_PUBLICKEYBYTES - RUDRAKSH_len_K), RUDRAKSH_len_K);
    offset += CRYPTO_PUBLICKEYBYTES;

    // Copy pkh & z
    memcpy(pkh, skb->bytes + offset, RUDRAKSH_len_K);
    offset += RUDRAKSH_len_K;
    memcpy(z, skb->bytes + offset, RUDRAKSH_len_K);

    // 2. 解密 (得到 m')
    rudraksh_pke_decrypt(c, &sk, &m_prime);
    original_msg(msg_prime, &m_prime); // Poly -> Bytes

    // 3. 重新計算 (K', r')
    uint8_t buf[2 * RUDRAKSH_len_K];
    memcpy(buf, pkh, RUDRAKSH_len_K);
    memcpy(buf + RUDRAKSH_len_K, msg_prime, RUDRAKSH_len_K);
    rudraksh_hash(kr_prime, buf, 2 * RUDRAKSH_len_K);

    // 4. 重新加密 (得到 c*)
    poly m_prime_poly;
    arrange_msg(&m_prime_poly, msg_prime);
    rudraksh_pke_encrypt(&pk, &m_prime_poly, kr_prime + RUDRAKSH_len_K, &c_star);

    // 5. 計算失敗時的 Key (K'') = H(c || z)
    uint8_t fail_input[CRYPTO_CIPHERTEXTBYTES + RUDRAKSH_len_K];
    memcpy(fail_input, c->bytes, CRYPTO_CIPHERTEXTBYTES);
    memcpy(fail_input + CRYPTO_CIPHERTEXTBYTES, z, RUDRAKSH_len_K);
    rudraksh_hash(k_fail, fail_input, sizeof(fail_input));

    // 6. 驗證 c == c* (Constant Time)
    // 假設 verify 返回 0 代表相等 (成功)，-1 代表不等 (失敗)
    int fail = verify(c->bytes, c_star.bytes, CRYPTO_CIPHERTEXTBYTES);

    // 7. 選擇輸出 Key (Constant Time Select)
    // 如果 fail=0 (成功)，複製 kr_prime (K')
    // 如果 fail=-1 (失敗)，複製 k_fail (K'')
    cmov(K->bytes, kr_prime, RUDRAKSH_len_K, (uint8_t)!fail); // 若 !fail 為 1，則搬移 kr_prime
    cmov(K->bytes, k_fail, RUDRAKSH_len_K, (uint8_t)fail);    // 若 fail 為 -1 (0xFF)，則搬移 k_fail
    
    // 注意：cmov 的實作需確保正確覆蓋。
    // 簡單邏輯： output = (K' & ~mask) | (K'' & mask)
}

*/

int main()
{
    rudraksh_pke_test();
}