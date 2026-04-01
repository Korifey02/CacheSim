/* test_vecadd: Сложение векторов
   Проверяет: линейный обход массивов (stride-1),
   один цикл, локальные массивы.
*/

int main()
{
    int i, n;
    int a[1000];
    int b[1000];
    int c[1000];

    n = 1000;

    for (i = 0; i < n; i = i + 1) {
        a[i] = i * 2;
        b[i] = n - i;
    }

    for (i = 0; i < n; i = i + 1) {
        c[i] = a[i] + b[i];
    }

    return 0;
}
