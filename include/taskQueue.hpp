// TaskQueue.hpp
#pragma once

#include<queue>
#include<functional>
#include<mutex>
#include<condition_variable>

using std::queue;
using std::function;
using std::mutex;
using std::condition_variable;

class Cache;

class TaskQueue
{
public:
    using ElemType = function<void(Cache &Cache)>;

public:
    TaskQueue(size_t capa);
    ~TaskQueue();

    // 添加任务
    void push(ElemType &&pTask);
    
    // 获取任务
    ElemType pop();

    // 判断队列是否满了
    bool full() const;

    // 判断队列是否为空
    bool empty() const;

    // 将所有等待在_notEmpty条件变量上的线程全部唤醒
    void wakeup();
private:
    // 任务队列的容量
    size_t _capacity;
    // 存放数据的数据结构
    queue<ElemType> _queue;
    // 互斥锁
    mutex _mutex; 
    // 非空条件变量
    condition_variable _notEmpty;
    // 非满条件变量
    condition_variable _notFull;
    // 标志这个队列是否结束任务
    bool _flag; 
};
