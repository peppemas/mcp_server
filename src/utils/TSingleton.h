//
// Created by giuse on 04 apr 2025.
//

#ifndef MCP_SERVER_TSINGLETON_H
#define MCP_SERVER_TSINGLETON_H

#include <memory>
#include <mutex>

template <typename T>
class TSingleton
{
public:
    // Deleted copy/move constructors and assignment operators
    TSingleton(const TSingleton&) = delete;
    TSingleton(TSingleton&&) = delete;
    TSingleton& operator=(const TSingleton&) = delete;
    TSingleton& operator=(TSingleton&&) = delete;

    static T& GetInstance()
    {
        std::call_once(initFlag, []() {
            instance.reset(new T());
        });
        return *instance;
    }

protected:
    TSingleton() = default;
    virtual ~TSingleton() = default;

private:
    static std::unique_ptr<T> instance;
    static std::once_flag initFlag;
};

// Static member definitions
template <typename T>
std::unique_ptr<T> TSingleton<T>::instance = nullptr;

template <typename T>
std::once_flag TSingleton<T>::initFlag;

#endif //MCP_SERVER_TSINGLETON_H
