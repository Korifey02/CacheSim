/* test_func_call: Вызов пользовательских функций + рекурсия
   Проверяет: определение функций, передача параметров,
   return, do-while, break, continue.
*/

int sum(int n)
{
    int i, s;
    s = 0;
    i = 1;
    do {
        s = s + i;
        i = i + 1;
    } while (i < n);
    return s;
}

int main()
{
    int i, n, r;
    int a[20];

    n = 10;

    for (i = 0; i < n; i = i + 1) {
        if (i == 5) {
            break;
        }
        a[i] = i * i;
    }

    for (i = 0; i < n; i = i + 1) {
        if (a[i] == 0) {
            continue;
        }
        a[i] = a[i] + 1;
    }

    r = sum(n);

    return 0;
}
