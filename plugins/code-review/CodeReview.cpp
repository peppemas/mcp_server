//
// Created by giuse on 06 apr 2025.
//

#include <thread>
#include <iostream>
#include "PluginAPI.h"
#include "json.hpp"

using json = nlohmann::json;

static PluginPrompt prompts[] = {
        {
            "code-review",
            "Asks the LLM to analyze code quality and suggest improvements",
        R"([{
            "name" : "language",
            "description" : "The programming language of the code",
            "required": true
        }])"
        }
};

const char* GetNameImpl() { return "code-review"; }
const char* GetPluginImpl() { return "1.0.0"; }
PluginType GetTypeImpl() { return PLUGIN_TYPE_PROMPTS; }

int InitializeImpl() {
    return 1;
}

const char* HandleRequestImpl(const char* req) {
    static char buffer[10240];

    auto request = json::parse(req);
    auto language = request["params"]["arguments"]["language"].get<std::string>();

    nlohmann::json response = json::object();
    nlohmann::json messages = json::array();
    messages.push_back(json::object({
            {"role", "user"},
            {"content", json::object({
                    {"type", "text"},
                    {"text", "Please analyze code quality and suggest improvements of this code written in " + language}
            })}}));
    response["description"] = "this is the code review prompt";
    response["messages"] = messages;

    std::string result = response.dump();
    strncpy(buffer, result.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0'; // Ensure null termination

    return buffer;
}

void ShutdownImpl() {
}

int GetPromptCountImpl() {
    return sizeof(prompts) / sizeof(prompts[0]);
}

const PluginPrompt* GetPromptImpl(int index) {
    if (index < 0 || index >= GetPromptCountImpl()) return nullptr;
    return &prompts[index];
}

static PluginAPI plugin = {
        GetNameImpl,
        GetPluginImpl,
        GetTypeImpl,
        InitializeImpl,
        HandleRequestImpl,
        ShutdownImpl,
        nullptr,
        nullptr,
        GetPromptCountImpl,
        GetPromptImpl,
        nullptr,
        nullptr
};

extern "C" PLUGIN_API PluginAPI* CreatePlugin() {
    return &plugin;
}

extern "C" PLUGIN_API void DestroyPlugin(PluginAPI*) {
    // Nothing to clean up for this example
}
