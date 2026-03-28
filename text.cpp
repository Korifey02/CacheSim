// test 1
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




// test 2
// int main()
// {
//     int i, j, n;
//     int a[100];
//     int b[100];
//     int c[100];
//
//     n = 1000;
//
//     for (i = 0; i < n; i = i + 1) {
//         a[i] = i * 2;
//         b[i] = n - i;
//     }
//
//     for (i = 0; i < n; i = i + 1) {
//         c[i] = a[i] + b[i];
//     }
//
//     for (i = 0; i < n; i = i + 1) {
//         for (j = 0; j < n; j = j + 1) {
//             if (i == j) {
//                 a[i] = a[i] * b[j];
//             }
//         }
//     }
//
//     return 0;
// }



// test 3
// int sum(int n)
// {
//     int i, s;
//     s = 0;
//     i = 1;
//     do {
//         s = s + i;
//         i = i + 1;
//     } while (i < n);
//     return s;
// }
//
// int main()
// {
//     int i, n, r;
//     int a[20];
//
//     n = 10;
//
//     for (i = 0; i < n; i = i + 1) {
//         if (i == 5) {
//             break;
//         }
//         a[i] = i * i;
//     }
//
//     for (i = 0; i < n; i = i + 1) {
//         if (a[i] == 0) {
//             continue;
//         }
//         a[i] = a[i] + 1;
//     }
//
//     r = sum(n);
//
//     return 0;
// }