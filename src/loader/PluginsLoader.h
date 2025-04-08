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
