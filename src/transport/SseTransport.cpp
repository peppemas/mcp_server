#include "SseTransport.h"

#include <iostream>
#include <httplib.h>
#include <sstream>
#include <chrono>

#include "aixlog.hpp"

namespace vx::transport {

    SSE::SSE(int port, const std::string& host) : host_(host), port_(port), server_(std::make_unique<httplib::Server>()) {
        SetupRoutes();
    }

    SSE::~SSE() {
        Stop();
    }

    void SSE::SetupRoutes() {
        server_->Options("/.*", [this](const httplib::Request& req, httplib::Response& res) {
            HandleOptionsRequest(req, res);
        });

        server_->Get("/health", [](const httplib::Request& req, httplib::Response& res) {
            res.set_content("{\"status\" : \"ok\"}", "application/json");
        });

        server_->Post("/mcp", [this](const httplib::Request& req, httplib::Response& res) {
            HandlePostMessage(req, res);
        });

        server_->Get("/mcp", [this](const httplib::Request& req, httplib::Response& res) {
            HandleSSEConnection(req, res);
        });

    }

    void SSE::HandleSSEConnection(const httplib::Request& req, httplib::Response& res) {
        std::cout << "SSE client connected" << std::endl;
        
        SetCORSHeaders(res);
        res.set_header("Content-Type", "text/event-stream");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");

        client_connected_.store(true);
        sse_active_.store(true);

        res.set_content_provider(
            "text/event-stream",
            [this](size_t offset, httplib::DataSink& sink) -> bool {
                static bool first_call = true;
                
                if (first_call) {
                    first_call = false;
                    std::string welcome = "data: {\"type\":\"connected\"}\n\n";
                    sink.write(welcome.data(), welcome.size());
                }

                std::unique_lock<std::mutex> lock(outgoing_mutex_);
                if (outgoing_cv_.wait_for(lock, std::chrono::milliseconds(100), 
                    [this]() { return !outgoing_messages_.empty() || !sse_active_.load(); })) {
                    
                    if (!outgoing_messages_.empty() && sse_active_.load()) {
                        std::string message = outgoing_messages_.front();
                        outgoing_messages_.pop();
                        lock.unlock();

                        std::string sse_msg = "data: " + message + "\n\n";
                        std::cout << "Sending SSE message: " << message << std::endl;
                        
                        if (!sink.write(sse_msg.data(), sse_msg.size())) {
                            std::cout << "Failed to write SSE message" << std::endl;
                            sse_active_.store(false);
                            client_connected_.store(false);
                            return false;
                        }
                    }
                }

                if (!sse_active_.load()) {
                    std::cout << "SSE connection terminating" << std::endl;
                    client_connected_.store(false);
                    return false;
                }
                
                return true;
            }
        );
    }

    bool SSE::Start() {
        if (server_running_.load()) {
            return false;
        }

        server_running_.store(true);

        server_thread_ = std::thread([this]() {
            LOG(INFO) << "Starting SSE server on " << host_ << ":" << port_ << std::endl;

            if (!server_->listen(host_.c_str(), port_)) {
                LOG(ERROR) << "Failed to start SSE server on " << host_ << ":" << port_ << std::endl;
                server_running_.store(false);
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return server_running_.load();
    }

    void SSE::Stop() {
        if (server_running_.load()) {
            return;
        }

        server_running_.store(false);
        client_connected_.store(false);
        sse_active_.store(false);

        if (server_) {
            server_->stop();
        }

        if (server_thread_.joinable()) {
            server_thread_.join();
        }

        incoming_cv_.notify_all();
        outgoing_cv_.notify_all();
    }

    void SSE::HandlePostMessage(const httplib::Request& req, httplib::Response& res) {
        SetCORSHeaders(res);
        
        if (!client_connected_.load()) {
            res.status = 503;
            res.set_content("{\"error\":\"No SSE connection\"}", "application/json");
            return;
        }

        std::string message = req.body;
        if (message.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"Empty message\"}", "application/json");
            return;
        }

        std::cout << "Received message via POST: " << message << std::endl;

        {
            std::lock_guard<std::mutex> lock(incoming_mutex_);
            incoming_messages_.push(message);
        }
        incoming_cv_.notify_one();

        res.status = 200;
        res.set_content("{\"status\":\"received\"}", "application/json");
    }

    void SSE::HandleOptionsRequest(const httplib::Request& req, httplib::Response& res) {
        SetCORSHeaders(res);
        res.status = 200;
    }

    void SSE::SetCORSHeaders(httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Access-Control-Max-Age", "86400");
    }

    std::pair<size_t, std::string> SSE::Read() {
        std::unique_lock<std::mutex> lock(incoming_mutex_);

        std::cout << "WAITING FOR LOCK TO BE RELEASED" << std::endl;

        std::cout << "incoming_messages_: " << incoming_messages_.empty() << std::endl;
        std::cout << "server_running: " << server_running_.load() << std::endl;
        incoming_cv_.wait(lock, [this]() {
            return !incoming_messages_.empty() || !server_running_.load();
        });

        std::cout << "LOCK RELEASED" << std::endl;

        if (!server_running_.load() && incoming_messages_.empty()) {
            return {0, ""};
        }

        if (!incoming_messages_.empty()) {
            std::string message = incoming_messages_.front();
            incoming_messages_.pop();
            return { message.length(), message };
        } else {
            return {0, ""};
        }
    }

    std::future<std::pair<size_t, std::string>> SSE::ReadAsync() {
        return std::async(std::launch::async, [this]() -> std::pair<size_t, std::string> {
            std::cout << "READ ASYNC CALLED!!!" << std::endl;
            return Read();
        });
    }

    void SSE::Write(const std::string& json_data) {

        std::cout << "RequestHandler delegated = " << json_data << std::endl;
        std::cout << "is_client_connected = " << client_connected_.load() << std::endl;
        if (!client_connected_.load()) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(outgoing_mutex_);
            outgoing_messages_.push(json_data);
        }

        outgoing_cv_.notify_one();
    }

    std::future<void> SSE::WriteAsync(const std::string& json_data) {
        return std::async(std::launch::async, [this, json_data] () {
            Write(json_data);
        });
    }

}