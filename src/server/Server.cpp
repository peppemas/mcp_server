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

#include <iostream>
#include <utility>
#include "Server.h"
#include "aixlog.hpp"
#include "version.h"
#include "../utils/MCPBuilder.h"

namespace vx::mcp {

    Server::Server() {
        functionMap = {
                {"initialize", [this](const json& req) { return this->InitializeCmd(req); }},
                {"ping", [this](const json& req) { return this->PingCmd(req); }},
                {"resources/list", [this](const json& req) { return this->ResourcesListCmd(req); }},
                {"resources/read", [this](const json& req) { return this->ResourcesReadCmd(req); }},
                {"tools/list", [this](const json& req) { return this->ToolsListCmd(req); }},
                {"tools/call", [this](const json& req) { return this->ToolsCallCmd(req); }},
                {"resources/subscribe", [this](const json& req) { return this->ResourcesSubscribeCmd(req); }},
                {"resources/unsubscribe", [this](const json& req) { return this->ResourcesUnsubscribeCmd(req); }},
                {"prompts/list", [this](const json& req) { return this->PromptsListCmd(req); }},
                {"prompts/get", [this](const json& req) { return this->PromptsGetCmd(req); }},
                {"logging/setLevel", [this](const json& req) { return this->LoggingSetLevelCmd(req); }},
                {"completion/complete", [this](const json& req) { return this->CompletionCompleteCmd(req); }},
                {"roots/list", [this](const json& req) { return this->RootsListCmd(req); }},
                {"notifications/initialized", [this](const json& req) { return this->NotificationInitializedCmd(req); }},
                {"notifications/cancelled", [this](const json& req) { return this->NotificationCancelledCmd(req); }},
                {"notifications/progress", [this](const json& req) { return this->NotificationProgressCmd(req); }},
                {"notifications/roots/list_changed", [this](const json& req) { return this->NotificationRootsListChangedCmd(req); }},
                {"notifications/resources/list_changed", [this](const json& req) { return this->NotificationResourcesListChangedCmd(req); }},
                {"notifications/resources/updated", [this](const json& req) { return this->NotificationResourcesUpdatedCmd(req); }},
                {"notifications/prompts/list_changed", [this](const json& req) { return this->NotificationPromptsListChangedCmd(req); }},
                {"notifications/tools/list_changed", [this](const json& req) { return this->NotificationToolsListChangedCmd(req); }},
                {"notifications/message", [this](const json& req) { return this->NotificationMessageCmd(req); }}
        };
    }

    Server::~Server() {
        Stop();
    }

