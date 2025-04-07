//
// Created by giuse on 03 apr 2025.
//

#ifndef MCP_SERVER_STDIO_TRANSPORT_H
#define MCP_SERVER_STDIO_TRANSPORT_H

#include "ITransport.h"

namespace vx::transport {

    class Stdio : public vx::ITransport {
    public:
        Stdio() = default;

        std::pair<size_t, std::string> Read() override;
        void Write(const std::string& json_data) override;

        inline std::string GetName() override { return "stdio"; }
        inline std::string GetVersion() override { return "0.1"; }
        inline int GetPort() override { return 0; }
    };

}

#endif //MCP_SERVER_STDIO_TRANSPORT_H
