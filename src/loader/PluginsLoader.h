//
// Created by giuse on 04 apr 2025.
//

#ifndef MCP_SERVER_PLUGINS_LOADER_H
#define MCP_SERVER_PLUGINS_LOADER_H

#ifdef _WIN32
#include <windows.h>
typedef HMODULE LibraryHandle;
#else
#include <dlfcn.h>
    typedef void* LibraryHandle;
#endif
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <filesystem>
#include <algorithm>

#include "aixlog.hpp"
#include "PluginAPI.h"

namespace vx::mcp {

    struct PluginEntry {
        std::string path;
        LibraryHandle handle;
        PluginAPI* instance;

        // Function pointers
        PluginAPI* (*createFunc)();
        void (*destroyFunc)(PluginAPI*);
    };

    class PluginsLoader {
    public:
        PluginsLoader();
        ~PluginsLoader();

        // Load plugins from a directory
        bool LoadPlugins(const std::string& directory);

        // Unload all plugins
        void UnloadPlugins();

        // Get loaded plugins
        const std::vector<PluginEntry>& GetPlugins() const;

    private:
        bool LoadPlugin(const std::string& path);
        void UnloadPlugin(PluginEntry& entry);

    private:
        std::vector<PluginEntry> m_plugins;
    };

}

#endif //MCP_SERVER_PLUGINS_LOADER_H
