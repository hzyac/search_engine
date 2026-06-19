// noncopyable.hpp
#ifndef SEARCH_ENGINE_NON_COPYABLE_HPP
#define SEARCH_ENGINE_NON_COPYABLE_HPP

// 禁止拷贝构造和赋值操作的基类
class NonCopyable {
protected:
    NonCopyable();
    ~NonCopyable();

    NonCopyable(const NonCopyable &rhs) = delete;
    NonCopyable& operator=(const NonCopyable &rhs) = delete;
};

#endif