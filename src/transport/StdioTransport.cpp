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
//   included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <iostream>
#include "StdioTransport.h"
#include "aixlog.hpp"

namespace vx::transport {

    std::pair<size_t, std::string> Stdio::Read() {
        std::string line;
        std::string json_data;

        // Read headers until an empty line
        while (true) {
            int c;
            while ((c = std::getc(stdin)) != EOF && c != '\n') {
                json_data += static_cast<char>(c);
            }
            break;
        }

        return {json_data.length(), json_data};
    }

    std::future<std::pair<size_t, std::string>> Stdio::ReadAsync() {
        return std::async(std::launch::async, []() {
            std::string json_data;
            int c;
            while ((c = std::getc(stdin)) != EOF && c != '\n') {
                json_data += static_cast<char>(c);
            }
            return std::make_pair(json_data.length(), json_data);
        });
    }

    void Stdio::Write(const std::string& json_data) {
        std::cout << json_data << std::endl << std::flush;
    }

    std::future<void> Stdio::WriteAsync(const std::string& json_data) {
        return std::async(std::launch::async, [json_data]() {
            std::cout << json_data << std::endl << std::flush;
        });
    }

}
