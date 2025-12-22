# include "../src/rudraksh_params.h"
# include "../src/rudraksh_crypto.h"
# include "../src/rudraksh_math.h"
# include "../src/rudraksh_random.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ==========================================================
// 輔助工具
// ==========================================================

// ANSI 顏色碼，讓輸出比較好看
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED   "\033[0;31m"
#define COLOR_RESET "\033[0m"

// 檢查 Buffer 是否全為 0 (全為 0 通常代表生成失敗)
int check_buffer_not_zero(const uint8_t *buf, size_t len, const char *name) {
    int is_zero = 1;
    for(size_t i=0; i<len; i++) {
        if (buf[i] != 0) {
            is_zero = 0;
            break;
        }
    }
    if (is_zero) {
        printf("[%sFAIL%s] %s is all zeros! (Data generation failed)\n", COLOR_RED, COLOR_RESET, name);
        return 0;
    }
    return 1;
}

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

int assert_bytes_eq(const uint8_t *a, const uint8_t *b, size_t len, const char *msg) {
    if (memcmp(a, b, len) == 0) {
        printf("[%sPASS%s] %s\n", COLOR_GREEN, COLOR_RESET, msg);
        return 1;
    } else {
        printf("[%sFAIL%s] %s\n", COLOR_RED, COLOR_RESET, msg);
        printf("Expected (first 8 bytes): ");
        for(size_t i=0; i<8 && i<len; i++) printf("%02X ", a[i]);
        printf("\nActual   (first 8 bytes): ");
        for(size_t i=0; i<8 && i<len; i++) printf("%02X ", b[i]);
        printf("\n");
        return 0;
    }
}

// ==========================================================
// 1. PKE 個別階段測試
// ==========================================================

void test_PKE_GEN() {
    printf("\n--- [Test] PKE KeyGen ---\n");
    public_key pk;
    secret_key sk;

    // 清空記憶體以確保測試生成的有效性
    memset(&pk, 0, sizeof(public_key));
    memset(&sk, 0, sizeof(secret_key));

    // 執行
    rudraksh_pke_keygen(&pk, &sk);

    // 驗證
    // 檢查 Seed 是否被寫入
    if (check_buffer_not_zero(pk.seed_A, RUDRAKSH_len_K, "pk.seed_A")) {
        printf("[%sPASS%s] PKE KeyGen finished without errors.\n", COLOR_GREEN, COLOR_RESET);
    }
}

void test_PKE_ENC() {
    printf("\n--- [Test] PKE Encryption ---\n");
    public_key pk;
    secret_key sk;
    cipher_text c;
    poly m;
    uint8_t r[RUDRAKSH_len_K * 2];
    uint8_t msg_bytes[RUDRAKSH_len_K];

    // Setup: 需要先有 Key
    rudraksh_pke_keygen(&pk, &sk);
    rudraksh_randombytes(msg_bytes, RUDRAKSH_len_K);
    arrange_msg(&m, msg_bytes);
    rudraksh_randombytes(r, sizeof(r));
    memset(&c, 0, sizeof(cipher_text));

    // 執行
    rudraksh_pke_encrypt(&pk, &m, r, &c);

    // 驗證
    if (check_buffer_not_zero(c.bytes, CRYPTO_CIPHERTEXTBYTES, "Ciphertext")) {
        printf("[%sPASS%s] PKE Encryption generated non-zero ciphertext.\n", COLOR_GREEN, COLOR_RESET);
    }
}

void test_PKE_DEC() {
    printf("\n--- [Test] PKE Decryption ---\n");
    public_key pk;
    secret_key sk;
    cipher_text c;
    poly m_in, m_out;
    uint8_t r[RUDRAKSH_len_K * 2];
    uint8_t msg_in[RUDRAKSH_len_K];
    uint8_t msg_out[RUDRAKSH_len_K];

    // Setup: 產生 Key -> 產生 Msg -> 加密
    rudraksh_pke_keygen(&pk, &sk);
    rudraksh_randombytes(msg_in, RUDRAKSH_len_K);
    arrange_msg(&m_in, msg_in);
    rudraksh_randombytes(r, sizeof(r));
    rudraksh_pke_encrypt(&pk, &m_in, r, &c);

    // 執行
    rudraksh_pke_decrypt(&c, &sk, &m_out);

    // 轉換回 bytes 以便比較
    original_msg(msg_out, &m_out);

    // 驗證
    assert_bytes_eq(msg_in, msg_out, RUDRAKSH_len_K, "PKE Decrypted message matches original");
}

// ==========================================================
// 2. KEM 個別階段測試
// ==========================================================

