#include <stdio.h>
#include <unistd.h>

static int calc(int a, int b)
{
    int sum = a + b;
    return sum * 2;
}

int main(void)
{
    int counter = 3;
    int result = calc(counter, 5);

    printf("pid=%d counter=%d result=%d\n",
           getpid(), counter, result);

    sleep(1);

    counter++;
    result = calc(counter, 7);

    printf("counter=%d result=%d\n", counter, result);
    return 0;
}
