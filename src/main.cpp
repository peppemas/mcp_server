#include "version.h"
#include "httplib.h"
#include "popl.hpp"
#include "StdioTransport.h"
#include "server/Server.h"
#include "aixlog.hpp"
#include "loader/PluginsLoader.h"

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
void ServerNotificationCallbackImpl(const char* pluginName, const char* method, const char* params) {
    std::lock_guard<std::mutex> lock(notificationState.serverNotificationMutex);
    if (server && server->IsValid()) {
        //server->SendNotification(pluginName, method, params);
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
        plugin.instance->notifications->SendToServer = ServerNotificationCallbackImpl;
    }

    //============================================================================================
    // start server
    //============================================================================================
    server->Name(name);
    server->VerboseLevel(verbose ? 1 : 0);
    server->OverrideCallback("tools/list", [&loader](const json& request) {
        nlohmann::ordered_json response;

        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
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
        nlohmann::ordered_json response;

        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
        response["result"] = json::object();
        response["result"]["isError"] = true;

        for (const auto& plugin : loader->GetPlugins()) {
            if (plugin.instance->GetType() == PLUGIN_TYPE_TOOLS) {
                for (int i = 0; i < plugin.instance->GetToolCount(); i++) {
                    auto pluginTool = plugin.instance->GetTool(i);
                    if (pluginTool->name == request["params"]["name"]) {
                        const char* res = plugin.instance->HandleRequest(request.dump().c_str());
                        response["result"] = json::parse(res);
                    }
                }
            }
        }

        return response;
    });
    server->OverrideCallback("prompts/list", [&loader](const json& request) {
        nlohmann::ordered_json response;

        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
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
        nlohmann::ordered_json response;

        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
        response["result"] = json::object();

        for (const auto& plugin : loader->GetPlugins()) {
            if (plugin.instance->GetType() == PLUGIN_TYPE_PROMPTS) {
                for (int i = 0; i < plugin.instance->GetPromptCount(); i++) {
                    auto pluginPrompt = plugin.instance->GetPrompt(i);
                    if (pluginPrompt->name == request["params"]["name"]) {
                        const char* res = plugin.instance->HandleRequest(request.dump().c_str());
                        response["result"] = json::parse(res);
                    }
                }
            }
        }

        return response;
    });
    server->OverrideCallback("resources/list", [&loader](const json& request) {
        nlohmann::ordered_json response;

        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
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
        nlohmann::ordered_json response;

        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
        response["result"] = json::object();

        for (const auto& plugin : loader->GetPlugins()) {
            if (plugin.instance->GetType() == PLUGIN_TYPE_RESOURCES) {
                for (int i = 0; i < plugin.instance->GetResourceCount(); i++) {
                    auto pluginResource = plugin.instance->GetResource(i);
                    if (pluginResource->uri == request["params"]["uri"]) {
                        const char* res = plugin.instance->HandleRequest(request.dump().c_str());
                        response["result"] = json::parse(res);
                    }
                }
            }
        }

        return response;
    });

    server->Connect(transport);

    return 0;
}
