// TaskQueue.cpp
#include "../include/taskQueue.hpp"
#include <iostream>

using std::unique_lock;
using std::cout;
using std::endl;

TaskQueue::TaskQueue(size_t capa)
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
    unique_lock<mutex>ul(_mutex);

    while(full())
    {
        cout << "TaskQueue 已满, 等待中..." << endl;
        _notFull.wait(ul);
    }

    _queue.push(std::move(pTask));
    cout << "TaskQueue::push() 添加任务, 当前队列大小: " << _queue.size() << endl;
    _notEmpty.notify_one();
}

// 获取任务
TaskQueue::ElemType TaskQueue::pop()
{
    unique_lock<mutex> ul(_mutex);

    while(empty() && !_flag)
    {
        cout << "TaskQueue 为空, 等待任务..." << endl;
        _notEmpty.wait(ul);
    }

    if(_flag)
    {
        cout << "TaskQueue::pop() 退出标志已设置, 返回 nullptr" << endl;
        return nullptr;
    }
    
    ElemType pTask = _queue.front();
    _queue.pop();
    cout << "TaskQueue::pop() 获取任务, 当前队列大小: " << _queue.size() << endl;
    _notFull.notify_one();
    
    return pTask;
}

// 判断队列是否满了
bool TaskQueue::full() const
{
    return _queue.size() == _capacity;
}

// 判断队列是否为空
bool TaskQueue::empty() const
{
    return 0 == _queue.size();
}

// 将所有等待在_notEmpty条件变量上的线程全部唤醒
void TaskQueue::wakeup()
{
    cout << "TaskQueue::wakeup() 唤醒所有等待线程" << endl;
    _flag = true;
    _notEmpty.notify_all();
}