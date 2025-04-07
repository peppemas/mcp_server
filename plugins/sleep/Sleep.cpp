//
// Created by giuse on 05 apr 2025.
//

#include <thread>
#include "PluginAPI.h"
#include "json.hpp"

using json = nlohmann::json;

static PluginTool methods[] = {
        {
            "sleep",
            "Pauses execution for the specified number of milliseconds.",
        R"({
            "$schema": "http://json-schema.org/draft-07/schema#",
            "type": "object",
            "properties": {
                "milliseconds": { "type": "number", "minimum": 0, "description": "Number of milliseconds to sleep." }
            },
            "required": ["milliseconds"],
            "additionalProperties": false
        })"
        }
};

const char* GetNameImpl() { return "sleep-tools"; }
const char* GetPluginImpl() { return "1.0.0"; }
PluginType GetTypeImpl() { return PLUGIN_TYPE_TOOLS; }

int InitializeImpl() {
    return 1;
}

const char* HandleRequestImpl(const char* req) {
    static char buffer[10240];

    auto request = json::parse(req);

    auto milliseconds = request["params"]["arguments"]["milliseconds"].get<int>();
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));

    nlohmann::json responseContent;
    responseContent["type"] = "text";
    responseContent["text"] = "Waited for " + std::to_string(milliseconds) + " milliseconds";

    nlohmann::json response;
    response["content"] = json::array();
    response["content"].push_back(responseContent);
    response["isError"] = false;

    std::string result = response.dump();
    strncpy(buffer, result.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0'; // Ensure null termination

    return buffer;
}

void ShutdownImpl() {
}

int GetToolCountImpl() {
    return sizeof(methods) / sizeof(methods[0]);
}

const PluginTool* GetToolImpl(int index) {
    if (index < 0 || index >= GetToolCountImpl()) return nullptr;
    return &methods[index];
}

static PluginAPI plugin = {
        GetNameImpl,
        GetPluginImpl,
        GetTypeImpl,
        InitializeImpl,
        HandleRequestImpl,
        ShutdownImpl,
        GetToolCountImpl,
        GetToolImpl,
        nullptr,
        nullptr,
        nullptr,
        nullptr
};

extern "C" PLUGIN_API PluginAPI* CreatePlugin() {
    return &plugin;
}

extern "C" PLUGIN_API void DestroyPlugin(PluginAPI*) {
    // Nothing to clean up for this example
}
