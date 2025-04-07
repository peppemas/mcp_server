//
// Created by giuse on 03 apr 2025.
//

#ifndef MCP_SERVER_SERVER_H
#define MCP_SERVER_SERVER_H

#include <memory>
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

    enum ErrorCode {
        ParseError = -32700,
        InvalidRequest = -32600,
        MethodNotFound = -32601,
        InvalidParams = -32602,
        InternalError = -32603
    };

    class Server {
    public:
        Server() = default;

        void Stop();
        bool Connect(const std::shared_ptr<ITransport>& transport);
        inline bool IsValid() { return true; }
        inline void VerboseLevel(int level) { m_verboseLevel = level; }
        inline void Name(const std::string& name) { m_name = name; }
        bool OverrideCallback(const std::string &method, std::function<json(const json&)> function);

    private:
        json HandleRequest(const json& request);
        json BuildRPCError(ErrorCode code, const std::string& id, const std::string &message);

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
        std::unordered_map<std::string, std::function<json(const json&)>> functionMap = {
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

        bool m_isStopping = false;
        int m_verboseLevel = 0;
        int m_parserErrors = 0;
        std::string m_name = "mcp-server";
    };

}

#endif //MCP_SERVER_SERVER_H
