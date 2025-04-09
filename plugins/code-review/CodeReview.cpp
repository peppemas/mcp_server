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
const char* GetVersionImpl() { return "1.0.0"; }
PluginType GetTypeImpl() { return PLUGIN_TYPE_PROMPTS; }

int InitializeImpl() {
    return 1;
}

char* HandleRequestImpl(const char* req) {
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
    char* buffer = new char[result.length() + 1];
#ifdef _WIN32
    strcpy_s(buffer, result.length() + 1, result.c_str());
#else
    strcpy(buffer, result.c_str());
#endif

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
        GetVersionImpl,
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
