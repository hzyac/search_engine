// ThreadPool.cpp
#include "../include/threadPool.hpp"
#include "../include/cache.hpp"
#include <iostream>
#include <exception>
#include <chrono>
#include <thread>

using std::cout;
using std::endl;

ThreadPool::ThreadPool(size_t threadNum, size_t queSize)
: _threadNum(threadNum)
, _threads()
, _queSize(queSize)
, _taskQueue(_queSize)
, _isExit(false)
, _maincache(new Cache())
{
    cout << "ThreadPool() 构造, 线程数: " << _threadNum << ", 队列大小: " << _queSize << endl;
}

ThreadPool::~ThreadPool()
{
    cout << "~ThreadPool() 析构, 删除 maincache" << endl;
    delete _maincache;
}

// 启动线程池
void ThreadPool::start()
{
    for(size_t i = 0; i < _threadNum; ++i)
    {
        _threads.push_back(thread(&ThreadPool::doTask, this));
        cout  << "创建线程id: " << _threads.back().get_id() << endl;
    }

    // 添加一个线程用来做定时处理缓存
    _threads.push_back(thread(&ThreadPool::broadCastcache, this));
    cout  << "创建线程id: " << _threads.back().get_id() << endl;
}

// 关闭线程池
void ThreadPool::stop()
{
    cout << "ThreadPool::stop() 开始关闭线程池" << endl;
    int waitCnt = 0;
    while(!_taskQueue.empty() && waitCnt < 10)
    {   
        cout << "ThreadPool 等待任务队列清空, 已等待 " << (++waitCnt) << " 秒" << endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    if(waitCnt >= 10)
    {
        cout << "ThreadPool 等待超时, 强制退出" << endl;
    }

    _isExit = true;
    _taskQueue.wakeup();

    cout << "ThreadPool 等待 " << _threadNum << " 个工作线程退出..." << endl;
    for(int i = 0; i < _threadNum; ++i)
    {
        cout << "ThreadPool 回收工作线程 " << i << endl;
        _threads[i].join(); // 回收子线程
    }
    // 回收处理缓存的线程
    cout << "ThreadPool 回收广播线程" << endl;
    _threads.back().join();
    cout << "ThreadPool::stop() 完成" << endl;
}

// 添加任务
void ThreadPool::addTask(Task &&pTask)
{
    if(pTask)
    {
        cout << "ThreadPool::addTask() 向任务队列添加任务" << endl;
        _taskQueue.push(std::move(pTask)); 
    }
}

// 获取任务
ThreadPool::Task ThreadPool::getTask()
{
    return _taskQueue.pop();
}

// 线程池交给工作线程执行的任务
void ThreadPool::doTask()
{
    // 创建读写线程
    Cache *threadcacheRW = new Cache();
    _subcachesRW.push_back(threadcacheRW);
    // 创建同步线程
    Cache *threadcacheSY = new Cache();
    _subcachesSY.push_back(threadcacheSY);
    // 获取所在位置
    size_t id = _subcachesRW.size() - 1;
    cout << "工作线程 " << std::this_thread::get_id() << " 启动, 下标id: " << id << endl;
    while(!_isExit)
    {
        Task &&pTask = getTask();
        // 每次进行更新, 防止交换过了
        threadcacheRW = _subcachesRW[id];
        if(pTask)
        {
            cout << "工作线程 " << std::this_thread::get_id() << " 开始执行任务" << endl;
            try
            {
                pTask(*threadcacheRW);
            }
            catch(const std::exception &e)
            {
                cout << "工作线程捕获异常: " << e.what() << endl;
            }
            cout << "工作线程 " << std::this_thread::get_id() << " 任务执行完毕" << endl;
        }
        else
        {
            cout << "pTask == nullptr" << endl;
        }
    }
    cout << "工作线程 " << std::this_thread::get_id() << " 退出, 删除本地缓存" << endl;
    delete _subcachesRW[id];
    delete _subcachesSY[id];
}

// 线程池中的所有线程的缓存统一起来
void ThreadPool::broadCastcache()
{
    cout << "广播线程 " << std::this_thread::get_id() << " 启动, 每30秒进行一次缓存同步" << endl;
    while(!_isExit)
    {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        cout << "\n====== 广播线程: 开始缓存同步 ======" << endl;

        // 进行缓存交换
        for(int i = 0; i < _subcachesSY.size(); ++i)
        {
            _subcachesSY[i]->lock();
            _subcachesRW[i]->lock();
            std::swap(_subcachesRW[i], _subcachesSY[i]);
            _subcachesRW[i]->unlock();
            _subcachesSY[i]->unlock();
        }

        cout << "Cache broadcast, sub-thread Cache sizes before:" << endl;
        for(Cache *subcache : _subcachesSY)
        {
            subcache->lock();
            cout << subcache->cache_size() << "  ";
            _maincache->merge(*subcache);
            subcache->clear();
            subcache->unlock();
        }
        cout << endl;

        cout << "更新缓存后的每个子线程缓存个数:" << endl;
        // 对所有子线程的缓存进行修改
        for(Cache *subcache : _subcachesSY)
        {
            subcache->lock();
            subcache->update(*_maincache);
            cout << subcache->cache_size() << ' ';
            subcache->unlock();
        }
        cout << endl;

        // 进行缓存交换
        for(int i = 0; i < _subcachesSY.size(); ++i)
        {
            _subcachesSY[i]->lock();
            _subcachesRW[i]->lock();
            std::swap(_subcachesRW[i], _subcachesSY[i]);
            _subcachesRW[i]->unlock();
            _subcachesSY[i]->unlock();
        }
    }
}