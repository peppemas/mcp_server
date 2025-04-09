//  The MIT License
//
//  Copyright (C) 2025 Giuseppe Mastrangelo
//
//  Permission is hereby granted, free of charge, to any person obtaining
//  a copy of this software and associated documentation files (the
//  'Software'), to deal in the Software without restriction, including
//  without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to
//  permit persons to whom the Software is furnished to do so, subject to
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be
//  included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
