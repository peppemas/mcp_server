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

#ifndef MCP_SERVER_HTTP_STREAM_TRANSPORT_HPP
#define MCP_SERVER_HTTP_STREAM_TRANSPORT_HPP
#include "ITransport.h"
#include "httplib.h"
#include <memory>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_map>
#include "utils/SessionBuilder.h"

namespace vx::transport {

    class HttpStream : public vx::ITransport {
    public:
        explicit HttpStream(int port = 8080, std::string host = "127.0.0.1");
        ~HttpStream();

        HttpStream(const HttpStream&) = delete;
        HttpStream& operator=(const HttpStream&) = delete;
        HttpStream(HttpStream&&) = delete;
        HttpStream& operator=(HttpStream&&) = delete;

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
        void HandleGetSSE(const httplib::Request& req, httplib::Response& res);
        void HandleDeleteSession(const httplib::Request& req, httplib::Response& res);
        static void HandleOptionsRequest(const httplib::Request& req, httplib::Response& res);
        static void SetCORSHeaders(httplib::Response& res);

        bool ValidateSession(const httplib::Request& req, httplib::Response& res) const;

        int port_;
        std::string host_;
        std::unique_ptr<httplib::Server> server_;
        std::thread server_thread_;
        std::atomic<bool> server_running_ {false};
        std::atomic<bool> client_connected_ {false};

        // Session management
        std::string session_id_;
        bool session_initialized_ {false};

        // Incoming message queue (client -> server, consumed by Read())
        std::queue<std::string> incoming_messages_;
        std::mutex incoming_mutex_;
        std::condition_variable incoming_cv_;

        // Pending request responses: maps request id (string) -> response string
        struct PendingRequest {
            std::promise<std::string> promise;
        };
        std::unordered_map<std::string, std::shared_ptr<PendingRequest>> pending_requests_;
        std::mutex pending_mutex_;

        // SSE stream for server-initiated notifications (GET /mcp)
        std::queue<std::string> sse_notifications_;
        std::mutex sse_mutex_;
        std::condition_variable sse_cv_;
        std::atomic<bool> sse_stream_active_ {false};
    };

}


#endif //MCP_SERVER_HTTP_STREAM_TRANSPORT_HPP
