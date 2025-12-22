# include "../src/rudraksh_math.h"
# include "../src/rudraksh_random.h"
# include "../src/rudraksh_params.h"
# include <stdio.h>

int main()
{
    printf("\n=============================================\n");
    printf("   Generator Test\n");
    printf("=============================================\n");
    printf("[1] Matrix A generator");
    // ==========================================================
    // 1. matrix A generator
    // ==========================================================
    polymat test_matrix[10];
    // lenK = 16 byte
    uint8_t key[RUDRAKSH_len_K];
    for(int i=0;i<10;i++)
    {
        polymat pmat;
        rudraksh_randombytes(key,RUDRAKSH_len_K); // 已成功確認其功能
        poly_matrixA_generator(&pmat,key);
        test_matrix[i] = pmat;
    }
    
    int poly_n = RUDRAKSH_K*RUDRAKSH_K*10;
    int coffe_n = poly_n*RUDRAKSH_N;
    printf("Gen polys : %d ,coeffs : %d\n", poly_n, coffe_n);

    long long sum = 0;

    for(int i=0;i<10;i++)
    {
        for(int mi=0;mi<RUDRAKSH_K;mi++)
        {
            for(int mj=0;mj<RUDRAKSH_K;mj++)
            {
                for(int n=0;n<RUDRAKSH_N;n++)
                {
                    int16_t coff = test_matrix[i].matrix[mi][mj].coeffs[n];
                    if(coff >= RUDRAKSH_Q)
                    {
                        printf("Err : coeff too big\n");
                        return 0;
                    }
                    sum = sum + coff;
                }
            }
        }
    }

    printf(">> Avg coffe : %lld( Avg = 3840 )\n",sum/coffe_n);


    // ==========================================================
    // 2. cbd eta generator
    // ==========================================================
    
    // 函數設計
    uint8_t key_eta[RUDRAKSH_len_K];
    rudraksh_randombytes(key_eta,RUDRAKSH_len_K);
    polyvec test_vec_s,test_vec_e;
    poly test_poly_e;
    polyvec_cbd_eta(&test_vec_s, &test_vec_e,key_eta);
    poly_cbd_eta(&test_poly_e, key_eta, (uint8_t)18);
    printf("\n[2] CBD generator \n");

    //分布測試
    int result[5] = {0,0,0,0,0}; // 2,1,0,-1,-2
    rudraksh_randombytes(key_eta,RUDRAKSH_len_K);
    for(int i=0;i<20;i++)
    {
        poly p;
        poly_cbd_eta(&p,key_eta,(uint8_t)i);
        for(int j=0;j<64;j++)
        {
            int index = p.coeffs[j]+2;
            if(index > 4) index -= RUDRAKSH_Q;
            result[index]++; // -2~2 -> 0~4
        }
    }
    printf("Distribution test:\n"); //分佈測試
    for(int i=0;i<5;i++)
    {
        int ans[5] = {80,320,480,320,80};
        printf("%d : %d (%d)\n",i-2,result[i],ans[i]);
    }

    printf("\n[2.2] CBD generator Fixed input\n");
    // 黃金輸入 (...擷取至rudraksh_generator.c)
    uint8_t byte[3] = {0x00,0x55,0x24};
    int ans[3][2] = {
        {0,0},
        {0,0},
        {-1,1}
    };
    int16_t a, b , h, l;

    for (int i = 0; i < 3; i++)
    {
        // --- 處理低 4 位 (生成第 2*i 個係數) ---
        // bits 0,1 是第一組 (a)，bits 2,3 是第二組 (b)
        // 技巧：(x >> 1) & 1 取出 bit 1，(x & 1) 取出 bit 0
        a = (byte[i] & 0x1) + ((byte[i] >> 1) & 0x1);        // HW(第一組)
        b = ((byte[i] >> 2) & 0x1) + ((byte[i] >> 3) & 0x1); // HW(第二組)
        l = a - b;

        // --- 處理高 4 位 (生成第 2*i+1 個係數) ---
        // bits 4,5 是第一組，bits 6,7 是第二組
        a = ((byte[i] >> 4) & 0x1) + ((byte[i] >> 5) & 0x1);
        b = ((byte[i] >> 6) & 0x1) + ((byte[i] >> 7) & 0x1);
        h = a - b;

        printf("-------------------\n");
        printf("test: %d , Ans: %d\n",l,ans[i][0]);
        printf("test: %d , Ans: %d\n",h,ans[i][1]);
    }

    printf("\n=============================================\n");
    printf("   End of Tests\n");
    printf("=============================================\n");
    return 0;
}