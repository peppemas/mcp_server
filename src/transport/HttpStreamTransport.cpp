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

#include "HttpStreamTransport.hpp"

#include "aixlog.hpp"
#include "json.hpp"

namespace vx::transport {

    HttpStream::HttpStream(int port, std::string host) : port_(port), host_(std::move(host)) {
        SetupRoutes();
    }

    HttpStream::~HttpStream() {
        HttpStream::Stop();
    }

    bool HttpStream::Start() {
        if (server_running_.load()) {
            return false;
        }

        server_running_.store(true);

        server_thread_ = std::thread([this]() {
            LOG(INFO) << "Starting HttpStream server on " << host_ << ":" << port_ << std::endl;

            if (!server_->listen(host_.c_str(), port_)) {
                LOG(ERROR) << "Failed to start HttpStream server on " << host_ << ":" << port_ << std::endl;
                server_running_.store(false);
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return server_running_.load();
    }

    void HttpStream::Stop() {
        if (!server_running_.load()) {
            return;
        }

        server_running_.store(false);
        client_connected_.store(false);

        if (server_) {
            server_->stop();
        }

        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

    void HttpStream::Write(const std::string& data) {
    }

    std::future<void> HttpStream::WriteAsync(const std::string &json_data) {
        return std::async(std::launch::async, []() {
             // do nothing (default)
         });
    }

    std::pair<size_t, std::string> HttpStream::Read() {
        return {0, ""};
    }

    std::future<std::pair<size_t, std::string>> HttpStream::ReadAsync() {
        return std::async(std::launch::async, []() -> std::pair<size_t, std::string> {
            return {0, ""}; // default empty read
        });
    }

    void HttpStream::SetupRoutes() {
        server_->Post("/mcp", [this](const httplib::Request& req, httplib::Response& res) {
            HandlePostMessage(req, res);
        });
        server_->Delete("/mcp", [this](const httplib::Request& req, httplib::Response& res) {
            HandleDeleteMessage(req, res);
        });
    }

    void HttpStream::HandlePostMessage(const httplib::Request& req, httplib::Response& res) {
    }

    void HttpStream::HandleDeleteMessage(const httplib::Request &req, httplib::Response &res) {
    }

    void HttpStream::SetCORSHeaders(httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS, DELETE");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, Mcp-Session-Id");
        res.set_header("Access-Control-Expose-Headers", "Mcp-Session-Id, WWW-Authenticate");
    }

}
