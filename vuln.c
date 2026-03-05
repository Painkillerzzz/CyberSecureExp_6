// 简单栈溢出实验受害程序（32 位环境编译）
#include <stdio.h>
#include <stdlib.h>

void attacker_function() {
    printf("栈溢出成功：已经跳转到 attacker_function！\n");
    fflush(stdout);
}

void vulnerable() {
    char buf[64];

    printf("请输入内容：\n");
    fflush(stdout);

    // 故意使用不安全的 gets 作为栈溢出入口
    gets(buf);

    printf("收到输入：%s\n", buf);
}

int main() {
    printf("简单栈溢出实验程序\n");
    vulnerable();
    printf("程序正常结束。\n");
    return 0;
}

