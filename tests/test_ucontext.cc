#include "zero/log.h"
#include <ucontext.h>

static ucontext_t uctx_main, uctx_func1, uctx_func2;

#define handle_error(msg)   \
    do {                    \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

static void func1(void) {
    printf("func1: started\n");
    printf("func1: swapcontext(&uctx_func1, &uctx_func2)\n");
    if (swapcontext(&uctx_func1, &uctx_func2) == -1)
        handle_error("swapcontext");
    printf("func1: returning\n");
}

static void func2(void) {
    printf("func2: started\n");
    printf("func2: swapcontext(&uctx_func2, &uctx_func1)\n");
    if (swapcontext(&uctx_func2, &uctx_func1) == -1)
        handle_error("swapcontext");
    printf("func2: returning\n");
}

int main(int argc, char* argv[]) {
    char func1_stack[16384];
    char func2_stack[16384];

    // 调用getcontext获取uctx_func1的上下文
    if (getcontext(&uctx_func1) == -1)
        handle_error("getcontext");
    
    // 如果直接在这里set重置上下文，会一直set陷入到死循环当中
    // setcontext(&uctx_func1);

    // 调用makecontext之前，分配新的栈，并指定ucontext_t *uc_link
    uctx_func1.uc_stack.ss_sp = func1_stack;
    uctx_func1.uc_stack.ss_size = sizeof(func1_stack);
    // 指定uc_link
    // 待当前上下文终止之后，切换到uc_link指定的上下文中去
    uctx_func1.uc_link = &uctx_main;
    // makecontext修改完上下文之后需要用setcontext或者swapcontext进行激活
    // 此时uctx_func1的上下文指向了func1的入口
    makecontext(&uctx_func1, func1, 0);

    if (getcontext(&uctx_func2) == -1)
        handle_error("getcontext");
    uctx_func2.uc_stack.ss_sp = func2_stack;
    uctx_func2.uc_stack.ss_size = sizeof(func2_stack);
    /* Successor context is f1(), unless argc > 1 */
    uctx_func2.uc_link = (argc > 1) ? NULL : &uctx_func1;
    // 此时uctx_func2的上下文指向了func2的入口
    makecontext(&uctx_func2, func2, 0);

    printf("main: swapcontext(&uctx_main, &uctx_func2)\n");
    // swapcontext是 先将当前上下文保存在第一个参数当中，然后执行第二个参数指定的上下文
    // 当uctx_func1上下文结束之后 切换到uctx_main上下文当中去继续执行
    if (swapcontext(&uctx_main, &uctx_func2) == -1)
        handle_error("swapcontext");


    printf("main: exiting\n");
    // setcontext(&uctx_main);
    exit(EXIT_SUCCESS);
}