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
// -----------------------------------------------------------------------------
//
//  Contributors:
//    - Erdenebileg Byamba (https://github.com/erd3n)
//        * Contribution: Initial implementation of SSE Transport
//    - Giuseppe Mastrangelo (https://github.com/peppemas)
//          * Contribution: Fixed code to be compatible with MCP Server specification
//
// -----------------------------------------------------------------------------

#ifndef MCP_SERVER_SSE_TRANSPORT_HPP
#define MCP_SERVER_SSE_TRANSPORT_HPP

#include "ITransport.h"
#include <memory>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <httplib.h>
#include "utils/SessionBuilder.h"

namespace vx::transport {

    class SSE : public vx::ITransport {
    public:
        explicit SSE(int port = 8080, std::string  host = "127.0.0.1");
        ~SSE();

        // Copy const. and assignment disabled
        SSE(const SSE&) = delete;
        SSE& operator=(const SSE&) = delete;

        // Move semantics disabled
        SSE(SSE&&) = delete;
        SSE& operator=(SSE&&) = delete;

        // Transport interface
        std::pair<size_t, std::string> Read() override;
        void Write(const std::string& json_data) override;

        std::future<std::pair<size_t, std::string>> ReadAsync() override;
        std::future<void> WriteAsync(const std::string& json_data) override;

        std::string GetName() override { return "sse"; };
        std::string GetVersion() override { return "0.4"; };
        int GetPort() override { return port_; };

        bool Start() override;
        void Stop() override;
        bool IsRunning() override { return server_running_.load(); }

    private:
        void SetupRoutes();
        void HandleSSEConnection(const httplib::Request& req, httplib::Response& res);
        void HandlePostMessage(const httplib::Request& req, httplib::Response& res);

        static void HandleOptionsRequest(const httplib::Request& req, httplib::Response& res);
        static void SetCORSHeaders(httplib::Response& res);

        std::string host_;
        int port_;
        std::unique_ptr<httplib::Server> server_;
        std::thread server_thread_;
        std::atomic<bool> server_running_ {false};
        std::atomic<bool> client_connected_ {false};

        // Message queues for bidirectional connections
        std::queue<std::string> incoming_messages_;
        std::queue<std::string> outgoing_messages_;
        std::mutex incoming_mutex_;
        std::mutex outgoing_mutex_;
        std::condition_variable incoming_cv_;
        std::condition_variable outgoing_cv_;

        // SSE connection management
        std::atomic<bool> sse_active_ {false};
    };

}

#endif //MCP_SERVER_SSE_TRANSPORT_HPP