/* test_global_array: Глобальный массив + простой линейный обход
   Проверяет: глобальные массивы, read-after-write.
*/

int a[1024];

int main()
{
    int i;

    for (i = 0; i < 1024; i = i + 1) {
        a[i] = i;
    }

    for (i = 0; i < 1024; i = i + 1) {
        a[i] = a[i];
    }

    return 0;
}
