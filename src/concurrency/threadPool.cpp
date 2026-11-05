#include "threadPool.hpp"
#include "cache.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>

ThreadPool::ThreadPool(std::size_t threadNum, std::size_t queueSize)
: _threadNum(threadNum)
, _taskQueue(queueSize)
, _maincache(std::make_unique<Cache>())
{
    if(threadNum == 0 || queueSize == 0)
    {
        throw std::invalid_argument("Thread and queue counts must be positive");
    }

    for(std::size_t i = 0; i < threadNum; ++i)
    {
        _caches.push_back(std::make_unique<Cache>());
    }
}

ThreadPool::~ThreadPool()
{
    stop();
}

void ThreadPool::start()
{
    if(!_threads.empty())
    {
        return;
    }

    try
    {
        for(std::size_t i = 0; i < _threadNum; ++i)
        {
            _threads.emplace_back(&ThreadPool::doTask, this, i);
        }

        _threads.emplace_back(&ThreadPool::broadCastcache, this);
    }
    catch(...)
    {
        stop();
        throw;
    }
}

void ThreadPool::stop()
{
    _isExit = true;
    _taskQueue.wakeup();
    _timer.notify_all();

    for(auto &thread : _threads)
    {
        if(thread.joinable())
        {
            thread.join();
        }
    }

    _threads.clear();
}

void ThreadPool::addTask(Task &&task)
{
    if(task)
    {
        _taskQueue.push(std::move(task));
    }
}

void ThreadPool::doTask(std::size_t id)
{
    while(!_isExit)
    {
        auto task = _taskQueue.pop();
        if(task)
        {
            try
            {
                task(*_caches[id]);
            }
            catch(const std::exception &error)
            {
                std::cerr << "Worker error: " << error.what() << '\n';
            }
        }
    }
}

void ThreadPool::broadCastcache()
{
    std::unique_lock<std::mutex> timerLock(_timerMutex);
    while(!_timer.wait_for(timerLock, std::chrono::seconds(30),
                           [this]
                           {
                               return _isExit.load();
                           }))
    {
        for(auto &cache : _caches)
        {
            cache->lock();
            _maincache->merge(*cache);
            cache->clear();
            cache->unlock();
        }

        for(auto &cache : _caches)
        {
            cache->lock();
            cache->update(*_maincache);
            cache->unlock();
        }
    }
}
