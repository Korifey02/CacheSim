int main()
{
    int i;
    int j;
    int k;
    int n;
    int a[4]; /* 2x2 = 4 */
    int b[4];
    int c[4];
    
    n = 2;

    
    /* Инициализация массивов */
    for (i = 0; i < 4; i = i + 1) {
        a[i] = i % 6;
        b[i] = 1 % 11;
    }

    /* Перемножение матриц */
    for (i = 0; i < n; i = i + 1) {
        for (j = 0; j < n; j = j + 1) {
            c[i * n + j] = 0;
            for (k = 0; k < n; k = k + 1) {
                c[i * n + j] = c[i * n + j] + a[i * n + k] * b[k * n + j];
            }
        }
    }

    return 0;
}
