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

    printf("\n[Comparison Result mod q (7681)]\n");
    printf(" Index  |  Actual  |  standard |   Diff \n");
    printf("-----------------------------------------------\n");
    for (int i = 0; i < RUDRAKSH_N; i++) {
        uint16_t orig = (uint16_t)v_temp.coeffs[i];
        if (i < 8) {
            printf("%7d | %8d | %9d | %6d \n", i, orig,1920*(i%4),(1920*(i%4))-orig);
        }
    }printf("-----------------------------------------------\n");
    printf("Forecast| stand+dif|     -     |  < 500 (mod q) \n");

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

int main()
{
    printf("\n=============================================\n");
    printf("   PKE Tests\n");
    printf("=============================================\n");
    
    rudraksh_pke_test();

    printf("\n=============================================\n");
    printf("   End of Tests\n");
    printf("=============================================\n");
}