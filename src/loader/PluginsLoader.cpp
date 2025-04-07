//
// Created by giuse on 04 apr 2025.
//

#include "PluginsLoader.h"

namespace vx::mcp {

    PluginsLoader::PluginsLoader() = default;

    PluginsLoader::~PluginsLoader() {
        UnloadPlugins();
    }

    bool PluginsLoader::LoadPlugins(const std::string& directory) {
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    std::string extension = entry.path().extension().string();

                    // Check if this is a shared library
#ifdef _WIN32
                    if (extension == ".dll")
#else
                    if (extension == ".so")
#endif
                    {
                        LoadPlugin(entry.path().string());
                    }
                }
            }
            return true;
        } catch (const std::exception& ex) {
            LOG(ERROR) << "Error loading plugins: " << ex.what() << std::endl;
            return false;
        }
    }

    bool PluginsLoader::LoadPlugin(const std::string& path) {
        PluginEntry entry;
        entry.path = path;

        // Load the shared library
#ifdef _WIN32
        entry.handle = LoadLibraryA(path.c_str());
        if (!entry.handle) {
            DWORD error = GetLastError();
            char errorMsg[256] = {0};
            FormatMessageA(
                    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr,
                    error,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    errorMsg,
                    sizeof(errorMsg),
                    nullptr
            );
            LOG(ERROR) << "Failed to load plugin: " << path
                       << " - Error " << error << ": " << errorMsg << std::endl;
            return false;
        }
        // Get function pointers
        entry.createFunc = (PluginAPI * (*)())GetProcAddress(entry.handle, "CreatePlugin");
        entry.destroyFunc = (void (*)(PluginAPI *))GetProcAddress(entry.handle, "DestroyPlugin");
#else
        entry.handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!entry.handle) {
            LOG(ERROR) << "Failed to load plugin: " << path << " - " << dlerror() << std::endl;
            return false;
        }

        // Get function pointers
        entry.createFunc = (PluginAPI * (*)())dlsym(entry.handle, "CreatePlugin");
        entry.destroyFunc = (void (*)(PluginAPI *))dlsym(entry.handle, "DestroyPlugin");
#endif

        // Check if required functions were found
        if (!entry.createFunc || !entry.destroyFunc) {
            LOG(ERROR) << "Plugin does not export required functions: " << path << std::endl;

#ifdef _WIN32
            FreeLibrary(entry.handle);
#else
            dlclose(entry.handle);
#endif

            return false;
        }

        // Create plugin instance
        entry.instance = entry.createFunc();

        // Initialize the plugin
        if (!entry.instance->Initialize()) {
            LOG(ERROR) << "Plugin initialization failed: " << path << std::endl;
            entry.destroyFunc(entry.instance);

#ifdef _WIN32
            FreeLibrary(entry.handle);
#else
            dlclose(entry.handle);
#endif

            return false;
        }

        // Add to a plugin list
        m_plugins.push_back(entry);
        LOG(INFO) << "Loaded plugin: " << entry.instance->GetName()
                  << " v" << entry.instance->GetVersion() << std::endl;

        return true;
    }

    void PluginsLoader::UnloadPlugins() {
        for (auto& entry : m_plugins) {
            UnloadPlugin(entry);
        }
        m_plugins.clear();
    }

    void PluginsLoader::UnloadPlugin(PluginEntry& entry) {
        if (entry.instance) {
            // Shutdown the plugin
            entry.instance->Shutdown();

            // Destroy the plugin instance
            entry.destroyFunc(entry.instance);
            entry.instance = nullptr;
        }

        // Unload the library
        if (entry.handle) {
#ifdef _WIN32
            FreeLibrary(entry.handle);
#else
            dlclose(entry.handle);
#endif
            entry.handle = nullptr;
        }
    }

    const std::vector<PluginEntry>& PluginsLoader::GetPlugins() const {
        return m_plugins;
    }

}