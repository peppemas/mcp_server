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

#include "PluginAPI.h"
#include "json.hpp"
#include "httplib.h"

using json = nlohmann::json;

static PluginTool methods[] = {
        {
            "get_weather",
            "Get weather forecast of a city in the world. just pass as parameter the latitude and longitude of the city you want to know the weather forecast.",
        R"({
            "$schema": "http://json-schema.org/draft-07/schema#",
            "type": "object",
            "properties": {
                "latitude": { "type": "string" },
                "longitude": { "type": "string" },
                "city": { "type": "string" }
            },
            "required": ["city","latitude","longitude"],
            "additionalProperties": false
        })"
        }
};

const char* GetNameImpl() { return "weather-tools"; }
const char* GetVersionImpl() { return "1.0.0"; }
PluginType GetTypeImpl() { return PLUGIN_TYPE_TOOLS; }

int InitializeImpl() {
    return 1;
}

char* HandleRequestImpl(const char* req) {
    auto request = json::parse(req);

    auto latitude = request["params"]["arguments"]["latitude"].get<std::string>();
    auto longitude = request["params"]["arguments"]["longitude"].get<std::string>();
    auto city = request["params"]["arguments"]["city"].get<std::string>();

    nlohmann::json weatherContent;

    httplib::Client cli("api.open-meteo.com");
    auto res = cli.Get("/v1/forecast?latitude=" + latitude + "&longitude=" + longitude + "&hourly=temperature_2m&forecast_days=1");
    if (res && res->status == 200) {
        // Parse the response from the weather API
        auto weatherData = json::parse(res->body);

        // Extract city name from the request
        auto city = request["params"]["arguments"]["city"].get<std::string>();

        // Create a human-readable message
        std::stringstream weatherMessage;
        weatherMessage << "Weather Forecast for " << city << ":\n\n";

        // Get the temperature data and time
        auto times = weatherData["hourly"]["time"];
        auto temperatures = weatherData["hourly"]["temperature_2m"];

        // Format the forecast in a human-friendly way
        weatherMessage << "Today's Temperature Forecast:\n";

        // Morning (6 AM - 12 PM)
        weatherMessage << "🌅 Morning: ";
        double morningTemp = 0.0;
        int morningCount = 0;
        for (int i = 6; i < 12; i++) {
            morningTemp += temperatures[i].get<double>();
            morningCount++;
        }
        morningTemp /= morningCount;
        weatherMessage << std::fixed << std::setprecision(1) << morningTemp << "°C\n";

        // Afternoon (12 PM - 6 PM)
        weatherMessage << "☀️ Afternoon: ";
        double afternoonTemp = 0.0;
        int afternoonCount = 0;
        for (int i = 12; i < 18; i++) {
            afternoonTemp += temperatures[i].get<double>();
            afternoonCount++;
        }
        afternoonTemp /= afternoonCount;
        weatherMessage << std::fixed << std::setprecision(1) << afternoonTemp << "°C\n";

        // Evening (6 PM - 12 AM)
        weatherMessage << "🌙 Evening: ";
        double eveningTemp = 0.0;
        int eveningCount = 0;
        for (int i = 18; i < 24; i++) {
            eveningTemp += temperatures[i].get<double>();
            eveningCount++;
        }
        eveningTemp /= eveningCount;
        weatherMessage << std::fixed << std::setprecision(1) << eveningTemp << "°C\n\n";

        // Find highest and lowest temperatures of the day
        double maxTemp = temperatures[0].get<double>();
        double minTemp = temperatures[0].get<double>();
        std::string maxTime, minTime;

        for (size_t i = 0; i < temperatures.size(); i++) {
            double temp = temperatures[i].get<double>();
            if (temp > maxTemp) {
                maxTemp = temp;
                maxTime = times[i].get<std::string>().substr(11, 5); // Extract HH:MM
            }
            if (temp < minTemp) {
                minTemp = temp;
                minTime = times[i].get<std::string>().substr(11, 5); // Extract HH:MM
            }
        }

        weatherMessage << "🔼 Highest: " << std::fixed << std::setprecision(1) << maxTemp << "°C at " << maxTime << "\n";
        weatherMessage << "🔽 Lowest: " << std::fixed << std::setprecision(1) << minTemp << "°C at " << minTime << "\n\n";

        // Add overall daily summary
        weatherMessage << "📊 Daily Summary: ";
        if (maxTemp > 25) {
            weatherMessage << "Hot day! ";
        } else if (maxTemp > 20) {
            weatherMessage << "Warm day. ";
        } else if (maxTemp > 10) {
            weatherMessage << "Mild temperatures. ";
        } else {
            weatherMessage << "Cool day. ";
        }

        // Add temperature variation description
        double tempVariation = maxTemp - minTemp;
        if (tempVariation > 10) {
            weatherMessage << "Large temperature variation throughout the day.";
        } else if (tempVariation > 5) {
            weatherMessage << "Moderate temperature changes expected.";
        } else {
            weatherMessage << "Fairly consistent temperatures today.";
        }

        weatherContent["type"] = "text";
        weatherContent["text"] = weatherMessage.str();
    } else {
        weatherContent["type"] = "text";
        weatherContent["text"] = "Cannot get weather forecast for " + city + ".";
    }

    nlohmann::json response;

    response["content"] = json::array();
    response["content"].push_back(weatherContent);
    response["isError"] = false;

    std::string result = response.dump();
    char* buffer = new char[result.length() + 1];
#ifdef _WIN32
    strcpy_s(buffer, result.length() + 1, result.c_str());
#else
    strcpy(buffer, result_str.c_str());
#endif

    return buffer;
}

void ShutdownImpl() {
}

int GetToolCountImpl() {
    return sizeof(methods) / sizeof(methods[0]);
}

const PluginTool* GetToolImpl(int index) {
    if (index < 0 || index >= GetToolCountImpl()) return nullptr;
    return &methods[index];
}

static PluginAPI plugin = {
        GetNameImpl,
        GetVersionImpl,
        GetTypeImpl,
        InitializeImpl,
        HandleRequestImpl,
        ShutdownImpl,
        GetToolCountImpl,
        GetToolImpl,
        nullptr,
        nullptr,
        nullptr,
        nullptr
};

extern "C" PLUGIN_API PluginAPI* CreatePlugin() {
    return &plugin;
}

extern "C" PLUGIN_API void DestroyPlugin(PluginAPI*) {
    // Nothing to clean up for this example
}
