#ifndef SEARCH_ENGINE_THREAD_POOL_HPP
#define SEARCH_ENGINE_THREAD_POOL_HPP
#include "taskQueue.hpp"
#include <cstddef>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class Cache;

class ThreadPool
{
public:
    using Task = std::function<void(Cache &)>;

    ThreadPool(std::size_t threadNum, std::size_t queueSize);
    ~ThreadPool();
    void start();
    void stop();
    void addTask(Task &&task);

private:
    void doTask(std::size_t id);
    void broadCastcache();

    std::size_t _threadNum;
    TaskQueue _taskQueue;
    std::atomic<bool> _isExit{false};
    std::vector<std::thread> _threads;
    std::vector<std::unique_ptr<Cache>> _caches;
    std::unique_ptr<Cache> _maincache;
    std::mutex _timerMutex;
    std::condition_variable _timer;
};

#endif
