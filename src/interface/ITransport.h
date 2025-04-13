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

#ifndef MCP_SERVER_ITRANSPORT_H
#define MCP_SERVER_ITRANSPORT_H

#include <string>
#include <future>

namespace vx {

    class ITransport {
    public:
        virtual std::pair<size_t, std::string> Read() = 0;
        virtual void Write(const std::string& json_data) = 0;

        virtual std::future<std::pair<size_t, std::string>> ReadAsync() = 0;
        virtual std::future<void> WriteAsync(const std::string& json_data) = 0;

        virtual std::string GetName() = 0;
        virtual std::string GetVersion() = 0;
        virtual int GetPort() = 0;
    };

}

#endif //MCP_SERVER_ITRANSPORT_H
