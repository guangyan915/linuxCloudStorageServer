#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <chrono>

using namespace std;

struct Task {
    function<void(int)> func;                                   // 任务函数
    int fd;                                                     // 任务参数
    Task(){}
    Task(function<void(int)> f, int _fd) : func(f), fd(_fd) {}
};

class ThreadPool {
public:
    ThreadPool(int min_threads, int max_threads, int max_queue_size, int idle_time);
    ~ThreadPool();
    void AddTask(Task task);                                    // 添加任务到线程池中
    void Stop();                                                // 停止线程池

private:
    void ThreadFunc();                                          // 线程函数
    Task GetTask();                                             // 获取任务
    bool IsEmpty();                                             // 判断任务队列是否为空
    void AdjustThreadPool();                                    // 动态调整线程池的大小

private:
    const int m_min_threads;                                    // 线程池中最少的线程数
    const int m_max_threads;                                    // 线程池中最多的线程数
    const int m_max_queue_size;                                 // 任务队列的最大长度
    const int m_idle_time;                                      // 线程的空闲时间（单位：毫秒）
    bool m_stop{ false };                                       // 是否停止线程池
    queue<Task> m_task_queue;                                   // 任务队列
    vector<thread> m_threads;                                   // 线程池中的所有线程
    mutex m_mutex;                                              // 互斥锁
    condition_variable m_cv;                                    // 条件变量
    atomic<int> m_busy_threads{ 0 };                            // 正在执行任务的线程数
    atomic<int> m_alive_threads{ 0 };                           // 存活的线程数
};

ThreadPool::ThreadPool(int min_threads, int max_threads, int max_queue_size, int idle_time)
    : m_min_threads(min_threads),
    m_max_threads(max_threads),
    m_max_queue_size(max_queue_size),
    m_idle_time(idle_time) {
    // 创建最少线程数的线程并运行
    for (int i = 0; i < m_min_threads; i++) {
        m_threads.emplace_back(&ThreadPool::ThreadFunc, this);
        m_alive_threads++;
    }
}

ThreadPool::~ThreadPool() {
    // 停止线程池
    Stop();
}

void ThreadPool::AddTask(Task task) {
    unique_lock<mutex> lock(m_mutex);
    // 如果任务队列已经满了，则阻塞等待
    m_cv.wait(lock, [this] { return m_task_queue.size() < m_max_queue_size || m_stop; });
    if (!m_stop) {
        m_task_queue.push(std::move(task));
        m_cv.notify_one();  // 通知一个等待的线程    
    }
}

void ThreadPool::Stop() {
    m_stop = true;  // 设置停止标志位为true
    m_cv.notify_all();  // 通知所有等待的线程
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    m_threads.clear();
}

void ThreadPool::ThreadFunc() {
    while (!m_stop) {
        auto task = GetTask();
        if (task.func) {
            m_busy_threads++;  // 正在执行任务的线程数加1
            task.func(task.fd);  // 执行任务函数
            m_busy_threads--;  // 正在执行任务的线程数减1
        }
    }
    m_alive_threads--;  // 存活的线程数减1
}

Task ThreadPool::GetTask() {
    unique_lock<mutex> lock(m_mutex);
    // 等待任务队列不为空或线程池停止
    m_cv.wait(lock, [this] { return !m_task_queue.empty() || m_stop; });
    Task task;
    if (!m_task_queue.empty()) {
        task = std::move(m_task_queue.front());
        m_task_queue.pop();
        m_cv.notify_one();  // 通知一个等待的线程
    }
    return task;
}

bool ThreadPool::IsEmpty() {
    unique_lock<mutex> lock(m_mutex);
    return m_task_queue.empty();
}

// 动态调整线程池的大小
void ThreadPool::AdjustThreadPool() {
    int queue_size = m_task_queue.size();
    int busy_threads = m_busy_threads.load();
    int alive_threads = m_alive_threads.load();

    // 如果任务队列已满且线程池中的线程数没有达到最大值，则创建新的线程
    if (queue_size > m_min_threads * 2 && alive_threads < m_max_threads) {
        m_threads.emplace_back(&ThreadPool::ThreadFunc, this);
        m_alive_threads++;
        return;
    }

    // 如果任务队列为空且存活的线程数超过最小值，则减少线程
    if (queue_size == 0 && busy_threads * 2 < alive_threads && alive_threads > m_min_threads) {
        m_alive_threads--;
        return;
    }

    // 如果有空闲的线程超过一定时间，则减少线程
    for (auto it = m_threads.begin(); it != m_threads.end() && alive_threads > m_min_threads; ) {
        if (it->joinable()) {
            it->join();
            it = m_threads.erase(it);
            m_alive_threads--;
            alive_threads--;
        }
        else {
            it++;
        }
    }
}

