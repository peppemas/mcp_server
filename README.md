# CPP MCP-SERVER

A C++ implementation of a Model Context Protocol Server with a pluggable module architecture.

| Server     | Resources | Prompts | Tools | Sampling | Notifications | Roots  | Transport  |
|------------|-----------|---------|-------|----------|---------------|--------|------------|
| MCP-SERVER | ✅*        | ✅	      | ✅	    | ❌	| ✅**           | ❌	     | stdio      |

\* Resource Templates aren't supported yet

\** Partially supported 

## Supported Platforms

| Platform     | Supported | Compiler |
|--------------|-----------|----------|
| Windows      | ✅        | MINGW64  |
| Ubuntu Linux | ✅        | GCC      |
| Mac OS       | ✅        | GCC      |

## How to compile

Clone the repository on your local machine 

```commandline
git clone https://github.com/peppemas/mcp_server.git
```

Compile

```commandline
cd mcp_server
mkdir build
cd build
cmake ..
make
```

## MCP Server Architecture

The MCP Server is designed to implement a Model Context Protocol, enabling a modular and extensible architecture. Below
is an overview of the project's architecture:

### Core Components

1. **Server Core**:  
   The central component of the server that handles the initialization and management of all major functionalities,
   including communication with clients, logging, and plugin management.

2. **Command-Line Interface**:  
   Provides command-line arguments for configuration:
    - `-n`: Specifies the name of the server to expose to clients. (optional)
    - `-p`: Sets the path to the plugins directory where to search the plugins (it searches also in subdirectories).
    - `-l`: Sets the path to the log directory.

3. **Plugin System**:  
   The server is designed to load plugins dynamically from a specified directory (`-p` argument). Each plugin extends
   the server's functionality, allowing for future expansion without modifying the core codebase.

4. **Logging Module**:  
   Logging is centralized, with logs stored in the directory specified by the `-l` parameter. This enhances debugging
   and monitoring of the server's activity.

### Execution Flow

1. **Initialization**:
    - The server starts by parsing the command-line arguments to configure the server name, plugin directory, and logs
      directory.
    - Logs are initialized, and the plugin system is prepared.

2. **Plugin Loading**:
    - Plugins from the specified directory are identified and loaded dynamically.
    - Each plugin is initialized and registered with the server.

3. **Handling Client Requests**:
    - The server listens for incoming client connections.
    - It processes requests based on the implemented Model Context Protocol, which relies on registered plugins for
      extended capabilities.

4. **Error Handling**:
    - Errors during startup or runtime are logged in the logging directory, ensuring minimal disruption to the server's
      operation.

### Plugin Extensibility

The MCP Server's extensibility is powered by its plugin system, enabling developers to expand the server's capabilities
without modifying the core logic. Plugins are dynamically loaded libraries and can be implemented in various ways
depending on the operating system:

- **On Windows**: Plugins are compiled as `.dll` files.
- **On Linux/MacOS**: Plugins are compiled as `.so` or `.dylib` (shared object) files.

#### Example Plugins

1. **Weather Plugin**:  
   The Weather plugin allows the server to provide weather-related functionalities. This plugin can process requests
   related to weather data, such as current temperature, forecasts, and other meteorological information. The plugin
   fetches data from third-party APIs to ensure up-to-date and accurate information. It demonstrates how the MCP server
   can be extended to offer services that require external data integration.

2. **Sleep Plugin**:  
   The Sleep plugin introduces delay or "sleep" functionality within the server's processing pipeline. This can be
   useful for simulating response delays or managing timed operations in protocols. Developers can leverage this plugin
   to build and test features related to time-based operations without altering core server behavior. It highlights how
   custom plugins can target specific, niche use cases.

3. **Code Review Plugin**:  
   The Code Review plugin is designed to assist developers in performing automated code reviews. This plugin analyzes
   code submitted to the server and provides helpful feedback, such as identifying potential bugs, code smells, and
   optimization opportunities. It follows standard coding practices and can be configured to enforce specific style
   guides (e.g., Google C++ Style Guide). Additionally, it supports multi-language inspection, ensuring compatibility
   with various programming languages. The plugin can integrate with version control systems to retrieve code changes
   directly for assessment, making it a valuable tool for collaborative development workflows.

## Claude Desktop Configuration

```json
{
  "mcpServers": {
    "mcp-server": {
      "command": "C:\\mcp-server\\mcp_server.exe",
      "args": [
        "-n","developer-server",
        "-l","C:\\mcp-server\\logs",
        "-p","C:\\mcp-server\\plugins"
      ],
      "env": {
        "CUSTOM_API_KEY_1": "your-api-key-here",  
        "CUSTOM_API_KEY_2": "your-api-key-here",
        "SAVE_DIR": "/path/to/save/directory"
      }      
    }
  }
}
```
**NOTE**: In the "env" block you can pass parameters to your plugins
e

### TODO LIST

* Sampling [IN PROGRESS]
* Review log level implementation
* Add json schema validator (https://github.com/pboettch/json-schema-validator)
* Implements SSE transport
* Test on different mcp-client (actually tested only on Claude Desktop)
