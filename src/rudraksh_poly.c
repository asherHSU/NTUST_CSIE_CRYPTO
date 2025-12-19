#include <stdint.h>
#include "rudraksh_math.h"
#include "rudraksh_params.h"

// ==========================================================
// 1. Encode / Decode (訊息 <-> 多項式)
// [修正] 論文 Table 1: B = 2 (每個係數 2 bits)
// 訊息長度 = 64 * 2 bits = 128 bits = 16 bytes
// ==========================================================

/**
 * Arrange_msg: 將 16-byte 的 msg 格式化為多項式 m
 * 僅做位元拆解，不進行數學放大。
 * 輸入: msg (16 bytes)
 * 輸出: m (64 個係數，每個係數值域 0~3)
 */
void arrange_msg(poly *m, const uint8_t msg[16]) {
    int i, j;
    
    for(i=0; i<16; i++) {
        uint8_t byte = msg[i];
        for(j=0; j<4; j++) {
            // 取出 2 bits，不做任何乘法
            // j=0 -> bits 0-1
            // j=1 -> bits 2-3 ...
            m->coeffs[4*i+j] = (byte >> (2*j)) & 0x3;
        }
    }
}

/**
 * Original_msg: 將多項式 m' 還原回 16-byte 訊息
 * 假設輸入的係數已經經過 Decode，數值都在 0~3 之間。
 * 輸入: m (64 個係數，值域 0~3)
 * 輸出: msg (16 bytes)
 */
void original_msg(uint8_t msg[16], const poly *m) {
    int i, j;

    for(i=0; i<16; i++) {
        msg[i] = 0; // 初始化為 0
        for(j=0; j<4; j++) {
            // 取出係數的低 2 位 (防止前面步驟有髒數據)
            uint8_t val = m->coeffs[4*i+j] & 0x3;
            // 移位並組裝
            msg[i] |= (val << (2*j));
        }
    }
}

/**
 * Encode: 將小係數多項式放大，以容納雜訊
 * 公式: m * floor(q / 2^B) = m * 1920
 * 輸入: m (係數 0~3)
 * 輸出: r (係數 0, 1920, 3840, 5760)
 */
void poly_encode(poly *r, const poly *m) {
    // 預計算常數: 7681 / 4 = 1920
    const int16_t factor = 1920; 

    for(int i=0; i<RUDRAKSH_N; i++) {
        // 純粹的純量乘法
        r->coeffs[i] = m->coeffs[i] * factor;
    }
}

/**
 * Decode: 從帶雜訊的多項式中還原原始係數
 * 公式: round(2^B * x / q) -> round(4 * x / 7681)
 * 輸入: noisy_poly (係數值域 0~7680，帶有雜訊)
 * 輸出: m (係數還原回 0~3)
 */
void poly_decode(poly *m, const poly *noisy_poly) {
    for(int i=0; i<RUDRAKSH_N; i++) {
        // 1. 取得數值 (假設已標準化為正數)
        uint32_t t = (uint32_t)noisy_poly->coeffs[i];
        
        // 2. 執行四捨五入公式: (t * 4 + q/2) / q
        // RUDRAKSH_Q = 7681, q/2 = 3840
        t = (t << 2) + 3840;
        t /= RUDRAKSH_Q;
        
        // 3. 取模 4 (確保結果是 0~3)
        m->coeffs[i] = t & 0x3;
    }
}

// ==========================================================
// 2. Compress / Decompress U (向量 u)
// 論文參數: p = 2^10 = 1024 (10 bits)
// ==========================================================

/**
 * Compress U: 將係數從 mod q 壓縮到 10 bits
 * Bit Packing: 4 個係數 (40 bits) -> 5 bytes
 */
void poly_compress_u(uint8_t *r, const poly *a) {
    int i;
    uint16_t t[4];
    int ctr = 0;

    for(i=0; i<RUDRAKSH_N/4; i++) {
        for(int j=0; j<4; j++) {
            // round(1024 * x / 7681)
            uint32_t val = (uint32_t)a->coeffs[4*i+j];
            val = (val << 10) + (RUDRAKSH_Q/2);
            val = val / RUDRAKSH_Q;
            t[j] = val & 0x3FF;
        }

        // Pack 4x10 bits -> 5 bytes
        r[ctr+0] = (t[0] >> 0);
        r[ctr+1] = (t[0] >> 8) | (t[1] << 2);
        r[ctr+2] = (t[1] >> 6) | (t[2] << 4);
        r[ctr+3] = (t[2] >> 4) | (t[3] << 6);
        r[ctr+4] = (t[3] >> 2);
        ctr += 5;
    }
}

