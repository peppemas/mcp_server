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
#include <utility>
#include "Server.h"
#include "aixlog.hpp"
#include "version.h"

namespace vx::mcp {

    bool Server::Connect(const std::shared_ptr<ITransport> &transport) {
        while (!m_isStopping) {
            auto [length, json_string] = transport->Read();
            try {
                LOG(DEBUG) << "Received: " << json_string << std::endl;
                if (json_string.empty()) continue;
                json request = json::parse(json_string);
                m_parserErrors = 0; // reset parser error
                json response = HandleRequest(request);
                if (response != nullptr) {
                    transport->Write(response.dump());
                }
            } catch (json::parse_error &e) {
                // ok... what should we do in this case ? exit process ? does nothing ?
                // for now, we manage a max parser consecutive errors
                LOG(ERROR) << "Error parsing JSON: " << e.what() << std::endl;
                if (++m_parserErrors > MAX_PARSER_ERRORS) return false;
            }
        }
        return true;
    }

    void Server::Stop() {
        m_isStopping = true;
    }

    json Server::HandleRequest(const json &request) {
        // log the request
        if (m_verboseLevel == 1) {
            LOG(DEBUG) << "=== Request START ===" << std::endl;
            LOG(DEBUG) << request.dump(4) << std::endl;
            LOG(DEBUG) << "=== Request END ===" << std::endl;
        }

        // mandatory checks
        if (!request.contains("method")) {
            return BuildRPCError(InvalidRequest, request["id"], "Missing method");
        }

        // handle command
        std::string methodName = request["method"];
        auto it = functionMap.find(methodName);
        if (it != functionMap.end()) {
            json response = it->second(request);
            if (response != nullptr) {
                if (m_verboseLevel == 1) {
                    LOG(DEBUG) << "=== Response START ===" << std::endl;
                    LOG(DEBUG) << response.dump(4) << std::endl;
                    LOG(DEBUG) << "=== Response END ===" << std::endl;
                }
            }
            return response;
        }

        // handle method not found case
        int id = request["id"];
        return BuildRPCError(ErrorCode::MethodNotFound, std::to_string(id), "Method not found");
    }

    bool Server::OverrideCallback(const std::string &method, std::function<json(const json &)> function) {
        if (functionMap.find(method) != functionMap.end()) {
            functionMap[method] = std::move(function);
            return true;
        }
        return false;
    }

    json Server::BuildRPCError(ErrorCode code, const std::string& id, const std::string &message) {
        return {
                {"jsonrpc", "2.0"},
                {"error", {{"code", code}, {"message", message}}},
                {"id", id}
        };
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
        response["result"]["serverInfo"]["name"] = m_name;
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
        return BuildRPCError(ErrorCode::MethodNotFound, request["id"], "Method not found");
    }

    json Server::ResourcesUnsubscribeCmd(const json &request) {
        LOG(WARNING) << "ResourcesUnsubscribeCmd called but NOT YET IMPLEMENTED" << std::endl;
        return BuildRPCError(ErrorCode::MethodNotFound, request["id"], "Method not found");
    }

    json Server::PromptsListCmd(const json &request) {
        nlohmann::ordered_json response;
        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
        response["result"]["prompts"] = json::array();
        return response;
    }

    json Server::PromptsGetCmd(const json &request) {
        return BuildRPCError(ErrorCode::MethodNotFound, request["id"], "Method not found");
    }

    json Server::LoggingSetLevelCmd(const json &request) {
        return BuildRPCError(ErrorCode::MethodNotFound, request["id"], "Method not found");
    }

    json Server::CompletionCompleteCmd(const json &request) {
        return BuildRPCError(ErrorCode::MethodNotFound, request["id"], "Method not found");
    }

    json Server::RootsListCmd(const json &request) {
        return BuildRPCError(ErrorCode::MethodNotFound, request["id"], "Method not found");
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

}
