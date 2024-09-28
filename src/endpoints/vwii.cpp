#include "vwii.h"

void registervWiiEndpoints(HttpServer &server) {
    // Launches the vWii System Menu (both TV and GamePad)
    server.when("/vwii/launch/menu")->posted([](const HttpRequest &req) {
        DEBUG_FUNCTION_LINE_INFO("Launching vWii Menu.");
        uint32_t outSize alignas(0x40);
        int ret = CMPTGetDataSize(&outSize);
        if (ret != 0) {
            return HttpResponse{500, "text/plain", "Error getting data buffer size"};
        }

        void *buf = malloc(outSize);
        CMPTLaunchMenu(buf, outSize);
        return HttpResponse{200};
    });

    // Launches the vWii Data Management Menu (both TV and GamePad)
    server.when("/vwii/launch/data_manager")->posted([](const HttpRequest &req) {
        DEBUG_FUNCTION_LINE_INFO("Launching vWii Data Manager.");
        uint32_t outSize alignas(0x40);
        int ret = CMPTGetDataSize(&outSize);
        if (ret != 0) {
            return HttpResponse{500, "text/plain", "Error getting data buffer size"};
        }

        void *buf = malloc(outSize);
        CMPTLaunchDataManager(buf, outSize);
        return HttpResponse{200};
    });
}
