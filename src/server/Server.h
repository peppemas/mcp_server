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

#ifndef MCP_SERVER_SERVER_H
#define MCP_SERVER_SERVER_H

#include <memory>
#include <queue>
#include <thread>
#include <condition_variable>
#include "ITransport.h"
#include "json.hpp"

using json = nlohmann::json;

#define MAX_PARSER_ERRORS 50

namespace vx::mcp {

    enum Capabilities {
        RESOURCES = 0 << 1,
        TOOLS = 0 << 2,
        PROMPTS = 0 << 3,
    };

    class Server {
    public:
        Server();
        ~Server();

        // Make Server non-copyable and non-movable for simplicity with threads/mutexes
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;

        bool Connect(const std::shared_ptr<ITransport>& transport);
        bool ConnectAsync(const std::shared_ptr<ITransport> &transport);

        void Stop();
        void StopAsync();

        inline bool IsValid() { return transport_ != nullptr; }
        inline void VerboseLevel(int level) { verboseLevel_ = level; }
        inline void Name(const std::string& name) { name_ = name; }
        bool OverrideCallback(const std::string &method, std::function<json(const json&)> function);
        void SendNotification(const std::string& pluginName, const char* notification);

    private:
        void WriterLoop();
        json HandleRequest(const json& request);

        json InitializeCmd(const json& request);
        json PingCmd(const json& request);
        json NotificationInitializedCmd(const json& request);
        json ToolsListCmd(const json& request);
        json ToolsCallCmd(const json& request);

        json ResourcesListCmd(const json& request);
        json ResourcesReadCmd(const json& request);
        json ResourcesSubscribeCmd(const json& request);
        json ResourcesUnsubscribeCmd(const json& request);
        json PromptsListCmd(const json& request);
        json PromptsGetCmd(const json& request);
        json LoggingSetLevelCmd(const json& request);
        json CompletionCompleteCmd(const json& request);
        json RootsListCmd(const json& request);

        json NotificationCancelledCmd(const json& request);
        json NotificationProgressCmd(const json& request);
        json NotificationRootsListChangedCmd(const json& request);
        json NotificationResourcesListChangedCmd(const json& request);
        json NotificationResourcesUpdatedCmd(const json& request);
        json NotificationPromptsListChangedCmd(const json& request);
        json NotificationToolsListChangedCmd(const json& request);
        json NotificationMessageCmd(const json& request);

    private:
        std::unordered_map<std::string, std::function<json(const json&)>> functionMap;

        bool isStopping_ = false;
        int verboseLevel_ = 0;
        int parserErrors_ = 0;
        std::string name_ = "mcp-server";

        std::shared_ptr<ITransport> transport_; // Store transport pointer
        std::queue<std::string> notification_queue_;
        std::mutex output_mutex_; // Protects both queue and transport writes
        std::condition_variable queue_cv_;
        std::thread writer_thread_;
        std::atomic<bool> writer_running_{false};

        std::thread reader_thread_;
        std::atomic<bool> reader_running_ = false;
    };

}

#endif //MCP_SERVER_SERVER_H
