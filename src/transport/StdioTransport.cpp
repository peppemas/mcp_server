//
// Created by giuse on 03 apr 2025.
//

#include <iostream>
#include "StdioTransport.h"
#include "aixlog.hpp"

namespace vx::transport {

    std::pair<size_t, std::string> Stdio::Read() {
        std::string line;
        std::string json_data;

        // Read headers until an empty line
        while (true) {
            int c;
            while ((c = std::getc(stdin)) != EOF && c != '\n') {
                json_data += static_cast<char>(c);
            }
            break;
        }

        return {json_data.length(), json_data};
    }

    void Stdio::Write(const std::string& json_data) {
        std::cout << json_data << std::endl << std::flush;
    }

}
