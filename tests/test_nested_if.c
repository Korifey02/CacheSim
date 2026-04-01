/* test_nested_if: Вложенные условия + цикл with if/else
   Проверяет: if/else внутри цикла, комбинации
   скалярных и массивных операций.
*/

int main()
{
    int i, j, n;
    int a[100];
    int b[100];
    int c[100];

    n = 100;

    for (i = 0; i < n; i = i + 1) {
        a[i] = i * 2;
        b[i] = n - i;
    }

    for (i = 0; i < n; i = i + 1) {
        if (a[i] > b[i]) {
            c[i] = a[i] - b[i];
        }
        else {
            c[i] = b[i] - a[i];
        }
    }

    return 0;
}
