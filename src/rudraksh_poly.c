#include <stdint.h>
#include "rudraksh_math.h"
#include "rudraksh_params.h"

// ==========================================================
// 1. Encode / Decode (訊息 <-> 多項式)
// [修正] 論文 Table 1: B = 2 (每個係數 2 bits)
// 訊息長度 = 64 * 2 bits = 128 bits = 16 bytes
// ==========================================================

/**
 * Encode: 將 16-byte 的訊息 m 轉換為多項式
 * 公式: Encode(m) = floor(q / 2^B) * m
 * 對於 B=2, q=7681: Scaling factor = floor(7681 / 4) = 1920
 */
void poly_frommsg(poly *r, const uint8_t msg[16]) {
    int i, j;
    
    // 每個 byte 有 8 bits，可以切成 4 個 2-bit 的塊
    // 總共 16 bytes * 4 = 64 個係數
    for(i=0; i<16; i++) {
        for(j=0; j<4; j++) {
            // 取出 2 bits
            int16_t mask = (msg[i] >> (2*j)) & 0x3;
            // 乘上 scaling factor 1920
            r->coeffs[4*i+j] = 1920 * mask;
        }
    }
}

/**
 * Decode: 將多項式轉換回 16-byte 訊息
 * 公式: Decode(m) = round(2^B * m / q)
 * 這裡 B=2，即 round(4 * x / 7681)
 */
void poly_tomsg(uint8_t msg[16], const poly *a) {
    int i, j;
    uint32_t t;

    for(i=0; i<16; i++) {
        msg[i] = 0;
        for(j=0; j<4; j++) {
            // t = (a * 4 + q/2) / q
            t  = ((uint32_t)a->coeffs[4*i+j] << 2) + (KYBER_Q/2);
            t /= KYBER_Q;
            // 取 2 bits 並組合回 byte
            msg[i] |= ((t & 0x3) << (2*j));
        }
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

    for(i=0; i<KYBER_N/4; i++) {
        for(int j=0; j<4; j++) {
            // round(1024 * x / 7681)
            uint32_t val = (uint32_t)a->coeffs[4*i+j];
            val = (val << 10) + (KYBER_Q/2);
            val = val / KYBER_Q;
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
    for(i=0; i<KYBER_N/4; i++) {
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
            val = (val * KYBER_Q) + 512;
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

    for(i=0; i<KYBER_N/8; i++) {
        for(int j=0; j<8; j++) {
            // round(8 * x / 7681)
            uint32_t val = (uint32_t)a->coeffs[8*i+j];
            val = (val << 3) + (KYBER_Q/2);
            val = val / KYBER_Q;
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
    for(i=0; i<KYBER_N/8; i++) {
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
            val = (val * KYBER_Q) + 4; // + t/2 (4)
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
    for(i=0; i<KYBER_N/8; i++) {
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
    for(i=0; i<KYBER_N/8; i++) {
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
    for(int i=0; i<KYBER_K; i++) {
        // 每個 poly 佔 104 bytes (64 * 13 / 8)
        poly_tobytes_13bit(r + i*104, &a->vec[i]);
    }
}

void polyvec_frombytes_13bit(polyvec *r, const uint8_t *a) {
    for(int i=0; i<KYBER_K; i++) {
        poly_frombytes_13bit(&r->vec[i], a + i*104);
    }
}

// 向量壓縮 U (10-bit)
void polyvec_compress_u(uint8_t *r, const polyvec *a) {
    for(int i=0; i<KYBER_K; i++) {
        // 每個 poly 壓縮後佔 80 bytes (64 * 10 / 8)
        poly_compress_u(r + i*80, &a->vec[i]);
    }
}

void polyvec_decompress_u(polyvec *r, const uint8_t *a) {
    for(int i=0; i<KYBER_K; i++) {
        poly_decompress_u(&r->vec[i], a + i*80);
    }
}

// 向量 NTT 轉換
void polyvec_ntt(polyvec *r) {
    for(int i=0; i<KYBER_K; i++) {
        poly_ntt(&r->vec[i]);
    }
}

void polyvec_invntt_tomont(polyvec *r) {
    for(int i=0; i<KYBER_K; i++) {
        poly_invntt_tomont(&r->vec[i]);
    }
}