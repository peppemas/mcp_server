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

#ifndef MCP_SERVER_MCPBUILDER_H
#define MCP_SERVER_MCPBUILDER_H

#include "json.hpp"
#include "base64.hpp"

using json = nlohmann::json;

class MCPBuilder {

public:

    enum ErrorCode {
        ParseError = -32700,
        InvalidRequest = -32600,
        MethodNotFound = -32601,
        InvalidParams = -32602,
        InternalError = -32603
    };

    static json Response(json request) {
        json response;
        response["jsonrpc"] = "2.0";
        response["id"] = request["id"];
        response["result"] = json::object();
        return response;
    }

    static json Error(ErrorCode code, const std::string& id, const std::string &message) {
        return {
                {"jsonrpc", "2.0"},
                {"error", {{"code", code}, {"message", message}}},
                {"id", id}
        };
    }

    static json TextContent(const std::string& text) {
        return json::object({
            {"type","text"},
            {"text", text }
        });
    }

    static json ImageContent(const std::vector<uint8_t>& data, const std::string& mimeType) {
        auto b64 = base64::encode_into<std::string>(data.begin(), data.end());
        return json::object({
            {"type","image"},
            {"mimeType", mimeType},
            {"data", b64}
        });
    }

    static json AudioContent(const std::vector<uint8_t>& data, const std::string& mimeType) {
        auto b64 = base64::encode_into<std::string>(data.begin(), data.end());
        return json::object({
            {"type","audio"},
            {"mimeType", mimeType},
            {"data", b64}
        });
    }

    static json ResourceText(const std::string& uri, const std::string& mime, const std::string& text) {
        return json::object({
            {"uri",      uri},
            {"mimeType", mime},
            {"text",     text}
        });
    }

    static json NotificationLog(const std::string& level, const std::string& data) {
        return json::object({
            {"jsonrpc", "2.0"},
            {"method","notifications/message"},
            {"params", { {"level",level}, {"data", data} }}
        });
    }

    static json NotificationProgress(const std::string& message, const std::string& progressToken, const int progress, const int total) {
        return json::object({
            {"jsonrpc", "2.0"},
            {"method","notifications/progress"},
            {"params", { {"progressToken",progressToken}, {"progress",progress}, {"total",total}, {"message",message} }}
        });
    }

};

#endif //MCP_SERVER_MCPBUILDER_H