void test_KEM_GEN() {
    printf("\n--- [Test] KEM KeyGen ---\n");
    public_key_bitstream pkb = {0};
    secret_key_bitstream skb = {0};

    memset(&pkb, 0, sizeof(pkb));
    memset(&skb, 0, sizeof(skb));

    // 執行
    rudraksh_kem_keygen(&pkb, &skb);

    // 驗證
    int pass_pk = check_buffer_not_zero(pkb.bytes, CRYPTO_PUBLICKEYBYTES, "Public Key Bits");
    int pass_sk = check_buffer_not_zero(skb.bytes, CRYPTO_SECRETKEYBYTES, "Secret Key Bits");

    if(pass_pk && pass_sk) {
        printf("[%sPASS%s] KEM KeyGen successful.\n", COLOR_GREEN, COLOR_RESET);
    }
}

void test_KEM_ENC() {
    printf("\n--- [Test] KEM Encapsulation ---\n");
    public_key_bitstream pkb = {0};
    secret_key_bitstream skb = {0};
    cipher_text c = {0};
    shared_secret k_enc = {0};

    // Setup
    rudraksh_kem_keygen(&pkb, &skb);
    memset(&c, 0, sizeof(c));
    memset(&k_enc, 0, sizeof(k_enc));

    // 執行
    rudraksh_kem_encapsulate(&pkb, &c, &k_enc);

    // 驗證
    int pass_ct = check_buffer_not_zero(c.bytes, CRYPTO_CIPHERTEXTBYTES, "Ciphertext");
    int pass_ss = check_buffer_not_zero(k_enc.bytes, RUDRAKSH_len_K, "Shared Secret");

    if(pass_ct && pass_ss) {
        printf("[%sPASS%s] KEM Encapsulation generated output.\n", COLOR_GREEN, COLOR_RESET);
    }
}

void test_KEM_DEC() {
    printf("\n--- [Test] KEM Decapsulation ---\n");
    public_key_bitstream pkb = {0};
    secret_key_bitstream skb = {0};
    cipher_text c = {0};
    shared_secret k_enc = {0}, k_dec = {0};

    // Setup
    rudraksh_kem_keygen(&pkb, &skb);
    rudraksh_kem_encapsulate(&pkb, &c, &k_enc);
    memset(&k_dec, 0, sizeof(k_dec));

    // 執行
    rudraksh_kem_decapsulate(&skb, &c, &k_dec);

    // 驗證
    assert_bytes_eq(k_enc.bytes, k_dec.bytes, RUDRAKSH_len_K, "KEM Shared Secrets Match");
}

// ==========================================================
// 1. PKE 單元測試 (Unit Test)
//    測試: PKE KeyGen -> Encrypt -> Decrypt -> Compare Message
// ==========================================================
void test_pke_correctness() {
    printf("\n=== Test 2.1: PKE Correctness (Internal Logic) ===\n");

    public_key pk;
    secret_key sk;
    cipher_text ct;
    poly m_original, m_decrypted;
    uint8_t msg_bytes_in[RUDRAKSH_len_K];
    uint8_t msg_bytes_out[RUDRAKSH_len_K];
    uint8_t coins[RUDRAKSH_len_K * 2]; // 隨機數 r (假設長度足夠)

    // 1. 生成金鑰
    rudraksh_pke_keygen(&pk, &sk);

    // 2. 準備隨機訊息
    rudraksh_randombytes(msg_bytes_in, RUDRAKSH_len_K);
    arrange_msg(&m_original, msg_bytes_in); // 將 bytes 轉換為多項式 m

    // 3. 準備加密用的隨機數 r
    rudraksh_randombytes(coins, RUDRAKSH_len_K * 2);

    // 4. 加密
    rudraksh_pke_encrypt(&pk, &m_original, coins, &ct);

    // 5. 解密
    rudraksh_pke_decrypt(&ct, &sk, &m_decrypted);

    // 6. 驗證 (將多項式轉回 bytes 進行比較)
    original_msg(msg_bytes_out, &m_decrypted);

    assert_bytes_eq(msg_bytes_in, msg_bytes_out, RUDRAKSH_len_K, "Decrypted message matches original");
}

// ==========================================================
// 2. KEM 綜合測試 (Integration Test)
//    測試: KEM KeyGen -> Encaps -> Decaps -> Compare Shared Secret
// ==========================================================
void test_kem_correctness() {
    printf("\n=== Test 2.2: KEM Encapsulation/Decapsulation ===\n");

    public_key_bitstream pkb = {0};
    secret_key_bitstream skb = {0};
    cipher_text ct = {0};
    shared_secret ss_alice = {0}, ss_bob = {0};

    // 1. KeyGen (Server side)
    rudraksh_kem_keygen(&pkb, &skb);

    // 2. Encapsulation (Client side)
    // Client 使用 pkb 生成共享密鑰 ss_alice 和密文 ct
    rudraksh_kem_encapsulate(&pkb, &ct, &ss_alice);

    // 3. Decapsulation (Server side)
    // Server 使用 skb 和 ct 還原共享密鑰 ss_bob
    rudraksh_kem_decapsulate(&skb, &ct, &ss_bob);

    // 4. 驗證共享密鑰一致性
    assert_bytes_eq(ss_alice.bytes, ss_bob.bytes, RUDRAKSH_len_K, "Shared secrets match");
}

