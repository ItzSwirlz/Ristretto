#include "../endpoints/device.h"
#include "../endpoints/gamepad.h"
#include "../endpoints/launch.h"
#include "../endpoints/odd.h"
#include "../endpoints/power.h"
#include "../endpoints/remote.h"
#include "../endpoints/switch.h"
#include "../endpoints/title.h"
#include "../endpoints/vwii.h"
#include "../languages.h"
#include "../utils/logger.h"
#include "globals.h"
#include "http.hpp"
#include <nn/ac.h>
#include <notifications/notifications.h>
#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemIntegerRange.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/config/WUPSConfigItemStub.h>
#include <wups/config_api.h>

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("Ristretto")
WUPS_PLUGIN_DESCRIPTION("home automation attempt");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("ItzSwirlz");
WUPS_PLUGIN_LICENSE("BSD");

WUPS_USE_WUT_DEVOPTAB();       // Use the wut devoptabs
WUPS_USE_STORAGE("ristretto"); // Unique id for the storage api

HttpServer server;
bool server_made = false;

#define ENABLE_SERVER_DEFAULT_VALUE true
#define ENABLE_SERVER_CONFIG_ID     "enableServer"
#define TITLE_LANG_CONFIG_ID        "titleLang"

bool enableServer = ENABLE_SERVER_DEFAULT_VALUE;

void make_server() {
    if (server_made) {
        return;
    }

    server_made = true;
    DEBUG_FUNCTION_LINE("Server started.");

    try {
        // Empty endpoint to allow for device discovery.
        server.when("/")->requested([](const HttpRequest &req) {
            return HttpResponse{200, "text/plain", "Ristretto"};
        });

        registerDeviceEndpoints(server);
        registerGamepadEndpoints(server);
        registerLaunchEndpoints(server);
        registerODDEndpoints(server);
        registerPowerEndpoints(server);
        registerRemoteEndpoints(server);
        registerSwitchEndpoints(server);
        registerTitleEndpoints(server);

        registervWiiEndpoints(server);

        // TODO: Make the port configurable
        server.startListening(8572);
    } catch (std::exception &e) {
        // FIXME: write good strings that can easily be translated
        NotificationModule_AddErrorNotification("Ristretto threw an exception. If the problem persists, check system logs.");
        DEBUG_FUNCTION_LINE_INFO("Exception thrown in the HTTP server: %s\n", e.what());
    }
}

void stop_server() {
    if (!enableServer || !server_made) return;
    // dont shut down what doesnt exist
    server.shutdown();
    server_made = false;
    DEBUG_FUNCTION_LINE("Server shut down.");
}

void make_server_on_thread() {
    try {
        std::jthread thready(make_server);
        thready.detach();
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_INFO("Exception thrown trying to make the server thread: %s\n", e.what());
    }
}

void enableServerChanged(ConfigItemBoolean *item, bool newValue) {
    // If the value has changed, we store it in the storage.
    if (newValue != enableServer)
        WUPSStorageAPI::Store(ENABLE_SERVER_CONFIG_ID, newValue);

    enableServer = newValue;
}

static void titleLangChanged(ConfigItemMultipleValues *item, uint32_t newValue) {
    if (newValue != titleLang) {
        WUPSStorageAPI::Store(TITLE_LANG_CONFIG_ID, newValue);
    }
    titleLang = newValue;
}

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

    constexpr WUPSConfigItemMultipleValues::ValuePair titleLangMap[] = {
            {LANG_ENGLISH, "English"},
            {LANG_JAPANESE, "Japanese"},
            {LANG_FRENCH, "French"},
            {LANG_GERMAN, "German"},
            {LANG_ITALIAN, "Italian"},
            {LANG_SPANISH, "Spanish"},
            {LANG_SIMPLIFIED_CHINESE, "Chinese (Simplified)"},
            {LANG_KOREAN, "Korean"},
            {LANG_DUTCH, "Dutch"},
            {LANG_PORTUGUESE, "Portuguese"},
            {LANG_RUSSIAN, "Russian"},
            {LANG_TRADITIONAL_CHINESE, "Chinese (Traditional)"}};

    try {
        root.add(WUPSConfigItemBoolean::Create(ENABLE_SERVER_CONFIG_ID, "Enable Server", ENABLE_SERVER_DEFAULT_VALUE, enableServer, enableServerChanged));
        root.add(WUPSConfigItemMultipleValues::CreateFromValue(TITLE_LANG_CONFIG_ID, "Title Language:", TITLE_LANG_DEFAULT_VALUE, titleLang, titleLangMap, titleLangChanged));
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_ERR("Creating config menu failed: %s", e.what());
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();

    if (server_made && !enableServer) stop_server();
    else if (!server_made && enableServer) {
        make_server_on_thread();
    }
}

// Function hooking for the HTTP server logs. Thank you to DanielKO
ssize_t
write_to_log(_reent *, void *, const char *ptr, size_t len) {
    try {
        // only way to guarantee it's null-terminated
        std::string buf{ptr, len};
        if (!WHBLogWrite(buf.c_str()))
            return -1;
        return buf.size();
    } catch (...) {
        return -1;
    }
}

__attribute__((__constructor__)) void
init_stdio() {
    static devoptab_t dev_out;
    dev_out.name           = "stdout";
    dev_out.write_r        = write_to_log;
    devoptab_list[STD_OUT] = &dev_out;
}

// Gets called ONCE when the plugin was loaded.
INITIALIZE_PLUGIN() {
    // Logging only works when compiled with `make DEBUG=1`. See the README for more information.
    WHBLogCafeInit();
    WHBLogUdpInit();
    NotificationModule_InitLibrary();

    DEBUG_FUNCTION_LINE("Hello world! - Ristretto");

    WUPSConfigAPIOptionsV1 configOptions = {.name = "Ristretto"};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback) != WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to init config api");
    }

    WUPSStorageError storageRes;
    if ((storageRes = WUPSStorageAPI::GetOrStoreDefault(ENABLE_SERVER_CONFIG_ID, enableServer, ENABLE_SERVER_DEFAULT_VALUE)) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("GetOrStoreDefault failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }
    if ((storageRes = WUPSStorageAPI::GetOrStoreDefault(TITLE_LANG_CONFIG_ID, titleLang, (uint32_t) TITLE_LANG_DEFAULT_VALUE)) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("GetOrStoreDefault failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }
    if ((storageRes = WUPSStorageAPI::SaveStorage()) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("SaveStorage failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }
}

// Gets called when the plugin will be unloaded.
DEINITIALIZE_PLUGIN() {
    stop_server();
    DEBUG_FUNCTION_LINE("Ristretto deinitializing.");
    NotificationModule_DeInitLibrary();
    WHBLogUdpDeinit();
    WHBLogCafeDeinit();
}

// Connections reset every time an application is launched.
ON_APPLICATION_START() {
    nn::ac::Initialize();
    nn::ac::ConnectAsync();
    if (!enableServer) return;
    make_server_on_thread();
}

ON_APPLICATION_ENDS() {
    if (!enableServer) return;
    stop_server();
}

DECL_FUNCTION(int32_t, VPADRead, VPADChan chan, VPADStatus *buffers, uint32_t count, VPADReadError *outError) {
    int result = real_VPADRead(chan, buffers, count, outError);
    if (*outError == VPAD_READ_SUCCESS) {
        vpad_battery = buffers->battery;
        if (button_value != 0) {
            buffers->hold |= button_value;
            button_value = 0; // done
        }
    }
    return result;
}

WUPS_MUST_REPLACE(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead);
