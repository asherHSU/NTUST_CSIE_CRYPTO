#include "rudraksh_math.h"
#include "rudraksh_params.h"
#include "rudraksh_random.h" // include ascon_prf

// ==========================================================
// 1. matrix_A generator
// ==========================================================

// once poly generator : poly in matrix A
inline void poly_generator(poly *p, const uint8_t *seed, const uint8_t i, const uint8_t j)
{
    // init state & prf
    RUDRAFKSH_STATE state;
    rudraksh_prf_init_matrixA(&state, seed, &i,&j);
    
    int count = 0; // poly 參數計數器
    uint64_t buffer = 0;   // 目前正在處理的位元區
    int bits_left = 0;     // 目前 buffer 剩幾個 bit 有效

    // 循環直到收集滿 64 個係數
    while (count < RUDRAKSH_N) {

        uint16_t val;

        // 情況 A: 舊的 buffer 夠用
        if (bits_left >= 13) {
            val = buffer & 0x1FFF;
            buffer >>= 13;
            bits_left -= 13;
        } 
        // 情況 B: 舊的 buffer 不夠 (需要拼接)
        else
        {
            // 1. 取得新的一塊資料
            uint64_t next_block = 0;
            uint8_t next_block_array[8];
            rudraksh_prf_put(&state, next_block_array);
            for (int i = 0; i < 8; i++)
            {
                next_block |= ((uint64_t)next_block_array[i] << (8 * i));
            }

            // 2. 算出還缺幾個 bits
            int needed = 13 - bits_left;

            // 3. 拼接： [新 block 的低 needed 位] + [舊 buffer 的所有位]
            // 注意：這裡直接算出 val，不需要把 next_block 全塞進 buffer
            val = (buffer) | ((next_block & ((1ULL << needed) - 1)) << bits_left);

            // 4. 更新 buffer
            // buffer 變成 next_block 裡「沒用到的部分」
            buffer = next_block >> needed;
            bits_left = 64 - needed;
        }

        // 4. 拒絕採樣 (Rejection Sampling)
        if (val < RUDRAKSH_Q)
        {
            p->coeffs[count++] = val;
        }
        // 如果 val >= 7681，就直接進入下一圈迴圈 (丟棄)
        // 但 bits_left 已經扣掉了，所以實際上就是丟掉了那 13 bits
    }
}

void poly_matrixA_generator(polymat *a, const uint8_t *seed)
{
    for (int i = 0; i < RUDRAKSH_K; i++)
    {
        for (int j = 0; j < RUDRAKSH_K; j++)
        {
            poly p;
            poly_generator(&p,seed,i,j);
            a->matrix[i][j] = p;
        }
    }
}

// ==========================================================
// 2. cbd_etc
// ==========================================================

// nonce -> Rudraksh_K = 9

// 生成 s or e with eta=1 , noce 
// s noce = [0 ~ l-1]
// e noce = [l ~ 2l-1]
void polyvec_cbd_eta(polyvec *s,polyvec *e, const uint8_t *key)
{
    for(size_t i=0;i<RUDRAKSH_K;i++) // K =9, nonce: s = [0~8] ; e = [9~17] 
    {
        poly ps,pe;

        poly_cbd_eta(&ps,key,i);
        poly_cbd_eta(&pe,key,(uint8_t)(i+RUDRAKSH_K));

        s->vec[i] = ps;
        e->vec[i] = pe;
    }
}

// 生成 e'' ，nonce = (uint8_t)RUDRAKSH*2 = 18
// 與 s,e 的生成共用，
void poly_cbd_eta(poly *e, const uint8_t *key, const uint8_t nonce)
{
    // 生成 poly coeff
    uint8_t buffer[32]; // 儲存 PRF bit stream

    // 初始化 ascon_prf
    RUDRAFKSH_STATE state;
    rudraksh_prf_init_cbd(&state, key, &nonce);
    for(size_t i=0;i<4;i++)    // 32byte = 8bytes (一次)*4
    {
        rudraksh_prf_put(&state,buffer+(i*8)); // <- test 確認
    }

    // 總共64個poly , 1 poly -> 4bit = 0.5 bytes, 64*0.5 = 32bytes = 8bytes (一次)*4
    for (int i = 0; i < 32; i++) {
        uint8_t byte = buffer[i];
        int16_t a, b;

        // --- 處理低 4 位 (生成第 2*i 個係數) ---
        // bits 0,1 是第一組 (a)，bits 2,3 是第二組 (b)
        // 技巧：(x >> 1) & 1 取出 bit 1，(x & 1) 取出 bit 0
        a = (byte & 0x1) + ((byte >> 1) & 0x1); // HW(第一組)
        b = ((byte >> 2) & 0x1) + ((byte >> 3) & 0x1); // HW(第二組)
        e->coeffs[2 * i] = a - b;

        // --- 處理高 4 位 (生成第 2*i+1 個係數) ---
        // bits 4,5 是第一組，bits 6,7 是第二組
        a = ((byte >> 4) & 0x1) + ((byte >> 5) & 0x1);
        b = ((byte >> 6) & 0x1) + ((byte >> 7) & 0x1);
        e->coeffs[2 * i + 1] = a - b;
    }
}