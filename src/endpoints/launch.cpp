#include "endpoints/launch.h"

void registerLaunchEndpoints(HttpServer &server) {
    // Launches the Wii U Menu
    server.when("/launch/menu")->posted([](const HttpRequest &req) {
        DEBUG_FUNCTION_LINE_INFO("Launching Menu.");
        // FIXME: May lock up when the plugin is inactive, like in friends list
        SYSLaunchMenu();
        return HttpResponse{200};
    });

    // Launches Mii Maker
    server.when("/launch/miistudio")->posted([](const HttpRequest &req) {
        SYSLaunchMiiStudio(NULL);
        return HttpResponse{200};
    });


    // Launches Notifications
    server.when("/launch/notifications")->posted([](const HttpRequest &req) {
        _SYSLaunchNotifications(NULL);
        return HttpResponse{200};
    });

    // Launches Parental Controls
    server.when("/launch/parental")->posted([](const HttpRequest &req) {
        _SYSLaunchParental(NULL);
        return HttpResponse{200};
    });

    // Launches System Settings
    // TODO: "Jumping to target" (opening a setting submenu directly)
    server.when("/launch/settings")->posted([](const HttpRequest &req) {
        _SYSLaunchSettings(NULL);
        return HttpResponse{200};
    });

    // Launches a title by the given title ID in decimal form.
    server.when("/launch/title")->posted([](const HttpRequest &req) {
        auto titleId = req.json().toObject();
        uint64_t id  = stoll(titleId["title"].toString());

        // Check title exists before trying to launch
        // FIXME: Wii application/Wii titles?
        if (SYSCheckTitleExists(id)) {
            DEBUG_FUNCTION_LINE_INFO("Launching requested title.");
            SYSLaunchTitle(id);
        } else {
            DEBUG_FUNCTION_LINE_ERR("Title ID doesn't exist!");
            return HttpResponse{404, "text/plain", "Title ID doesn't exist!"};
        }

        return HttpResponse{200};
    });
}