    void Server::WriterLoop() {
        LOG(INFO) << "Writer thread started." << std::endl;
        while (writer_running_.load()) {
            std::string notification_to_send;
            {
                std::unique_lock<std::mutex> lock(output_mutex_);
                // Wait until queue is not empty OR the writer should stop
                queue_cv_.wait(lock, [this] { return !notification_queue_.empty() || !writer_running_.load(); });

                // Check running flag again after waking up
                if (!writer_running_.load() && notification_queue_.empty()) {
                    break; // Exit loop if stopped and queue is empty
                }

                if (!notification_queue_.empty()) {
                    notification_to_send = std::move(notification_queue_.front());
                    notification_queue_.pop();
                }
            } // Release lock before potentially blocking write

            if (!notification_to_send.empty() && transport_) {
                try {
                    // Note: Write itself is not locked here, assuming transport handles internal sync
                    // If transport->Write is not thread-safe, the lock needs to span this call too.
                    // For stdio, writing from one thread should be okay, but locking provides safety.
                    // Re-locking here for safety with potential other writes (responses).
                    std::lock_guard<std::mutex> write_lock(output_mutex_);
                    if (transport_) { // Check transport again after potential delay
                        LOG(DEBUG) << "Sending Notification: " << notification_to_send << std::endl;
                        transport_->Write(notification_to_send);
                    }
                } catch (const std::exception& e) {
                    LOG(ERROR) << "Error writing notification: " << e.what() << std::endl;
                    // Decide how to handle write errors (e.g., log, ignore, stop?)
                }
            }
            // Small sleep to prevent tight loop if errors occur rapidly
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        LOG(INFO) << "Writer thread stopped." << std::endl;
    }

    bool Server::Connect(const std::shared_ptr<ITransport> &transport) {
        if (!transport) {
            LOG(ERROR) << "Connect called with null transport." << std::endl;
            return false;
        }

        transport_ = transport; // Store the transport pointer
        isStopping_ = false; // Reset stopping flag

        // Start the writer thread
        writer_running_ = true;
        writer_thread_ = std::thread(&Server::WriterLoop, this);

        while (!isStopping_) {
            auto [length, json_string] = transport->Read();
            if (isStopping_) break;

            if (length == 0 && json_string.empty()) {
                LOG(INFO) << "Read returned empty data, potentially client disconnected." << std::endl;
                isStopping_ = true;
                break;
            }

            try {
                if (json_string.empty()) continue;
                LOG(DEBUG) << "Received: " << json_string << std::endl;
                json request = json::parse(json_string);
                parserErrors_ = 0; // reset parser error
                json response = HandleRequest(request);
                if (response != nullptr) {
                    std::lock_guard<std::mutex> lock(output_mutex_);
                    LOG(DEBUG) << "Sending Response: " << response.dump() << std::endl;
                    transport_->Write(response.dump());
                }
            } catch (json::parse_error &e) {
                // ok... what should we do in this case ? exit process ? does nothing ?
                // for now, we manage a max parser consecutive errors
                LOG(ERROR) << "Error parsing JSON: " << e.what() << std::endl;
                if (++parserErrors_ > MAX_PARSER_ERRORS) return false;
            }
        }

        Stop();

        return true;
    }

    bool Server::ConnectAsync(const std::shared_ptr<ITransport> &transport) {
        if (!transport) {
            LOG(ERROR) << "ConnectAsync called with null transport." << std::endl;
            return false;
        }

        transport_ = transport;
        isStopping_ = false;

        // Start the writer thread
        writer_running_ = true;
        writer_thread_ = std::thread(&Server::WriterLoop, this);

        // Start the async reader thread
        reader_running_ = true;
        reader_thread_ = std::thread([this]() {
            LOG(INFO) << "Async Reader thread started." << std::endl;
            while (reader_running_ && !isStopping_) {
                try {
                    auto future = transport_->ReadAsync();
                    auto [length, json_string] = future.get();

                    if (isStopping_ || (length == 0 && json_string.empty())) {
                        LOG(INFO) << "Empty message or stopping. Reader exiting.";
                        break;
                    }

                    if (!json_string.empty()) {
                        LOG(DEBUG) << "Received: " << json_string << std::endl;
                        json request = json::parse(json_string);
                        parserErrors_ = 0;

                        json response = HandleRequest(request);
                        if (response != nullptr) {
                            std::lock_guard<std::mutex> lock(output_mutex_);
                            LOG(DEBUG) << "Sending Response: " << response.dump() << std::endl;
                            transport_->Write(response.dump());
                        }
                    }
                } catch (json::parse_error &e) {
                    LOG(ERROR) << "Error parsing JSON: " << e.what() << std::endl;
                    if (++parserErrors_ > MAX_PARSER_ERRORS) {
                        isStopping_ = true;
                        break;
                    }
                } catch (const std::exception &e) {
                    LOG(ERROR) << "Reader thread exception: " << e.what() << std::endl;
                    isStopping_ = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            LOG(INFO) << "Async Reader thread exiting." << std::endl;
        });

        return true;
    }

    void Server::Stop() {
        if (isStopping_) return; // Avoid redundant stopping

        isStopping_ = true;
        LOG(INFO) << "Stopping server..." << std::endl;

        // Signal and join writer thread
        writer_running_ = false;
        queue_cv_.notify_one(); // Wake up the writer thread if waiting
        if (writer_thread_.joinable()) {
            writer_thread_.join();
            LOG(INFO) << "Writer thread joined." << std::endl;
        }
        LOG(INFO) << "Server stopped." << std::endl;
    }

    void Server::SendNotification(const std::string& pluginName, const char* notification) {
        if (isStopping_) {
            LOG(WARNING) << pluginName << " attempted to send notification while server stopping." << std::endl;
            return;
        }

        // Add notification to the queue (protected by the mutex)
        {
            std::lock_guard<std::mutex> lock(output_mutex_);
            notification_queue_.emplace(notification);
        }
        queue_cv_.notify_one(); // Notify the writer thread
    }

    json Server::HandleRequest(const json &request) {
        // log the request
        if (verboseLevel_ == 1) {
            LOG(DEBUG) << "=== Request START ===" << std::endl;
            LOG(DEBUG) << request.dump(4) << std::endl;
            LOG(DEBUG) << "=== Request END ===" << std::endl;
        }

        // mandatory checks
        if (!request.contains("method")) {
            return MCPBuilder::Error(MCPBuilder::InvalidRequest, request["id"], "Missing method");
        }

        // handle command
        std::string methodName = request["method"];
        auto it = functionMap.find(methodName);
        if (it != functionMap.end()) {
            json response = it->second(request);
            if (response != nullptr) {
                if (verboseLevel_ == 1) {
                    LOG(DEBUG) << "=== Response START ===" << std::endl;
                    LOG(DEBUG) << response.dump(4) << std::endl;
                    LOG(DEBUG) << "=== Response END ===" << std::endl;
                }
            }
            return response;
        }

        // handle method not found case
        int id = request["id"];
        return MCPBuilder::Error(MCPBuilder::MethodNotFound, std::to_string(id), "Method not found");
    }

    bool Server::OverrideCallback(const std::string &method, std::function<json(const json &)> function) {
        if (functionMap.find(method) != functionMap.end()) {
            functionMap[method] = std::move(function);
            return true;
        }
        return false;
    }

    json Server::InitializeCmd(const json &request) {
        LOG(INFO) << "InitializeCommand" << std::endl;
        if (request.contains("params")) {
            json params = request["params"];

            // Access rootUri
            if (params.contains("rootUri")) {
                std::string rootUri = params["rootUri"].get<std::string>();
                LOG(INFO) << "rootUri: " << rootUri << std::endl;
            }

            // Access rootPath (deprecated)
            if (params.contains("rootPath")) {
                std::string rootPath = params["rootPath"].get<std::string>();
                LOG(INFO) << "rootPath: " << rootPath << std::endl;
            }

            // Access initializationOptions
            if (params.contains("initializationOptions")) {
                json initializationOptions = params["initializationOptions"];
                // Access specific initialization options as needed
                LOG(INFO) << "initializationOptions: " << initializationOptions.dump() << std::endl;
            }

            // Access capabilities
            if (params.contains("capabilities")) {
                json capabilities = params["capabilities"];

                // Access workspace capabilities
                if (capabilities.contains("workspace") && capabilities["workspace"].contains("workspaceFolders")) {
                    bool workspaceFolders = capabilities["workspace"]["workspaceFolders"].get<bool>();
                    LOG(INFO) << "workspaceFolders: " << workspaceFolders << std::endl;
                }

                // Access textDocument capabilities
                if (capabilities.contains("textDocument") && capabilities["textDocument"].contains("synchronization")) {
                    json synchronization = capabilities["textDocument"]["synchronization"];
                    if (synchronization.contains("didChange") && synchronization["didChange"].contains("synchronizationKind")){
                        int synchronizationKind = synchronization["didChange"]["synchronizationKind"].get<int>();
                        LOG(INFO) << "synchronizationKind: " << synchronizationKind << std::endl;
                    }
                }

                // Access completion capabilities
                if (capabilities.contains("textDocument") && capabilities["textDocument"].contains("completion") && capabilities["textDocument"]["completion"].contains("completionItem")) {
                    json completionItem = capabilities["textDocument"]["completion"]["completionItem"];
                    if (completionItem.contains("snippetSupport")){
                        bool snippetSupport = completionItem["snippetSupport"].get<bool>();
                        LOG(INFO) << "snippetSupport: " << snippetSupport << std::endl;
                    }
                }
            }

            // Access trace
            if (params.contains("trace")) {
                std::string trace = params["trace"].get<std::string>();
                LOG(INFO) << "trace: " << trace << std::endl;
            }

            // Access workspaceFolders array
            if (params.contains("workspaceFolders")) {
                json workspaceFoldersArray = params["workspaceFolders"];
                for (const auto& folder : workspaceFoldersArray) {
                    std::string uri = folder["uri"].get<std::string>();
                    std::string name = folder["name"].get<std::string>();
                    LOG(INFO) << "workspaceFolder uri: " << uri << " name: " << name << std::endl;
                }
            }
        }
        nlohmann::ordered_json response = {};
        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
        response["result"]["protocolVersion"] = request["params"]["protocolVersion"];
        /// TODO: the "listChanged" parameter actually broke the Claude Desktop
        /// should we check the the protocol version ???
        response["result"]["capabilities"]["tools"] = json::object();
        response["result"]["capabilities"]["prompts"] = json::object();
        response["result"]["capabilities"]["resources"]["subscribe"] = true;
        response["result"]["capabilities"]["logging"] = json::object();
        response["result"]["serverInfo"]["name"] = name_;
        response["result"]["serverInfo"]["version"] = PROJECT_VERSION;
        return response;
    }

    json Server::PingCmd(const json &request) {
        nlohmann::ordered_json response = {};
        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
        response["result"] = json::object();
        return response;
    }

    json Server::ResourcesListCmd(const json &request) {
        nlohmann::ordered_json response;
        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
        response["result"]["resources"] = json::array();
        return response;
    }

    json Server::ResourcesReadCmd(const json &request) {
        return json();
    }

    json Server::ToolsListCmd(const json &request) {
        nlohmann::ordered_json response;
        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
        response["result"]["tools"] = json::array();
        return response;
    }

    json Server::ToolsCallCmd(const json &request) {
        LOG(DEBUG) << "ToolsCallCmd called" << std::endl;
        nlohmann::ordered_json response;
        nlohmann::ordered_json defaultTextContent;

        defaultTextContent["type"] = "text";
        defaultTextContent["text"] = "you should override this method in your plugin.";

        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
        response["result"]["content"] = json::array();
        response["result"]["content"].push_back(defaultTextContent);
        response["result"]["isError"] = true;

        return response;
    }

    json Server::ResourcesSubscribeCmd(const json &request) {
        LOG(WARNING) << "ResourcesSubscribeCmd called but NOT YET IMPLEMENTED" << std::endl;
        return MCPBuilder::Error(MCPBuilder::MethodNotFound, request["id"], "Method not found");
    }

    json Server::ResourcesUnsubscribeCmd(const json &request) {
        LOG(WARNING) << "ResourcesUnsubscribeCmd called but NOT YET IMPLEMENTED" << std::endl;
        return MCPBuilder::Error(MCPBuilder::MethodNotFound, request["id"], "Method not found");
    }

    json Server::PromptsListCmd(const json &request) {
        nlohmann::ordered_json response;
        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
        response["result"]["prompts"] = json::array();
        return response;
    }

    json Server::PromptsGetCmd(const json &request) {
        return MCPBuilder::Error(MCPBuilder::MethodNotFound, request["id"], "Method not found");
    }

    json Server::LoggingSetLevelCmd(const json &request) {
        return MCPBuilder::Error(MCPBuilder::MethodNotFound, request["id"], "Method not found");
    }

    json Server::CompletionCompleteCmd(const json &request) {
        return MCPBuilder::Error(MCPBuilder::MethodNotFound, request["id"], "Method not found");
    }

    json Server::RootsListCmd(const json &request) {
        return MCPBuilder::Error(MCPBuilder::MethodNotFound, request["id"], "Method not found");
    }

    json Server::NotificationInitializedCmd(const json &request) {
        return nullptr;
    }

    json Server::NotificationCancelledCmd(const json &request) {
        return nullptr;
    }

    json Server::NotificationProgressCmd(const json &request) {
        return nullptr;
    }

    json Server::NotificationRootsListChangedCmd(const json &request) {
        return nullptr;
    }

    json Server::NotificationResourcesListChangedCmd(const json &request) {
        return nullptr;
    }

    json Server::NotificationResourcesUpdatedCmd(const json &request) {
        return nullptr;
    }

    json Server::NotificationPromptsListChangedCmd(const json &request) {
        return nullptr;
    }

    json Server::NotificationToolsListChangedCmd(const json &request) {
        return nullptr;
    }

    json Server::NotificationMessageCmd(const json &request) {
        return nullptr;
    }

    void Server::StopAsync() {
        if (isStopping_) return;

        isStopping_ = true;
        LOG(INFO) << "Stopping async server..." << std::endl;

        // Stop writer thread
        writer_running_ = false;
        queue_cv_.notify_one();
        if (writer_thread_.joinable()) {
            writer_thread_.join();
            LOG(INFO) << "Writer thread joined." << std::endl;
        }

        // Stop reader thread
        reader_running_ = false;
        if (reader_thread_.joinable()) {
            reader_thread_.join();
            LOG(INFO) << "Reader thread joined." << std::endl;
        }

        LOG(INFO) << "Async server stopped." << std::endl;
    }
}
