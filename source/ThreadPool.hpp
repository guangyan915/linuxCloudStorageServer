#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#define DEFAULT_THREAD_NUM 10  // 默认线程池中线程的数量

using namespace std;

// 任务结构体
struct Task{
    int task_id;     // 任务ID
    void (*task_func)(int);  // 任务函数指针
    int task_arg;    // 任务参数
};

// 线程池类
class ThreadPool{
public:
    ThreadPool(int thread_num = DEFAULT_THREAD_NUM);  // 构造函数
    ~ThreadPool();  // 析构函数
    void AddTask(Task task);  // 添加任务到线程池中
    void Stop();  // 停止线程池

private:
    static void ThreadFunc(void* arg);  // 线程函数
    Task GetTask();  // 获取任务
    bool IsEmpty();  // 判断任务队列是否为空

private:
    int m_thread_num;  // 线程池中线程的数量
    bool m_stop;  // 是否停止线程池
    queue<Task> m_task_queue;  // 任务队列
    mutex m_mutex;  // 互斥锁
    condition_variable m_cv;  // 条件变量
};

// 构造函数
ThreadPool::ThreadPool(int thread_num):m_thread_num(thread_num),m_stop(false)
{
    // 创建线程并运行
    for(int i=0; i<m_thread_num; i++){
        thread t(ThreadFunc, this);
        t.detach();
    }
}

// 析构函数
ThreadPool::~ThreadPool()
{
    // 停止线程池
    Stop();
}

// 添加任务到线程池中
void ThreadPool::AddTask(Task task)
{
    unique_lock<mutex> lock(m_mutex);
    m_task_queue.push(task);
    m_cv.notify_one();  // 通知一个等待的线程
}

// 停止线程池
void ThreadPool::Stop()
{
    m_stop = true;  // 设置停止标志位为true
    m_cv.notify_all();  // 通知所有等待的线程

    // 等待所有线程退出
    while(m_thread_num){
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

// 线程函数
void ThreadPool::ThreadFunc(void* arg)
{
    ThreadPool* pool = static_cast<ThreadPool*>(arg);

    while(!pool->m_stop){
        Task task = pool->GetTask();
        if(task.task_func){
            task.task_func(task.task_arg);
        }
    }

    // 线程退出
    pool->m_thread_num--;
}

// 获取任务
Task ThreadPool::GetTask()
{
    unique_lock<mutex> lock(m_mutex);

    // 等待任务队列不为空
    while(m_task_queue.empty() && !m_stop){
        m_cv.wait(lock);
    }

    Task task;
    if(!m_task_queue.empty()){
        task = m_task_queue.front();
        m_task_queue.pop();
    }

    return task;
}

// 判断任务队列是否为空
bool ThreadPool::IsEmpty()
{
    unique_lock<mutex> lock(m_mutex);
    return m_task_queue.empty();
}

