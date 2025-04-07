//
// Created by giuse on 04 apr 2025.
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

typedef void (*ServerNotificationCallback)(const char* pluginName, const char* method, const char* params);

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
    ServerNotificationCallback SendToServer;
} NotificationSystem;

typedef struct {
    const char* (*GetName)();
    const char* (*GetVersion)();
    PluginType (*GetType)();
    int (*Initialize)();
    const char* (*HandleRequest)(const char* request);
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
