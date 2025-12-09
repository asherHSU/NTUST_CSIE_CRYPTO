# include "../src/rudraksh_math.h"
# include "../src/rudraksh_random.h"
# include "../src/rudraksh_params.h"
# include <stdio.h>

int main()
{
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

    printf("Avg coffe : %lld( Avg = 3840 )",sum/coffe_n);

}