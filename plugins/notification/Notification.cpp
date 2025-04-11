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

// ***** THIS PLUGIN IS IN THE DEVELOPMENT STAGE COULD NOT WORK... *****

#include <thread>
#include "PluginAPI.h"
#include "json.hpp"
#include "../../src/utils/MCPBuilder.h"

using json = nlohmann::json;

static PluginAPI* g_plugin = nullptr;

static PluginTool methods[] = {
        {
            "progress_test",
            "Execute a long running process and inform the client about the progress",
        R"({
            "$schema": "http://json-schema.org/draft-07/schema#",
            "type": "object",
            "properties": {},
            "required": [],
            "additionalProperties": false
        })"
        },
        {
            "logging_test",
            "Execute a logging test. Send a message from server to the client",
        R"({
            "$schema": "http://json-schema.org/draft-07/schema#",
            "type": "object",
            "properties": {},
            "required": [],
            "additionalProperties": false
        })"
        }
};

const char* GetNameImpl() { return "notification-tools"; }
const char* GetVersionImpl() { return "1.0.0"; }
PluginType GetTypeImpl() { return PLUGIN_TYPE_TOOLS; }

int InitializeImpl() {
    return 1;
}

char* HandleRequestImpl(const char* req) {
    auto request = json::parse(req);

    if (request["params"]["name"] == "logging_test") {
        if (g_plugin) {
            std::string message = MCPBuilder::NotificationLog("notice","****** THIS IS A LOGGING TEST!").dump();
            g_plugin->notifications->SendToClient(GetNameImpl(), message.c_str());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else if (request["params"]["name"] == "progress_test") {
        // Total duration in seconds
        const int totalDuration = 10;

        if (!request["params"].contains("_meta") || !request["params"]["_meta"].contains("progressToken")) {
            nlohmann::json errorResponse;
            errorResponse["content"] = json::array();
            errorResponse["content"].push_back(MCPBuilder::TextContent("Missing required parameter: progressToken."));
            errorResponse["isError"] = true;

            std::string errorResult = errorResponse.dump();
            char *errorBuffer = new char[errorResult.length() + 1];
#ifdef _WIN32
            strcpy_s(errorBuffer, errorResult.length() + 1, errorResult.c_str());
#else
            strcpy(errorBuffer, errorResult.c_str());
#endif
            return errorBuffer;
        }
        std::string progressToken = request["params"]["_meta"]["progressToken"].get<std::string>();
        

        if (g_plugin) {
            for (int i = 1; i <= totalDuration; i++) {
                // Wait for 1 second
                std::this_thread::sleep_for(std::chrono::seconds(1));

                // Calculate progress percentage
                int progressPercent = (i * 100) / totalDuration;

                // Create a progress notification
                std::string progressMessage = "Progress: " + std::to_string(progressPercent) + "%";
                std::string message = MCPBuilder::NotificationProgress(progressMessage, progressToken, progressPercent, 100).dump();

                // Send notification to client
                g_plugin->notifications->SendToClient(GetNameImpl(), message.c_str());
            }
        }
    }

    nlohmann::json responseContent = MCPBuilder::TextContent("test completed.");

    nlohmann::json response;
    response["content"] = json::array();
    response["content"].push_back(responseContent);
    response["isError"] = false;

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

int GetToolCountImpl() {
    return sizeof(methods) / sizeof(methods[0]);
}

const PluginTool* GetToolImpl(int index) {
    if (index < 0 || index >= GetToolCountImpl()) return nullptr;
    return &methods[index];
}

static PluginAPI plugin = {
        GetNameImpl,
        GetVersionImpl,
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
    g_plugin = &plugin;
    return &plugin;
}

extern "C" PLUGIN_API void DestroyPlugin(PluginAPI*) {
    // Nothing to clean up for this example
}