// ==========================================================
// 3. KEM 錯誤處理測試 (Negative Test)
//    測試: 篡改密文 -> Decaps -> 驗證 Shared Secret 不一致 (隱式拒絕)
// ==========================================================
void test_kem_implicit_rejection() {
    printf("\n=== Test 3: KEM Implicit Rejection (Security) ===\n");

    public_key_bitstream pkb;
    secret_key_bitstream skb;
    cipher_text ct;
    shared_secret ss_alice, ss_bob;

    // 1. 正常流程 setup
    rudraksh_kem_keygen(&pkb, &skb);
    rudraksh_kem_encapsulate(&pkb, &ct, &ss_alice);

    // 2. 攻擊：隨機修改密文的一個 byte
    // 這模擬了攻擊者攔截並修改密文，或者傳輸錯誤
    ct.bytes[0] ^= 0xFF; 
    ct.bytes[CRYPTO_CIPHERTEXTBYTES/2] ^= 0xAA;

    // 3. 嘗試解封裝
    rudraksh_kem_decapsulate(&skb, &ct, &ss_bob);

    // 4. 驗證
    // 因為 Rudraksh 是 CCA 安全的，解封裝應該不會崩潰，
    // 而是會生成一個與 Alice 不同的隨機密鑰 (Implicit Rejection)。
    if (memcmp(ss_alice.bytes, ss_bob.bytes, RUDRAKSH_len_K) != 0) {
        printf("[%sPASS%s] Rejected invalid ciphertext (Keys do NOT match)\n", COLOR_GREEN, COLOR_RESET);
    } else {
        printf("[%sFAIL%s] Ciphertext was modified but keys still match! (CCA failure)\n", COLOR_RED, COLOR_RESET);
    }
}

// ==========================================================
// Main Function
// ==========================================================
int main() {
    // 初始化隨機數種子 (如果 randombytes 實作依賴 system time)
    srand(time(NULL));

    printf("=======================================\n");
    printf(" Rudraksh Unit & Integration Tests\n");
    printf("=======================================\n");
    printf("\n=== Test 1: Test PKE / KEM function singel ===\n");
    // 1. PKE 個別測試
    test_PKE_GEN();
    test_PKE_ENC();
    test_PKE_DEC();

    // 2. KEM 個別測試
    test_KEM_GEN();
    test_KEM_ENC();
    test_KEM_DEC();

    printf("=============================================\n");
    printf("   Rudraksh KEM Functional Verification\n");
    printf("=============================================\n");
    printf("Parameters:\n");
    printf("  Public Key Size:  %d bytes\n", CRYPTO_PUBLICKEYBYTES);
    printf("  Secret Key Size:  %d bytes\n", CRYPTO_SECRETKEYBYTES);
    printf("  Ciphertext Size:  %d bytes\n", CRYPTO_CIPHERTEXTBYTES);
    printf("  Shared Secret:    %d bytes\n", RUDRAKSH_len_K);

    // 執行測試
    test_pke_correctness();
    test_kem_correctness();
    test_kem_implicit_rejection();

    // 簡單的壓力測試 (跑 100 次確保沒有隨機性導致的邊緣錯誤)
    printf("\n=== Test 4: Stress Test (100 iterations) ===\n");
    int fails = 0;
    for(int i=0; i<100; i++) {
        public_key_bitstream pkb = {0};
        secret_key_bitstream skb = {0};
        cipher_text ct = {0};
        shared_secret k1 = {0}, k2 = {0};

        rudraksh_kem_keygen(&pkb, &skb);
        rudraksh_kem_encapsulate(&pkb, &ct, &k1);
        rudraksh_kem_decapsulate(&skb, &ct, &k2);

        if (memcmp(k1.bytes, k2.bytes, RUDRAKSH_len_K) != 0) {
            fails++;
            printf("Iteration %d failed!\n", i);
        }
    }
    
    if (fails == 0) {
        printf("[%sPASS%s] All 100 iterations successful.\n", COLOR_GREEN, COLOR_RESET);
    } else {
        printf("[%sFAIL%s] %d failures detected in stress test.\n", COLOR_RED, COLOR_RESET, fails);
    }

    printf("\n=============================================\n");
    printf("   End of Tests\n");
    printf("=============================================\n");

    return 0;
}