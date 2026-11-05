// TaskQueue.cpp
#include "taskQueue.hpp"
#include <iostream>

TaskQueue::TaskQueue(std::size_t capa)
: _capacity(capa)
, _queue()
, _mutex()
, _notEmpty()
, _notFull()
, _flag(false)
{
}

TaskQueue::~TaskQueue()
{
}

// 添加任务
void TaskQueue::push(ElemType &&pTask)
{
    std::unique_lock<std::mutex> ul(_mutex);

    while(_queue.size() == _capacity && !_flag)
    {
        std::cout << "TaskQueue 已满, 等待中..." << std::endl;
        _notFull.wait(ul);
    }

    if(_flag)
    {
        return;
    }

    _queue.push(std::move(pTask));
    std::cout << "TaskQueue::push() 添加任务, 当前队列大小: " << _queue.size() << std::endl;
    _notEmpty.notify_one();
}

// 获取任务
TaskQueue::ElemType TaskQueue::pop()
{
    std::unique_lock<std::mutex> ul(_mutex);

    while(_queue.empty() && !_flag)
    {
        std::cout << "TaskQueue 为空, 等待任务..." << std::endl;
        _notEmpty.wait(ul);
    }

    if(_flag)
    {
        std::cout << "TaskQueue::pop() 退出标志已设置, 返回 nullptr" << std::endl;
        return nullptr;
    }

    ElemType pTask = _queue.front();
    _queue.pop();
    std::cout << "TaskQueue::pop() 获取任务, 当前队列大小: " << _queue.size() << std::endl;
    _notFull.notify_one();

    return pTask;
}

// 判断队列是否满了
bool TaskQueue::full() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _queue.size() == _capacity;
}

// 判断队列是否为空
bool TaskQueue::empty() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return 0 == _queue.size();
}

// 将所有等待在_notEmpty条件变量上的线程全部唤醒
void TaskQueue::wakeup()
{
    std::cout << "TaskQueue::wakeup() 唤醒所有等待线程" << std::endl;
    std::lock_guard<std::mutex> lock(_mutex);
    _flag = true;
    _notFull.notify_all();
    _notEmpty.notify_all();
}
