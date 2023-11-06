#include <iostream>

#include "thread_pool.h"

// 设置线程睡眠时间
void simulate_hard_computation() {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

// 添加两个数字的简单函数并打印结果
void multiply(const int a, const int b) {
    simulate_hard_computation();
    const int res = a * b;
    std::cout << a << " * " << b << " = " << res << std::endl;
}

// 添加并输出结果
void multiply_output(int& out, const int a, const int b) {
    simulate_hard_computation();
    out = a * b;
    // std::cout << a << " * " << b << " = " << out << std::endl;
}

// 结果返回
int multiply_return(const int a, const int b) {
    simulate_hard_computation();
    const int res = a * b;
    // std::cout << a << " * " << b << " = " << res << std::endl;
    return res;
}

int main() {
    ThreadPool pool(4);

    pool.init();

    for (int i = 1; i <= 2; ++i) {
        for (int j = 1; j <= 3; ++j) {
            pool.submit(multiply, i, j);
        }
    }

    int  output;
    auto future1 = pool.submit(multiply_output, std::ref(output), 10, 20);

    future1.get();
    std::cout << 10 << " * " << 20 << " = " << output << std::endl;

    auto future2 = pool.submit(multiply_return, 20, 30);

    int output2 = future2.get();
    std::cout << 20 << " * " << 30 << " = " << output2 << std::endl;

    pool.shutdown();

    return 0;
}