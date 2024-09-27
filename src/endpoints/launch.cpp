#include "launch.h"

inline SysAppSettingsArgs *settingsArgsFromTarget(SYSSettingsJumpToTarget target) {
    SysAppSettingsArgs *ret = new SysAppSettingsArgs;
    ret->stdArgs            = (SYSStandardArgsIn) NULL;
    ret->jumpTo             = target;
    ret->firstBootKind      = 0;
    return ret;
}

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

    // Launches System Settings - the main application.
    server.when("/launch/settings")->posted([](const HttpRequest &req) {
        _SYSLaunchSettings(NULL);
        return HttpResponse{200};
    });

    // ----------------------------------------
    //       System Settings Targets
    // ----------------------------------------
    //
    // There are no title IDs for the specific "targets" - the submenus in
    // the System Settings. That's why they aren't exposed via MCP. An
    // integration should manually add inputs and allow for customizing
    // which inputs are "blacklisted".
    //
    // These exist because if desired to just open for example
    // the Internet Settings, this provides an endpoint to launch it.
    // This just opens internet settings but when exiting won't bring you
    // back to selecting another settings menu, but back to the Wii U Menu.
    // This is used when you see the "Set Up Connection" screen.

    server.when("/launch/settings/internet")->posted([](const HttpRequest &req) {
        _SYSLaunchSettings(settingsArgsFromTarget(SYS_SETTINGS_JUMP_TO_INTERNET));
        return HttpResponse{200};
    });

    server.when("/launch/settings/data_management")->posted([](const HttpRequest &req) {
        _SYSLaunchSettings(settingsArgsFromTarget(SYS_SETTINGS_JUMP_TO_DATA_MANAGEMENT));
        return HttpResponse{200};
    });

    server.when("/launch/settings/tv_remote")->posted([](const HttpRequest &req) {
        _SYSLaunchSettings(settingsArgsFromTarget(SYS_SETTINGS_JUMP_TO_TV_REMOTE));
        return HttpResponse{200};
    });

    server.when("/launch/settings/date_time")->posted([](const HttpRequest &req) {
        _SYSLaunchSettings(settingsArgsFromTarget(SYS_SETTINGS_JUMP_TO_DATE_TIME));
        return HttpResponse{200};
    });

    server.when("/launch/settings/country")->posted([](const HttpRequest &req) {
        _SYSLaunchSettings(settingsArgsFromTarget(SYS_SETTINGS_JUMP_TO_COUNTRY));
        return HttpResponse{200};
    });

    server.when("/launch/settings/quick_start")->posted([](const HttpRequest &req) {
        _SYSLaunchSettings(settingsArgsFromTarget(SYS_SETTINGS_JUMP_TO_QUICK_START_SETTINGS));
        return HttpResponse{200};
    });

    server.when("/launch/settings/tv_connection")->posted([](const HttpRequest &req) {
        _SYSLaunchSettings(settingsArgsFromTarget(SYS_SETTINGS_JUMP_TO_TV_CONNECTION_TYPE));
        return HttpResponse{200};
    });

    // Wipe console will not be added. (TODO: Consider ignoring all input if in system settings for security reasons)
    // System Transfer has its own title ID.
    // Data Management 2 is the same as regular data management
    // None and Unknown are just regular, Initial Settings will also not be added.
    // ----------------------------------------


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
