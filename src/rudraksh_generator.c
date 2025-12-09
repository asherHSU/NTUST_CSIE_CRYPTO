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
    rudraksh_prf_init(&state, seed, &i,&j);
    
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

