#include "endpoints/launch.h"

void registerLaunchEndpoints(HttpServer &server) {
    // Launches the Wii U Menu
    server.when("/launch/menu")->posted([](const HttpRequest &req) {
        DEBUG_FUNCTION_LINE_INFO("Launching Menu.");
        // FIXME: May lock up when the plugin is inactive, like in friends list
        SYSLaunchMenu();
        return HttpResponse{200};
    });


    // Launches a title. First it checks if it exists
    server.when("/launch/title")->posted([](const HttpRequest &req) {
        auto titleId = req.json().toObject();
        uint64_t id  = stoll(titleId["title"].toString());
        if (SYSCheckTitleExists(id)) {
            DEBUG_FUNCTION_LINE_INFO("Launching requested title.");
            SYSLaunchTitle(id);
        } else {
            DEBUG_FUNCTION_LINE_ERR("Title ID doesn't exist!");
            return HttpResponse{404};
        }
        return HttpResponse{200};
    });
}
