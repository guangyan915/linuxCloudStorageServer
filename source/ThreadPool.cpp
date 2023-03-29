#include <iostream>
#include <chrono>
#include "ThreadPool.hpp"

using namespace std;

void PrintTask(int task_id) {
    cout << "Task " << task_id << " is running in thread " << this_thread::get_id() << endl;
    this_thread::sleep_for(chrono::seconds(1));
}

int main() {
    ThreadPool thread_pool(4); // 创建一个包含4个线程的线程池

    for (int i = 1; i <= 10; i++) {
        Task task{i, PrintTask, i};
        thread_pool.AddTask(task); // 将任务添加到线程池中
    }

    this_thread::sleep_for(chrono::seconds(5)); // 等待5秒钟，确保所有任务都已执行完毕

    thread_pool.Stop(); // 停止线程池

    return 0;
}