void poly_decompress_u(poly *r, const uint8_t *a) {
    int i;
    int ctr = 0;
    for(i=0; i<RUDRAKSH_N/4; i++) {
        uint16_t t[4];
        // Unpack 5 bytes -> 4x10 bits
        t[0] = (a[ctr+0] >> 0) | ((uint16_t)a[ctr+1] << 8);
        t[1] = (a[ctr+1] >> 2) | ((uint16_t)a[ctr+2] << 6);
        t[2] = (a[ctr+2] >> 4) | ((uint16_t)a[ctr+3] << 4);
        t[3] = (a[ctr+3] >> 6) | ((uint16_t)a[ctr+4] << 2);

        for(int j=0; j<4; j++) {
            t[j] &= 0x3FF;
            // round(7681 * x / 1024)
            uint32_t val = (uint32_t)t[j];
            val = (val * RUDRAKSH_Q) + 512;
            val >>= 10;
            r->coeffs[4*i+j] = (int16_t)val;
        }
        ctr += 5;
    }
}

// ==========================================================
// 3. [新增] Compress / Decompress V (多項式 v)
// 論文參數: t = 2^3 = 8 (3 bits)
// ==========================================================

/**
 * Compress V: 將係數壓縮到 3 bits
 * Bit Packing: 8 個係數 (24 bits) -> 3 bytes
 */
void poly_compress_v(uint8_t *r, const poly *a) {
    int i;
    uint8_t t[8];
    int ctr = 0;

    for(i=0; i<RUDRAKSH_N/8; i++) {
        for(int j=0; j<8; j++) {
            // round(8 * x / 7681)
            uint32_t val = (uint32_t)a->coeffs[8*i+j];
            val = (val << 3) + (RUDRAKSH_Q/2);
            val = val / RUDRAKSH_Q;
            t[j] = val & 0x7; // 取 3 bits
        }

        // Pack 8x3 bits -> 3 bytes
        // byte 0: t0(3) | t1(3) | t2(2)
        r[ctr+0] = (t[0]) | (t[1] << 3) | (t[2] << 6);
        // byte 1: t2(1) | t3(3) | t4(3) | t5(1)
        r[ctr+1] = (t[2] >> 2) | (t[3] << 1) | (t[4] << 4) | (t[5] << 7);
        // byte 2: t5(2) | t6(3) | t7(3)
        r[ctr+2] = (t[5] >> 1) | (t[6] << 2) | (t[7] << 5);
        
        ctr += 3;
    }
}

void poly_decompress_v(poly *r, const uint8_t *a) {
    int i;
    int ctr = 0;
    for(i=0; i<RUDRAKSH_N/8; i++) {
        uint8_t t[8];
        
        // Unpack 3 bytes -> 8x3 bits
        t[0] = a[ctr+0] & 0x7;
        t[1] = (a[ctr+0] >> 3) & 0x7;
        t[2] = ((a[ctr+0] >> 6) | (a[ctr+1] << 2)) & 0x7;
        t[3] = (a[ctr+1] >> 1) & 0x7;
        t[4] = (a[ctr+1] >> 4) & 0x7;
        t[5] = ((a[ctr+1] >> 7) | (a[ctr+2] << 1)) & 0x7;
        t[6] = (a[ctr+2] >> 2) & 0x7;
        t[7] = (a[ctr+2] >> 5) & 0x7;

        for(int j=0; j<8; j++) {
            // round(7681 * x / 8)
            uint32_t val = (uint32_t)t[j];
            val = (val * RUDRAKSH_Q) + 4; // + t/2 (4)
            val >>= 3; // divide by 8
            r->coeffs[8*i+j] = (int16_t)val;
        }
        ctr += 3;
    }
}

// ==========================================================
// 4. [新增] 13-bit Serialization (用於 PK 和 SK)
// 論文參數: q = 7681 (13 bits)
// ==========================================================

/**
 * Serialize 13-bit: 將多項式係數 (mod q) 打包成 bytes
 * 用途: 生成 Public Key (b) 和 Secret Key (s)
 * 邏輯: 8 個係數 (8 * 13 = 104 bits) -> 13 bytes
 */
