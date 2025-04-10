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

#include "version.h"
#include "httplib.h"
#include "popl.hpp"
#include "StdioTransport.h"
#include "server/Server.h"
#include "aixlog.hpp"
#include "loader/PluginsLoader.h"
#include "json.hpp"
#include "utils/MCPBuilder.h"

using namespace popl;

std::shared_ptr<vx::mcp::Server> server;

struct NotificationState {
    std::mutex serverNotificationMutex;
};
NotificationState notificationState;

/// stop handler Ctrl+C
void stop_handler(sig_atomic_t s) {
    std::cout <<"Stopping server..." << std::endl;
    if (server && server->IsValid()) {
        server->Stop();
    }
    std::cout << "done." << std::endl;
    exit(0);
}

/// Notification Implementation from plugins to mcp-client
void ClientNotificationCallbackImpl(const char* pluginName, const char* notification) {
    std::lock_guard<std::mutex> lock(notificationState.serverNotificationMutex);
    if (server && server->IsValid()) {
        server->SendNotification(pluginName, notification);
    }
}

/// main entry point
int main(int argc, char **argv) {
    std::string name;
    std::string plugins_directory;
    std::string logs_directory;
    bool verbose;

    auto transport = std::make_shared<vx::transport::Stdio>();
    auto loader = std::make_shared<vx::mcp::PluginsLoader>();
    server = std::make_shared<vx::mcp::Server>();

    //============================================================================================
    // setup signal handler (Ctrl+C)
    //============================================================================================
    signal(SIGINT, stop_handler);

    //============================================================================================
    // setup command line options
    //============================================================================================
    OptionParser op("Allowed options");
    auto help_option = op.add<Switch>("", "help", "produce help message");
    auto name_option = op.add<Value<std::string>>("n", "name", "the name of the server", "mcp-server");
    auto plugins_directory_option = op.add<Value<std::string>>("p", "plugins", "the directory where to load the plugins", "./plugins");
    auto logs_directory_option = op.add<Value<std::string>>("l", "logs", "the directory where to store the logs", "./logs");
    auto verbose_option = op.add<Value<bool>>("v", "verbose", "enable verbose", verbose);
    name_option->assign_to(&name);
    plugins_directory_option->assign_to(&plugins_directory);
    logs_directory_option->assign_to(&logs_directory);
    verbose_option->assign_to(&verbose);

    //============================================================================================
    // parse options
    //============================================================================================
    try {
        op.parse(argc, argv);
        if (help_option->count() == 1) {
            std::cout << op << std::endl;
            return 0;
        }
    } catch (const popl::invalid_option& e) {
        std::cerr << "Invalid Option Exception: " << e.what() << std::endl;
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }

    //============================================================================================
    // setup logger
    //============================================================================================
    // Get the current time as ISO 8601 string
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H-%M-%S");
    std::string iso_date = ss.str();

    // Concatenate ISO date to logname
    std::string logFilename = logs_directory + "/mcp-server_" + iso_date + ".log";
    auto sink_file = std::make_shared<AixLog::SinkFile>(AixLog::Severity::trace, logFilename);
    AixLog::Log::init({sink_file});

    //============================================================================================
    // print logo and info
    //============================================================================================
    LOG(INFO) << " __  __  _____ _____        _____ ______ _______      ________ _____  " << std::endl;
    LOG(INFO) << "|  \\/  |/ ____|  __ \\      / ____|  ____|  __ \\ \\    / /  ____|  __ \\ " << std::endl;
    LOG(INFO) << "| \\  / | |    | |__) |____| (___ | |__  | |__) \\ \\  / /| |__  | |__) |" << std::endl;
    LOG(INFO) << "| |\\/| | |    |  ___/______\\___ \\|  __| |  _  / \\ \\/ / |  __| |  _  / " << std::endl;
    LOG(INFO) << "| |  | | |____| |          ____) | |____| | \\ \\  \\  /  | |____| | \\ \\ " << std::endl;
    LOG(INFO) << "|_|  |_|\\_____|_|         |_____/|______|_|  \\_\\  \\/   |______|_|  \\_\\" << std::endl;
    LOG(INFO) << "Starting mcp-server v" << PROJECT_VERSION << " (transport: " << transport->GetName() << " v" << transport->GetVersion() << ") on port: " << transport->GetPort() << std::endl;
    LOG(INFO) << "Press Ctrl+C to exit." << std::endl;

    //============================================================================================
    // load all plugins from the plugins directory
    //============================================================================================
    if (loader->LoadPlugins(plugins_directory)) {
        LOG(INFO) << "Successfully loaded plugins" << std::endl;
    }

    //============================================================================================
    // enable notification system
    //============================================================================================
    for (auto& plugin : loader->GetPlugins()) {
        plugin.instance->notifications = new NotificationSystem();
        plugin.instance->notifications->SendToClient = ClientNotificationCallbackImpl;
    }

    //============================================================================================
    // start server
    //============================================================================================
    server->Name(name);
    server->VerboseLevel(verbose ? 1 : 0);
    server->OverrideCallback("tools/list", [&loader](const json& request) {
        nlohmann::ordered_json response = MCPBuilder::Response(request);
        response["result"]["tools"] = json::array();

        for (const auto& plugin : loader->GetPlugins()) {
            if (plugin.instance->GetType() == PLUGIN_TYPE_TOOLS) {
                for (int i = 0; i < plugin.instance->GetToolCount(); i++) {
                    nlohmann::ordered_json tool;
                    auto pluginTool = plugin.instance->GetTool(i);
                    tool["name"] = pluginTool->name;
                    tool["description"] = pluginTool->description;
                    tool["inputSchema"] = nlohmann::json::parse(pluginTool->inputSchema);
                    response["result"]["tools"].push_back(tool);
                }
            }
        }

        return response;
    });
    server->OverrideCallback("tools/call", [&loader](const json& request) {
        nlohmann::ordered_json response = MCPBuilder::Response(request);

        char* res_ptr = nullptr;

        for (const auto& plugin : loader->GetPlugins()) {
            if (plugin.instance->GetType() == PLUGIN_TYPE_TOOLS) {
                for (int i = 0; i < plugin.instance->GetToolCount(); i++) {
                    auto pluginTool = plugin.instance->GetTool(i);
                    if (pluginTool->name == request["params"]["name"]) {
                        res_ptr = plugin.instance->HandleRequest(request.dump().c_str());
                        if (res_ptr) {
                            try {
                                response["result"] = json::parse(res_ptr);
                                response["result"]["isError"] = false;
                            } catch (const json::parse_error& e) {
                                response["result"]["isError"] = true;
                                response["result"]["content"] = json::array();
                                response["result"]["content"].push_back({{"type", "text"}, {"text", "Plugin returned malformed data."}});
                            }
                            // --- Free the allocated memory ---
                            delete[] res_ptr;
                        } else {
                            LOG(ERROR) << "Plugin " << pluginTool->name << " returned nullptr." << std::endl;
                        }
                        return response;
                    }
                }
            }
        }

        return response;
    });
    server->OverrideCallback("prompts/list", [&loader](const json& request) {
        nlohmann::ordered_json response = MCPBuilder::Response(request);
        response["result"]["prompts"] = json::array();

        for (const auto& plugin : loader->GetPlugins()) {
            if (plugin.instance->GetType() == PLUGIN_TYPE_PROMPTS) {
                for (int i = 0; i < plugin.instance->GetPromptCount(); i++) {
                    nlohmann::ordered_json prompt;
                    auto pluginPrompt = plugin.instance->GetPrompt(i);
                    prompt["name"] = pluginPrompt->name;
                    prompt["description"] = pluginPrompt->description;
                    prompt["arguments"] = nlohmann::json::parse(pluginPrompt->arguments);
                    response["result"]["prompts"].push_back(prompt);
                }
            }
        }

        return response;
    });
    server->OverrideCallback("prompts/get", [&loader](const json& request) {
        nlohmann::ordered_json response = MCPBuilder::Response(request);

        char* res_ptr = nullptr;

        for (const auto& plugin : loader->GetPlugins()) {
            if (plugin.instance->GetType() == PLUGIN_TYPE_PROMPTS) {
                for (int i = 0; i < plugin.instance->GetPromptCount(); i++) {
                    auto pluginPrompt = plugin.instance->GetPrompt(i);
                    if (pluginPrompt->name == request["params"]["name"]) {
                        res_ptr = plugin.instance->HandleRequest(request.dump().c_str());
                        if (res_ptr) {
                            try {
                                response["result"] = json::parse(res_ptr);
                            } catch (const json::parse_error& e) {
                                LOG(ERROR) << "Plugin " << pluginPrompt->name << " returned malformed data." << std::endl;
                                // TODO: how can we handle error here ?
                            }
                            // --- Free the allocated memory ---
                            delete[] res_ptr;
                        }
                    }
                    return response;
                }
            }
        }

        return response;
    });
    server->OverrideCallback("resources/list", [&loader](const json& request) {
        nlohmann::ordered_json response = MCPBuilder::Response(request);
        response["result"]["resources"] = json::array();

        for (const auto& plugin : loader->GetPlugins()) {
            if (plugin.instance->GetType() == PLUGIN_TYPE_RESOURCES) {
                for (int i = 0; i < plugin.instance->GetResourceCount(); i++) {
                    nlohmann::ordered_json resource;
                    auto pluginResource = plugin.instance->GetResource(i);
                    resource["name"] = pluginResource->name;
                    resource["description"] = pluginResource->description;
                    resource["uri"] = pluginResource->uri;
                    resource["mimeType"] = pluginResource->mime;
                    response["result"]["resources"].push_back(resource);
                }
            }
        }

        return response;
    });
    server->OverrideCallback("resources/read", [&loader](const json& request) {
        nlohmann::ordered_json response = MCPBuilder::Response(request);

        char* res_ptr = nullptr;

        for (const auto& plugin : loader->GetPlugins()) {
            if (plugin.instance->GetType() == PLUGIN_TYPE_RESOURCES) {
                for (int i = 0; i < plugin.instance->GetResourceCount(); i++) {
                    auto pluginResource = plugin.instance->GetResource(i);
                    if (pluginResource->uri == request["params"]["uri"]) {
                        res_ptr = plugin.instance->HandleRequest(request.dump().c_str());
                        if (res_ptr) {
                            try {
                                response["result"] = json::parse(res_ptr);
                            } catch (const json::parse_error& e) {
                                LOG(ERROR) << "Plugin " << pluginResource->name << " returned malformed data." << std::endl;
                                // TODO: how can we handle error here ?
                            }
                            // --- Free the allocated memory ---
                            delete[] res_ptr;
                        }
                    }
                }
            }
        }

        return response;
    });

    server->Connect(transport);

    return 0;
}
