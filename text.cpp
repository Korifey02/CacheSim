int main()
{
    int i, j, k, n, l, m;
    int a[10000];
    int b[10000];
    int c[10000];
    n = 100;
    for(i = 0; i < 10000; i=i+1) {
        a[i] = i % 6;
        b[i] = 1 % 11;
    }
    for(i = 0; i < n; i=i+1) {
        for(j = 0; j < n; j=j+1) {
            c[i*n+j] = 0;
            for(k = 0; k < n; k=k+1) {
                c[i*n+j] = c[i*n+j] + a[i*n+k] * b[k*n+j];
            }
        }
    }
    return 0;
}
