#include "odd.h"

void registerODDEndpoints(HttpServer &server) {
    // Returns the title ID if what is in the ODD.
    // FIXME: Only works for Wii U titles - add Wii Game support
    server.when("/odd/titleid")->requested([](const HttpRequest &req) {
        int handle = MCP_Open();
        if (handle < 0) { // some error?
            throw std::runtime_error{"MCP_Open() failed with error " + std::to_string(handle)};
        }

        uint32_t outCount;
        std::vector<MCPTitleListType> titleList(1); // there should only be one
        MCPError error = MCP_TitleListByDeviceType(handle, MCP_DEVICE_TYPE_ODD, &outCount, titleList.data(), titleList.size() * sizeof(MCPTitleListType));
        MCP_Close(handle);
        if (error) {
            DEBUG_FUNCTION_LINE_ERR("Error at MCP_TitleListByDevice");
            return HttpResponse{500, "text/plain", "Couldn't get the title of the disc in the ODD! Error at MCP_TitleListByDevice"};
        }

        uint64_t titleId = titleList.at(0).titleId;
        std::string ret  = std::format("{:d}", titleId);
        return HttpResponse{200, ret};
    });
}
