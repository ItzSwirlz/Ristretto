#include "device.h"

void registerDeviceEndpoints(HttpServer &server) {
    // Returns the following deivce information in JSON: The serial ID, model number, software version and hardware version.
    // TODO: Look at what else from MCPSysProdSettings we should return
    server.when("/device/info")->requested([](const HttpRequest &req) {
        int handle = MCP_Open();
        if (handle < 0) {
            throw std::runtime_error{"MCP_Open() failed with error " + std::to_string(handle)};
        }

        MCPSysProdSettings settings alignas(0x40);
        MCPError error = MCP_GetSysProdSettings(handle, &settings);
        if (error) {
            DEBUG_FUNCTION_LINE_ERR("Error at MCP_GetSysProdSettings");
            return HttpResponse{500, "text/plain", "Couldn't get device info! Error at MCP_GetSysProdSettings"};
        }

        MCPSystemVersion *version = new MCPSystemVersion;
        error                     = MCP_GetSystemVersion(handle, version);
        if (error) {
            DEBUG_FUNCTION_LINE_ERR("Error at MCP_GetSystemVersion");
            return HttpResponse{500, "text/plain", "Couldn't get device info! Error at MCP_GetSystemVersion"};
        }

        MCP_Close(handle);

        BSPHardwareVersion hw_ver;
        BSPError err = bspGetHardwareVersion(&hw_ver);
        if (err) {
            DEBUG_FUNCTION_LINE_ERR("Error at bspGetHardwareVersion");
            return HttpResponse{500, "text/plain", "Couldn't get device info! Error at bspGetHardwareVersion"};
        }

        miniJson::Json::_object ret;
        ret["serial_id"]        = settings.serial_id;
        ret["model_number"]     = settings.model_number;
        ret["system_version"]   = std::format("{:d}.{:d}.{:d}{}", version->major, version->minor, version->patch, version->region);
        ret["hardware_version"] = std::format("{:d}", hw_ver);

        return HttpResponse{200, ret};
    });


    // Gets the device serial number.
    server.when("/device/serial_id")->requested([](const HttpRequest &req) {
        // Credit to .danielko on Discord
        int handle = MCP_Open();
        if (handle < 0) { // some error?
            throw std::runtime_error{"MCP_Open() failed with error " + std::to_string(handle)};
        }

        MCPSysProdSettings settings alignas(0x40);
        MCPError error = MCP_GetSysProdSettings(handle, &settings);
        MCP_Close(handle);
        if (error) {
            DEBUG_FUNCTION_LINE_ERR("Error at MCP_GetSysProdSettings");
            return HttpResponse{500, "text/plain", "Couldn't get the serial! Error at MCP_GetSysProdSettings"};
        }

        DEBUG_FUNCTION_LINE_INFO("Obtained serial: %s", settings.serial_id);
        return HttpResponse{200, "text/plain", settings.serial_id};
    });

    // Gets the device model
    server.when("/device/model_number")->requested([](const HttpRequest &req) {
        // Credit to .danielko on Discord
        int handle = MCP_Open();
        if (handle < 0) { // some error?
            throw std::runtime_error{"MCP_Open() failed with error " + std::to_string(handle)};
        }

        MCPSysProdSettings settings alignas(0x40);
        MCPError error = MCP_GetSysProdSettings(handle, &settings);
        MCP_Close(handle);
        if (error) {
            DEBUG_FUNCTION_LINE_ERR("Error at MCP_GetSysProdSettings");
            return HttpResponse{500, "text/plain", "Couldn't get the model number! Error at MCP_GetSysProdSettings"};
        }

        DEBUG_FUNCTION_LINE_INFO("Obtained model number: %s", settings.model_number);
        return HttpResponse{200, "text/plain", settings.model_number};
    });

    // Gets the device version.
    server.when("/device/version")->requested([](const HttpRequest &req) {
        int handle = MCP_Open();
        if (handle < 0) {
            throw std::runtime_error{"MCP_Open() failed with error " + std::to_string(handle)};
        }

        MCPSystemVersion *version = new MCPSystemVersion;
        MCPError error            = MCP_GetSystemVersion(handle, version);
        MCP_Close(handle);
        if (error) {
            DEBUG_FUNCTION_LINE_ERR("Error at MCP_SystemVersion");
            return HttpResponse{500, "text/plain", "Couldn't get the system version! Error at MCP_SystemVersion"};
        }

        std::string ret = std::format("{:d}.{:d}.{:d}{}", version->major, version->minor, version->patch, version->region);
        return HttpResponse{200, "text/plain", ret};
    });

    // Gets the device hardware version from the BSP (in decimal form)
    // Frontend can deal with correspinding the version to the text
    server.when("/device/hardware_version")->requested([](const HttpRequest &req) {
        BSPHardwareVersion hw_ver;
        BSPError err = bspGetHardwareVersion(&hw_ver);
        if (err) {
            DEBUG_FUNCTION_LINE_ERR("Error at bspGetHardwareVersion");
            return HttpResponse{500, "text/plain", "Couldn't get the hardware version! Error at bspGetHardwareVersion"};
        }

        std::string ret = std::format("{:d}", hw_ver);
        return HttpResponse{200, "text/plain", ret};
    });
}
