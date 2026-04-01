/* test_while_loop: while-цикл с накоплением суммы
   Проверяет: while, скалярные переменные в выражениях,
   чтение массива внутри while.
*/

int main()
{
    int i, sum, n;
    int a[1024];

    n = 1024;
    sum = 0;

    for (i = 0; i < n; i = i + 1) {
        a[i] = i;
    }

    i = 0;
    while (i < n) {
        sum = sum + a[i];
        i = i + 1;
    }

    return sum;
}
