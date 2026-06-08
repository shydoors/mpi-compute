#include "cuda_runtime.h"
#include "device_launch_parameters.h"
/** 定义了与设备（GPU）相关的参数，例如线程块大小等。*/
#include <stdio.h>

__global__ void myfirstkernel(void)
{
}

int main()
{
    myfirstkernel<<<1, 3>>>();
    /**
     * 第一个数字1表示线程块的数量（grid size），
     * 这里只有一个线程块。
     * 第二个数字3表示每个线程块中的线程数量（block
     * size），这里每个线程块有3个线程。
     */
    printf("Hello, CUDA!\n");
}
