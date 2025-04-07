//
// Created by giuse on 03 apr 2025.
//

#ifndef MCP_SERVER_ITRANSPORT_H
#define MCP_SERVER_ITRANSPORT_H

#include <string>

namespace vx {

    class ITransport {
    public:
        virtual std::pair<size_t, std::string> Read() = 0;
        virtual void Write(const std::string& json_data) = 0;

        virtual std::string GetName() = 0;
        virtual std::string GetVersion() = 0;
        virtual int GetPort() = 0;
    };

}

#endif //MCP_SERVER_ITRANSPORT_H
