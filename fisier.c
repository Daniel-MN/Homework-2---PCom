#include <stdio.h>

extern void func(int &x);

int main() {
    int x = 1;
    func(x);
    printf("%d\n", x);
    return 0;
}