#include "rudraksh_random.h"
#include "rudraksh_params.h"

// 位於ascon/xof/opt64 //
#include "ascon/api.h"
#include "ascon/ascon.h"
#include "ascon/permutations.h" // 確保這檔案裡有 P12 的定義
#include "ascon/word.h"         // 確保有 LOADBYTES, STOREBYTES, PAD

// 定義 XOF 的狀態結構 (如果 ascon.h 沒定義，就用這個)
// 通常 ascon_state_t 已經在 ascon.h 定義好了
// typedef struct { uint64_t x[5]; } ascon_state_t;

// P12 實作
void P12(ascon_state_t* s) { P12ROUNDS(s); }

// ==========================================
// 1. 初始化 (Init)
// ==========================================
void rudraksh_ascon_init(ascon_state_t* s,uint64_t iv) {
    // 參考 hash.c 的 /* initialize */ 部分
    // 注意：這裡是 XOF，所以 IV 必須是 ASCON_XOF_IV
    s->x[0] = iv;
    s->x[1] = 0;
    s->x[2] = 0;
    s->x[3] = 0;
    s->x[4] = 0;
    
    // 初始置換
    P12(s);
}

// ==========================================
// 2. 吸入種子 (Absorb)
// ==========================================
// Rudraksh 通常一次性傳入 32 bytes 的 seed，所以簡化處理
void rudraksh_ascon_absorb(ascon_state_t* s, const unsigned char* in, unsigned long long len) {
    // 參考 hash.c 的 /* absorb ... */ 部分
    
    // 處理滿塊 (Full Blocks) - 雖然 seed 通常很短，但保持完整性比較好
    while (len >= ASCON_HASH_RATE) {
        s->x[0] ^= LOADBYTES(in, 8);
        P12(s);
        in += ASCON_HASH_RATE;
        len -= ASCON_HASH_RATE;
    }
    
    // 處理最後一塊與填充 (Padding) - 這非常重要！
    // 即使 len 為 0，PAD 宏也會加上必要的 0x80... 結束符號
    s->x[0] ^= LOADBYTES(in, len);
    s->x[0] ^= PAD(len);
    
    // 吸入結束後的置換，準備進入 Squeeze 階段
    P12(s);
}

// ==========================================
// 3. Hash 擠出函式
// ==========================================
void rudraksh_ascon_hash_squeeze(ascon_state_t* s, unsigned char* out) {
    
    // ASCON 的 Rate 是 64 bits (8 bytes)
    // 邏輯：提取 -> 置換 -> 提取 -> 置換...
    
    for(int8_t i = 0; i < (RUDRAKSH_len_K / 8); i++) {
        // 1. 從狀態的第一個字 (x[0]) 提取數據
        // 注意：這裡使用 word.h 提供的 STOREBYTES 來處理 Endian
        STOREBYTES(out, s->x[0], 8);
        
        // 2. 關鍵：執行 P12 置換
        P12(s);
        
        // 3. 更新指標
        out += 8;
    }
}


void rudraksh_hash(uint8_t *output, const uint8_t *input, size_t inlen)
{
    ascon_state_t state;
    rudraksh_ascon_init(&state,ASCON_HASH_IV);
    rudraksh_ascon_absorb(&state,input,inlen);
    rudraksh_ascon_hash_squeeze(&state,output);
}

// 需要先初始化 state
void rudraksh_prf_init(RUDRAFKSH_STATE *s, uint8_t *key, uint8_t *nonce_i, uint8_t *nonce_j)
{
    uint8_t input_buf[RUDRAKSH_PRF_IN_BYTES]; // 18 bytes

    // 串接 Seed 與 Nonce
    // [ Seed (0..15) | Nonce (16, 17) ]
    memcpy(input_buf, key, 16);
    input_buf[16] = (uint8_t)(*nonce_i & 0xFF);
    input_buf[17] = (uint8_t)(*nonce_j & 0xFF);

    rudraksh_ascon_init(s,ASCON_XOF_IV);
    rudraksh_ascon_absorb(s,input_buf,RUDRAKSH_PRF_IN_BYTES);
}

// call 一次 產生 8 bytes
void rudraksh_prf_put(RUDRAFKSH_STATE *s,uint8_t *out )
{
    STOREBYTES(out, s->x[0], 8);
    P12(s);
}