void poly_tobytes_13bit(uint8_t *r, const poly *a) {
    int i;
    int ctr = 0;
    for(i=0; i<RUDRAKSH_N/8; i++) {
        // 取出 8 個係數
        uint16_t t[8];
        for(int k=0; k<8; k++) t[k] = a->coeffs[8*i+k] & 0x1FFF; // Mask 13 bits

        // 暴力打包: 8 coeffs -> 13 bytes
        r[ctr+0]  = (t[0] >> 0);
        r[ctr+1]  = (t[0] >> 8) | (t[1] << 5);
        r[ctr+2]  = (t[1] >> 3);
        r[ctr+3]  = (t[1] >> 11) | (t[2] << 2);
        r[ctr+4]  = (t[2] >> 6) | (t[3] << 7);
        r[ctr+5]  = (t[3] >> 1);
        r[ctr+6]  = (t[3] >> 9) | (t[4] << 4);
        r[ctr+7]  = (t[4] >> 4);
        r[ctr+8]  = (t[4] >> 12) | (t[5] << 1);
        r[ctr+9]  = (t[5] >> 7) | (t[6] << 6);
        r[ctr+10] = (t[6] >> 2);
        r[ctr+11] = (t[6] >> 10) | (t[7] << 3);
        r[ctr+12] = (t[7] >> 5);

        ctr += 13;
    }
}

/**
 * De-serialize 13-bit: 從 bytes 還原係數
 */
void poly_frombytes_13bit(poly *r, const uint8_t *a) {
    int i;
    int ctr = 0;
    for(i=0; i<RUDRAKSH_N/8; i++) {
        uint16_t t[8];
        
        t[0] = (a[ctr+0] >> 0) | ((uint16_t)a[ctr+1] << 8);
        t[1] = (a[ctr+1] >> 5) | ((uint16_t)a[ctr+2] << 3) | ((uint16_t)a[ctr+3] << 11);
        t[2] = (a[ctr+3] >> 2) | ((uint16_t)a[ctr+4] << 6);
        t[3] = (a[ctr+4] >> 7) | ((uint16_t)a[ctr+5] << 1) | ((uint16_t)a[ctr+6] << 9);
        t[4] = (a[ctr+6] >> 4) | ((uint16_t)a[ctr+7] << 4) | ((uint16_t)a[ctr+8] << 12);
        t[5] = (a[ctr+8] >> 1) | ((uint16_t)a[ctr+9] << 7);
        t[6] = (a[ctr+9] >> 6) | ((uint16_t)a[ctr+10] << 2) | ((uint16_t)a[ctr+11] << 10);
        t[7] = (a[ctr+11] >> 3) | ((uint16_t)a[ctr+12] << 5);

        for(int k=0; k<8; k++) r->coeffs[8*i+k] = t[k] & 0x1FFF;
        
        ctr += 13;
    }
}

// ==========================================================
// 5. [新增] Vector Wrappers (處理 l=9 的向量)
// ==========================================================

// 向量打包 (13-bit)
void polyvec_tobytes_13bit(uint8_t *r, const polyvec *a) {
    for(int i=0; i<RUDRAKSH_K; i++) {
        // 每個 poly 佔 104 bytes (64 * 13 / 8)
        poly_tobytes_13bit(r + i*104, &a->vec[i]);
    }
}

void polyvec_frombytes_13bit(polyvec *r, const uint8_t *a) {
    for(int i=0; i<RUDRAKSH_K; i++) {
        poly_frombytes_13bit(&r->vec[i], a + i*104);
    }
}

// 向量壓縮 U (10-bit)
void polyvec_compress_u(uint8_t *r, const polyvec *a) {
    for(int i=0; i<RUDRAKSH_K; i++) {
        // 每個 poly 壓縮後佔 80 bytes (64 * 10 / 8)
        poly_compress_u(r + i*80, &a->vec[i]);
    }
}

void polyvec_decompress_u(polyvec *r, const uint8_t *a) {
    for(int i=0; i<RUDRAKSH_K; i++) {
        poly_decompress_u(&r->vec[i], a + i*80);
    }
}

// 向量 NTT 轉換
void polyvec_ntt(polyvec *r) {
    for(int i=0; i<RUDRAKSH_K; i++) {
        poly_ntt(&r->vec[i]);
    }
}

void polyvec_invntt_tomont(polyvec *r) {
    for(int i=0; i<RUDRAKSH_K; i++) {
        poly_invntt_tomont(&r->vec[i]);
    }
}