// ThreadPool.hpp
#ifndef SEARCH_ENGINE_THREAD_POOL_HPP
#define SEARCH_ENGINE_THREAD_POOL_HPP

#include "taskQueue.hpp"
#include <cstddef>
#include <functional>
#include <thread>
#include <vector>

using std::function;
using std::thread;
using std::vector;

class Cache;

class ThreadPool {
public:
    using Task = function<void(Cache &Cache)>;

public:
    ThreadPool(size_t threadNum, size_t queSize);
    ~ThreadPool();

    // 启动线程池
    void start();
    // 关闭线程池
    void stop();

    // 添加任务
    void addTask(Task &&pTask);
    // 获取任务
    Task getTask();

    // 线程池交给工作线程执行的任务, 传入线程id方便获取交换后的指针
    void doTask();

    // 线程池中的所有线程的缓存统一起来
    void broadCastcache();

private:
    // 子线程的数目
    size_t _threadNum;
    // 存放子线程的容器
    vector<thread> _threads;
    // 任务队列的大小
    size_t _queSize; 
    // 存放任务的任务队列
    TaskQueue _taskQueue;
    // 标识线程池有没有退出的标志位
    bool _isExit; 
    // cache缓存
    Cache *_maincache;
    // 存储所有子线程的读写缓存
    vector<Cache *> _subcachesRW;
    // 存储所有子线程的同步缓存
    vector<Cache *> _subcachesSY;
};

#endif