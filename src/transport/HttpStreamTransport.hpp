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
//  documentation https://simplescraper.io/blog/how-to-mcp#introduction

#ifndef MCP_SERVER_HTTP_STREAM_TRANSPORT_HPP
#define MCP_SERVER_HTTP_STREAM_TRANSPORT_HPP
#include "ITransport.h"
#include "httplib.h"

namespace vx::transport {

    class HttpStream : public vx::ITransport {
    public:
        explicit HttpStream(int port = 8080, std::string host = "127.0.0.1");
        ~HttpStream();

        bool Start() override;

        void Stop() override;

        bool IsRunning() override { return server_running_.load(); }

        std::pair<size_t, std::string> Read() override;

        void Write(const std::string &json_data) override;

        std::future<std::pair<size_t, std::string>> ReadAsync() override;

        std::future<void> WriteAsync(const std::string &json_data) override;

        std::string GetName() override { return "httpstream"; }

        std::string GetVersion() override { return "0.1"; }

        int GetPort() override { return port_; }

    private:
        void SetupRoutes();
        void HandlePostMessage(const httplib::Request& req, httplib::Response& res);
        void HandleDeleteMessage(const httplib::Request& req, httplib::Response& res);
        static void SetCORSHeaders(httplib::Response& res);

        int port_;
        std::string host_;
        std::unique_ptr<httplib::Server> server_;
        std::thread server_thread_;
        std::atomic<bool> server_running_ {false};
        std::atomic<bool> client_connected_ {false};
    };

}


#endif //MCP_SERVER_HTTP_STREAM_TRANSPORT_HPP