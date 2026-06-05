#include <stdio.h>
#include <stdlib.h>

// 声明 add 函数，并使用 noinline 属性强制禁止内联
int add(int a, int b) __attribute__((noinline));

int add(int a, int b) {
    return a + b;
}

int main(int argc, char** argv) {
    // 使用 volatile 防止编译器在生成 IR 阶段把变量优化成常量
    volatile int x = 3;
    volatile int y = 4;

    // 引入外部不可预测的输入，给 main 函数穿上“防弹衣”
    // if (argc > 2) {
    //     x = atoi(argv[1]);
    //     y = atoi(argv[2]);
    // }

    int result = add(x, y);

    printf("最终的计算结果是: %d\n", result);
    return 0;
}
