#include "power.h"

void registerPowerEndpoints(HttpServer &server) {
    // Shuts down the console regardless of what state it currently is in.
    server.when("/power/shutdown")->posted([](const HttpRequest &req) {
        DEBUG_FUNCTION_LINE_INFO("Shutting down Wii U.");
        OSShutdown();

        return HttpResponse{200};
    });

    // Reboot the console regardless of what state it currently is in.
    server.when("/power/reboot")->posted([](const HttpRequest &req) {
        DEBUG_FUNCTION_LINE_INFO("Rebooting Wii U.");
        OSForceFullRelaunch();
        SYSLaunchMenu();

        return HttpResponse{200};
    });
}
