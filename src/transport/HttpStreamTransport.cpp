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

    HttpStream::HttpStream(int port, std::string host) : port_(port), host_(std::move(host)),
        server_(std::make_unique<httplib::Server>()) {
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
        sse_stream_active_.store(false);

        if (server_) {
            server_->stop();
        }

        if (server_thread_.joinable()) {
            server_thread_.join();
        }

        incoming_cv_.notify_all();
        sse_cv_.notify_all();

        // Cancel any pending requests
        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            for (auto& [id, pending] : pending_requests_) {
                try {
                    pending->promise.set_value("");
                } catch (...) {}
            }
            pending_requests_.clear();
        }
    }

    std::pair<size_t, std::string> HttpStream::Read() {
        std::unique_lock<std::mutex> lock(incoming_mutex_);

        incoming_cv_.wait(lock, [this]() {
            return !incoming_messages_.empty() || !server_running_.load();
        });

        if (!server_running_.load() && incoming_messages_.empty()) {
            return {0, ""};
        }

        if (!incoming_messages_.empty()) {
            std::string message = incoming_messages_.front();
            incoming_messages_.pop();
            return {message.length(), message};
        }

        return {0, ""};
    }

    void HttpStream::Write(const std::string& json_data) {
        if (!client_connected_.load()) {
            return;
        }

        try {
            auto parsed = nlohmann::json::parse(json_data);

            // Check if this is a response to a pending request (has "id" and "result" or "error")
            if (parsed.contains("id") && (parsed.contains("result") || parsed.contains("error"))) {
                std::string id_str;
                if (parsed["id"].is_number()) {
                    id_str = std::to_string(parsed["id"].get<int>());
                } else {
                    id_str = parsed["id"].get<std::string>();
                }

                std::lock_guard<std::mutex> lock(pending_mutex_);
                auto it = pending_requests_.find(id_str);
                if (it != pending_requests_.end()) {
                    LOG(DEBUG) << "Routing response to pending request id=" << id_str << std::endl;
                    it->second->promise.set_value(json_data);
                    pending_requests_.erase(it);
                    return;
                }
            }

            // Server-initiated notification: queue for SSE stream
            if (sse_stream_active_.load()) {
                std::lock_guard<std::mutex> lock(sse_mutex_);
                sse_notifications_.push(json_data);
                sse_cv_.notify_one();
            }
        } catch (const std::exception& e) {
            LOG(ERROR) << "Error in Write: " << e.what() << std::endl;
        }
    }

    std::future<std::pair<size_t, std::string>> HttpStream::ReadAsync() {
        return std::async(std::launch::async, [this]() -> std::pair<size_t, std::string> {
            return Read();
        });
    }

    std::future<void> HttpStream::WriteAsync(const std::string& json_data) {
        return std::async(std::launch::async, [this, json_data]() {
            Write(json_data);
        });
    }

    void HttpStream::SetupRoutes() {
        server_->Options("/.*", [](const httplib::Request& req, httplib::Response& res) {
            HandleOptionsRequest(req, res);
        });

        server_->Get("/health", [](const httplib::Request& req, httplib::Response& res) {
            res.set_content("{\"status\":\"ok\"}", "application/json");
        });

        server_->Post("/mcp", [this](const httplib::Request& req, httplib::Response& res) {
            HandlePostMessage(req, res);
        });

        server_->Get("/mcp", [this](const httplib::Request& req, httplib::Response& res) {
            HandleGetSSE(req, res);
        });

        server_->Delete("/mcp", [this](const httplib::Request& req, httplib::Response& res) {
            HandleDeleteSession(req, res);
        });
    }

    void HttpStream::HandlePostMessage(const httplib::Request& req, httplib::Response& res) {
        SetCORSHeaders(res);

        // Validate Content-Type
        auto content_type = req.get_header_value("Content-Type");
        if (content_type.find("application/json") == std::string::npos) {
            res.status = 415;
            res.set_content("{\"error\":\"Unsupported Media Type. Expected application/json\"}", "application/json");
            return;
        }

        // Validate Accept header: client must accept application/json and optionally text/event-stream
        auto accept = req.get_header_value("Accept");
        if (!accept.empty() && accept.find("application/json") == std::string::npos) {
            res.status = 406;
            res.set_content("{\"error\":\"Not Acceptable. Must accept application/json\"}", "application/json");
            return;
        }

        std::string message = req.body;
        if (message.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"Empty message body\"}", "application/json");
            return;
        }

        nlohmann::json parsed;
        try {
            parsed = nlohmann::json::parse(message);
        } catch (const nlohmann::json::parse_error& e) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid JSON\"}", "application/json");
            return;
        }

        // Check if this is the initialize request (first message, no session required)
        bool is_initialize = parsed.contains("method") && parsed["method"] == "initialize";

        if (is_initialize) {
            // Generate session ID on initialize
            session_id_ = vx::utils::SessionBuilder::GenerateUniqueSessionID();
            session_initialized_ = true;
            client_connected_.store(true);
            LOG(INFO) << "Session initialized: " << session_id_ << std::endl;
        } else if (session_initialized_) {
            // Validate session for non-initialize requests
            if (!ValidateSession(req, res)) {
                return;
            }
        }

        // Check if this is a notification (no "id" field) or a request (has "id" field)
        bool is_notification = !parsed.contains("id");

        if (is_notification) {
            // Queue the notification for the server to process
            LOG(DEBUG) << "Received notification via POST: " << message << std::endl;
            {
                std::lock_guard<std::mutex> lock(incoming_mutex_);
                incoming_messages_.push(message);
            }
            incoming_cv_.notify_one();

            // Notifications get 202 Accepted
            res.status = 202;
            if (session_initialized_) {
                res.set_header("Mcp-Session-Id", session_id_);
            }
            return;
        }

        // This is a request - we need to wait for the response from the Server
        std::string id_str;
        if (parsed["id"].is_number()) {
            id_str = std::to_string(parsed["id"].get<int>());
        } else {
            id_str = parsed["id"].get<std::string>();
        }

        LOG(DEBUG) << "Received request via POST (id=" << id_str << "): " << message << std::endl;

        // Create a pending request with a promise/future pair
        auto pending = std::make_shared<PendingRequest>();
        std::future<std::string> response_future = pending->promise.get_future();

        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            pending_requests_[id_str] = pending;
        }

        // Queue the message for the Server to process via Read()
        {
            std::lock_guard<std::mutex> lock(incoming_mutex_);
            incoming_messages_.push(message);
        }
        incoming_cv_.notify_one();

        // Wait for the Server to process and call Write() with the response
        auto status = response_future.wait_for(std::chrono::seconds(30));
        if (status == std::future_status::timeout) {
            LOG(ERROR) << "Request timed out (id=" << id_str << ")" << std::endl;
            // Clean up the pending request
            {
                std::lock_guard<std::mutex> lock(pending_mutex_);
                pending_requests_.erase(id_str);
            }
            res.status = 504;
            res.set_content("{\"error\":\"Request timed out\"}", "application/json");
            return;
        }

        std::string response_data = response_future.get();
        if (response_data.empty()) {
            res.status = 500;
            res.set_content("{\"error\":\"Internal server error\"}", "application/json");
            return;
        }

        res.status = 200;
        res.set_content(response_data, "application/json");
        if (session_initialized_) {
            res.set_header("Mcp-Session-Id", session_id_);
        }
    }

    void HttpStream::HandleGetSSE(const httplib::Request& req, httplib::Response& res) {
        SetCORSHeaders(res);

        // Validate session
        if (session_initialized_ && !ValidateSession(req, res)) {
            return;
        }

        LOG(DEBUG) << "SSE stream client connected" << std::endl;

        res.set_header("Content-Type", "text/event-stream");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");
        if (session_initialized_) {
            res.set_header("Mcp-Session-Id", session_id_);
        }

        sse_stream_active_.store(true);

        res.set_content_provider(
            "text/event-stream",
            [this](size_t offset, httplib::DataSink& sink) -> bool {
                using clock = std::chrono::steady_clock;
                static thread_local auto last_ping = clock::now();
                const auto ping_interval = std::chrono::seconds(15);

                auto terminate = [this]() -> bool {
                    sse_stream_active_.store(false);
                    sse_cv_.notify_all();
                    return false;
                };

                try {
                    // Keep-alive ping
                    if (clock::now() - last_ping >= ping_interval) {
                        const char* ping = ": ping\n\n";
                        if (!sink.write(ping, std::strlen(ping))) {
                            LOG(ERROR) << "SSE keep-alive write failed" << std::endl;
                            return terminate();
                        }
                        last_ping = clock::now();
                    }

                    // Wait for notifications
                    std::unique_lock<std::mutex> lock(sse_mutex_);
                    sse_cv_.wait_for(lock, std::chrono::milliseconds(200), [this]() {
                        return !sse_notifications_.empty() || !sse_stream_active_.load();
                    });

                    if (!sse_stream_active_.load()) {
                        return terminate();
                    }

                    if (!sse_notifications_.empty()) {
                        std::string message = sse_notifications_.front();
                        sse_notifications_.pop();
                        lock.unlock();

                        std::string sse_msg = "event: message\ndata: " + message + "\n\n";
                        LOG(DEBUG) << "Sending SSE notification: " << message << std::endl;

                        if (!sink.write(sse_msg.data(), sse_msg.size())) {
                            LOG(ERROR) << "SSE notification write failed" << std::endl;
                            return terminate();
                        }
                    }

                    return true;
                } catch (const std::exception& ex) {
                    LOG(ERROR) << "Exception in SSE content provider: " << ex.what() << std::endl;
                    return terminate();
                } catch (...) {
                    LOG(ERROR) << "Unknown exception in SSE content provider" << std::endl;
                    return terminate();
                }
            }
        );
    }

    void HttpStream::HandleDeleteSession(const httplib::Request& req, httplib::Response& res) {
        SetCORSHeaders(res);

        if (!session_initialized_) {
            res.status = 404;
            res.set_content("{\"error\":\"No active session\"}", "application/json");
            return;
        }

        if (!ValidateSession(req, res)) {
            return;
        }

        LOG(INFO) << "Session terminated by client: " << session_id_ << std::endl;

        session_initialized_ = false;
        client_connected_.store(false);
        sse_stream_active_.store(false);

        // Wake up any blocked Read() calls
        incoming_cv_.notify_all();
        sse_cv_.notify_all();

        res.status = 200;
        res.set_content("{\"status\":\"session terminated\"}", "application/json");
    }

    bool HttpStream::ValidateSession(const httplib::Request& req, httplib::Response& res) const {
        auto client_session = req.get_header_value("Mcp-Session-Id");
        if (client_session.empty() || client_session != session_id_) {
            LOG(ERROR) << "Invalid session ID: " << client_session << " (expected: " << session_id_ << ")" << std::endl;
            res.status = 404;
            res.set_content("{\"error\":\"Invalid or missing session ID\"}", "application/json");
            return false;
        }
        return true;
    }

    void HttpStream::HandleOptionsRequest(const httplib::Request& req, httplib::Response& res) {
        SetCORSHeaders(res);
        res.status = 200;
    }

    void HttpStream::SetCORSHeaders(httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, Mcp-Session-Id");
        res.set_header("Access-Control-Expose-Headers", "Content-Type, Mcp-Session-Id");
        res.set_header("Access-Control-Max-Age", "86400");
    }

}
