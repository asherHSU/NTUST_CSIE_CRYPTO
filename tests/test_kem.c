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


// ==========================================================
// 2. Key Encapsulation Mechanism (KEM) APIs
//    介面層：處理 Pack/Unpack <-> Internal Structs
// ==========================================================

// KEM KeyGen: 輸出序列化的 Bytes
void rudraksh_kem_test() //public_key_bitstream *pkb, secret_key_bitstream *skb
{
    // [Internal] 宣告內部運算結構
    // interface
    public_key_bitstream pkb;
    secret_key_bitstream skb;

    // c
    cipher_text c;
    cipher_text c_star;

    // pk sk
    public_key pk_g;
    public_key pk_e;
    public_key pk_d;
    secret_key sk;

    // msg
    uint8_t msg[RUDRAKSH_len_K];
    poly m;
    poly m_prime;
    uint8_t msg_prime[RUDRAKSH_len_K];
    
    // K r
    shared_secret K;
    uint8_t kr[2 * RUDRAKSH_len_K];         // K || r
    uint8_t kr_prime[2 * RUDRAKSH_len_K];   // K' || r'
    uint8_t k_fail[RUDRAKSH_len_K];

    // 3. 準備 Secret Key 的額外資料
    uint8_t pkh[RUDRAKSH_len_K];
    uint8_t pkh2[RUDRAKSH_len_K];
    uint8_t z[RUDRAKSH_len_K];

    // 1. 執行核心 KeyGen
    rudraksh_pke_keygen(&pk_g, &sk);
    
    // 2. 序列化 Public Key (Pack -> pkb->bytes)
    // b 向量 (13-bit packed)
    polyvec_tobytes_13bit(pkb.bytes, &pk_g.b);
    // seed_A (直接複製)
    memcpy(pkb.bytes + (CRYPTO_PUBLICKEYBYTES - RUDRAKSH_len_K), pk_g.seed_A, RUDRAKSH_len_K);
    
    // 計算 H(pk)
    rudraksh_hash(pkh, pkb.bytes, CRYPTO_PUBLICKEYBYTES,RUDRAKSH_len_K);
    // 生成隨機數 z
    rudraksh_randombytes(z, RUDRAKSH_len_K);

    // 4. 序列化 Secret Key (Pack -> skb->bytes)
    // 格式: s || pk || pkh || z
    size_t offset = 0;

    // Pack s
    polyvec_tobytes_13bit(skb.bytes + offset, &sk.s);
    offset += (CRYPTO_SECRETKEYBYTES - CRYPTO_PUBLICKEYBYTES - 2*RUDRAKSH_len_K);

    // Copy pk bytes
    memcpy(skb.bytes + offset, pkb.bytes, CRYPTO_PUBLICKEYBYTES);
    offset += CRYPTO_PUBLICKEYBYTES;

    // Copy pkh
    memcpy(skb.bytes + offset, pkh, RUDRAKSH_len_K);
    offset += RUDRAKSH_len_K;

    // Copy z
    memcpy(skb.bytes + offset, z, RUDRAKSH_len_K);


// KEM Encapsulation: 輸入 PK Bytes, 輸出 CT Bytes 和 Shared Secret Bytes
//(public_key_bitstream *pkb, cipher_text *c, shared_secret *K)

    
    // 1. 反序列化 Public Key (Unpack -> Internal PK)
    polyvec_frombytes_13bit(&pk_e.b, pkb.bytes);
    memcpy(pk_e.seed_A, pkb.bytes + (CRYPTO_PUBLICKEYBYTES - RUDRAKSH_len_K), RUDRAKSH_len_K);

    // 2. 生成隨機訊息 msg
    rudraksh_randombytes(msg, RUDRAKSH_len_K);

    arrange_msg(&m, msg); // 轉換為 poly

    // 3. 計算 pkh
    rudraksh_hash(pkh2, pkb.bytes, CRYPTO_PUBLICKEYBYTES,RUDRAKSH_len_K);

    // 4. 生成 (K, r)
    // buffer = pkh || msg
    uint8_t buf[2 * RUDRAKSH_len_K];
    memcpy(buf, pkh2, RUDRAKSH_len_K);
    memcpy(buf + RUDRAKSH_len_K, msg, RUDRAKSH_len_K);
    rudraksh_hash(kr, buf, 2 * RUDRAKSH_len_K, 2*RUDRAKSH_len_K);

    // 5. 輸出 Shared Secret K
    memcpy(K.bytes, kr, RUDRAKSH_len_K);

    // 6. 加密 (PKE Encrypt 會自動處理成 c->bytes)
    // 使用 kr 的後半段作為隨機數 r
    rudraksh_pke_encrypt(&pk_e, &m, kr + RUDRAKSH_len_K, &c);


// KEM Decapsulation: 輸入 SK Bytes, CT Bytes, 輸出 Shared Secret Bytes
//(secret_key_bitstream *skb, cipher_text *c, shared_secret *K)

    // [Internal] 宣告內部結構
    

    // 1. 反序列化 Secret Key (Unpack -> Internal SK & PK)
    offset = 0;
    
    // Unpack s
    polyvec_frombytes_13bit(&sk.s, skb.bytes);
    offset += (CRYPTO_SECRETKEYBYTES - CRYPTO_PUBLICKEYBYTES - 2*RUDRAKSH_len_K);

    // Unpack pk (從 SK 中還原，用於再加密驗證)
    const uint8_t *pk_bytes_ptr = skb.bytes + offset;
    polyvec_frombytes_13bit(&pk_d.b, pk_bytes_ptr);
    memcpy(pk_d.seed_A, pk_bytes_ptr + (CRYPTO_PUBLICKEYBYTES - RUDRAKSH_len_K), RUDRAKSH_len_K);
    offset += CRYPTO_PUBLICKEYBYTES;

    // Copy pkh & z
    memcpy(pkh, skb.bytes + offset, RUDRAKSH_len_K);
    offset += RUDRAKSH_len_K;
    memcpy(z, skb.bytes + offset, RUDRAKSH_len_K);

    // 2. 解密 (得到 m')
    rudraksh_pke_decrypt(&c, &sk, &m_prime);
    original_msg(msg_prime, &m_prime); // Poly -> Bytes

    printf("M  : ");
    for(int i = 0;i<16;i++)
    {
        printf("%02X",msg[i]);
    }
    printf("\n");

    printf("M' : ");
    for(int i = 0;i<16;i++)
    {
        printf("%02X",msg_prime[i]);
    }
    printf("\n");
    
    int count = 0;
    for(int i=0;i<16;i++)
    {
        if(msg[i] != msg_prime[i])
        {
            count++;
        }
    }
    if(count != 0)
    {
        printf("Fail\n");
    }
    else
    {
        printf("Pass\n");
    }


    // 3. 重新計算 (K', r')
    uint8_t buf2[2 * RUDRAKSH_len_K];
    memcpy(buf2, pkh, RUDRAKSH_len_K);
    memcpy(buf2 + RUDRAKSH_len_K, msg_prime, RUDRAKSH_len_K);
    rudraksh_hash(kr_prime, buf2, 2 * RUDRAKSH_len_K, 2*RUDRAKSH_len_K);

    // compare kr kr_prime
    printf("\nKr  : ");
    for(int i = 0;i<32;i++)
    {
        printf("%02X",kr[i]);
        if(i%4 == 3)printf("|");
    }
    printf("\n");

    printf("Kr' : ");
    for(int i = 0;i<32;i++)
    {
        printf("%02X",kr_prime[i]);
        if(i%4 == 3)printf("|");
    }
    printf("\n");
    int count_kr =0;
    for(int i=0;i<32;i++)
    {
        if(kr[i] != kr_prime[i])count_kr++;
    }

    if(count_kr != 0)
    {
        printf("Fail\n");
    }
    else
    {
        printf("Pass\n");
    }

    // 4. 重新加密 (得到 c*)
    poly m_prime_poly;
    arrange_msg(&m_prime_poly, msg_prime);
    rudraksh_pke_encrypt(&pk_d, &m_prime_poly, kr_prime + RUDRAKSH_len_K, &c_star);

    // compare pk_g pk_e pk_d
    int same_pkg_pke = 1, same_pkg_pkd = 1;
    printf("\npk_g  : \n");
    printf("\tb:\n");
    for(int i = 0;i<4;i++)
    {
        for(int j =0;j<4;j++)
        {
            printf("%02X",pk_g.b.vec[i].coeffs[j]);
            if(j%4 == 3)printf("|");
            if(j%16 == 15)printf("\n");
        }
    }
    printf("\n");
    printf("\tseedA:\n");
    for(int i=0;i<RUDRAKSH_len_K;i++)
    {
        printf("%02X",pk_g.seed_A[i]);
        if(i%4 == 3)printf("|");
    }
    printf("\n");

    // pke
    printf("\npk_e  : \n");
    printf("\tb:\n");
    for(int i = 0;i<4;i++)
    {
        for(int j =0;j<4;j++)
        {
            printf("%02X",pk_e.b.vec[i].coeffs[j]);
            if(j%4 == 3)printf("|");
            if(j%16 == 15)printf("\n");
            if(pk_g.b.vec[i].coeffs[j] != pk_e.b.vec[i].coeffs[j]) same_pkg_pke = 0;
        }
    }
    printf("\n");
    printf("\tseedA:\n");
    for(int i=0;i<RUDRAKSH_len_K;i++)
    {
        printf("%02X",pk_e.seed_A[i]);
        if(i%4 == 3)printf("|");
        if(pk_e.seed_A[i] != pk_g.seed_A[i]) same_pkg_pke ++;
    }
    printf("\n");

    // pkd
    printf("\npk_d  : \n");
    printf("\tb:\n");
    for(int i = 0;i<4;i++)
    {
        for(int j =0;j<4;j++)
        {
            printf("%02X",pk_d.b.vec[i].coeffs[j]);
            if(j%4 == 3)printf("|");
            if(j%16 == 15)printf("\n");
            if(pk_g.b.vec[i].coeffs[j] != pk_d.b.vec[i].coeffs[j])same_pkg_pkd = 0;
        }
    }
    printf("\n");
    printf("\tseedA:\n");
    for(int i=0;i<RUDRAKSH_len_K;i++)
    {
        printf("%02X",pk_d.seed_A[i]);
        if(i%4 == 3)printf("|");
        if(pk_d.seed_A[i] != pk_g.seed_A[i]) same_pkg_pkd ++;
    }
    printf("\n");

    if(same_pkg_pke == 0)
    {
        printf("PK G-E Fail\n");
    }
    else
    {
        printf("PK G-E Pass\n");
    }

    if(same_pkg_pkd == 0)
    {
        printf("PK G-D Fail\n");
    }
    else
    {
        printf("PK G-D Pass\n");
    }
    


    // 5. 計算失敗時的 Key (K'') = H(c || z)
    uint8_t fail_input[CRYPTO_CIPHERTEXTBYTES + RUDRAKSH_len_K];
    memcpy(fail_input, c.bytes, CRYPTO_CIPHERTEXTBYTES);
    memcpy(fail_input + CRYPTO_CIPHERTEXTBYTES, z, RUDRAKSH_len_K);
    rudraksh_hash(k_fail, fail_input, sizeof(fail_input),2*RUDRAKSH_len_K);

    // 6. 驗證 c == c* (Constant Time)
    // 假設 verify 返回 0 代表相等 (成功)，1 代表不等 (失敗)
    int fail = verify(c.bytes, c_star.bytes, CRYPTO_CIPHERTEXTBYTES);

    if(fail == 0) // 成功
    {
        printf("\nV : pass\n");
    }
    else if(fail == 1)
    {   // 失敗
        printf("\nV : fail\n");
    }
    // 7. 選擇輸出 Key (Constant Time Select)
    // 如果 fail=0 (成功)，複製 kr_prime (K')
    // 如果 fail=-1 (失敗)，複製 k_fail (K'')
    cmov(K.bytes, kr_prime, RUDRAKSH_len_K, (uint8_t)!fail); // 若 !fail 為 1，則搬移 kr_prime
    cmov(K.bytes, k_fail, RUDRAKSH_len_K, (uint8_t)fail);    // 若 fail 為 -1 (0xFF)，則搬移 k_fail
    
    // 注意：cmov 的實作需確保正確覆蓋。
    // 簡單邏輯： output = (K' & ~mask) | (K'' & mask)
    
}


int main()
{
    printf("\n=============================================\n");
    printf("   KEM Tests\n");
    printf("=============================================\n");
    rudraksh_kem_test();

    // 即使在同一個函數內跑兩次，看看 c1 和 c2 是否一樣

    public_key pk;
    poly m;
    uint8_t seed_r[RUDRAKSH_len_K];
    cipher_text c1,c2;

    rudraksh_pke_encrypt(&pk, &m, seed_r, &c1);
    rudraksh_pke_encrypt(&pk, &m, seed_r, &c2);

    if (memcmp(c1.bytes, c2.bytes, CRYPTO_CIPHERTEXTBYTES) == 0) {
        printf("PASS: Encryption is deterministic.\n");
    } else {
        printf("FAIL: Encryption is NOT deterministic! Each call produces different noise.\n");
    }

    printf("\n=============================================\n");
    printf("   End of Tests\n");
    printf("=============================================\n");
}