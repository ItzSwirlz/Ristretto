#include "endpoints/gamepad.h"

void registerGamepadEndpoints(HttpServer &server) {
    server.when("/gamepad/battery")->requested([](const HttpRequest &req) {
        std::string ret = std::format("{:d}", vpad_battery);
        return HttpResponse{200, "text/plain", ret};
    });
}
