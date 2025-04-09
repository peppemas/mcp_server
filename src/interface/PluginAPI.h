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

#ifndef MCP_SERVER_PLUGINAPI_H
#define MCP_SERVER_PLUGINAPI_H

#ifdef _WIN32
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ClientNotificationCallback)(const char* pluginName, const char* notification);

typedef enum {
    PLUGIN_TYPE_TOOLS = 0,
    PLUGIN_TYPE_PROMPTS = 1,
    PLUGIN_TYPE_RESOURCES = 2
} PluginType;

typedef struct {
    const char* name;
    const char* description;
    const char* inputSchema;  // JSON schema as a string
} PluginTool;

typedef struct {
    const char* name;
    const char* description;
    const char* arguments;  // JSON arguments as a string
} PluginPrompt;

typedef struct {
    const char* name;
    const char* description;
    const char* uri;
    const char* mime;
} PluginResource;

typedef struct {
    ClientNotificationCallback SendToClient;    // you should not touch this
} NotificationSystem;

typedef struct {
    const char* (*GetName)();
    const char* (*GetVersion)();
    PluginType (*GetType)();
    int (*Initialize)();
    char* (*HandleRequest)(const char* request);
    void (*Shutdown)();
    int (*GetToolCount)();
    const PluginTool* (*GetTool)(int index);
    int (*GetPromptCount)();
    const PluginPrompt* (*GetPrompt)(int index);
    int (*GetResourceCount)();
    const PluginResource* (*GetResource)(int index);
    NotificationSystem* notifications;
} PluginAPI;

PLUGIN_API PluginAPI* CreatePlugin();
PLUGIN_API void DestroyPlugin(PluginAPI*);

#ifdef __cplusplus
}
#endif

#endif //MCP_SERVER_PLUGINAPI_H
