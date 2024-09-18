#include "languages.h"
#include "utils/logger.h"
#include <coreinit/bsp.h>
#include <coreinit/filesystem.h>
#include <coreinit/launch.h>
#include <coreinit/mcp.h>
#include <coreinit/thread.h>
#include <cstddef>
#include <http/http.hpp>
#include <malloc.h>
#include <nn/ac.h>
#include <nn/acp/title.h>
#include <nn/act.h>
#include <nn/sl/TitleListCache.h>
#include <sysapp/launch.h>
#include <sysapp/title.h>
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

/**
    All of this defines can be used in ANY file.
    It's possible to split it up into multiple files.

**/

WUPS_USE_WUT_DEVOPTAB();       // Use the wut devoptabs
WUPS_USE_STORAGE("ristretto"); // Unique id for the storage api

HttpServer server;
bool server_made     = false;
int button_value     = 0;
uint8_t vpad_battery = 0;

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

        server.when("/gamepad/battery")->requested([](const HttpRequest &req) {
            std::string ret = std::format("{:d}", vpad_battery);
            return HttpResponse{200, "text/plain", ret};
        });

        // Presses a specific key based on what is requested.
        // The "button" key in the JSON request will be the value
        // of the button as defined in the VPADButtons enum.
        // For performance purposes on the console, Ristretto
        // should not have to map which key means what.
        // The home automation integration should do so and set the value directly.
        server.when("/remote/key")->posted([](const HttpRequest &req) {
            auto key = req.json().toObject();

            button_value = stoi(key["button"].toString());
            return HttpResponse{200};
        });

        server.when("/title/current")->requested([](const HttpRequest &req) {
            ACPTitleId id;
            ACPResult res = ACPGetTitleIdOfMainApplication(&id);
            if (res) {
                DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleIdOfMainApplication");
                return HttpResponse{500, "text/plain", "Couldn't get the current title! Error at ACPGetTitleIdOfMainApplication"};
            }
            ACPMetaXml *meta = new ACPMetaXml;
            res              = ACPGetTitleMetaXml(id, meta);
            if (res) {
                DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleMetaXml");
                return HttpResponse{500, "text/plain", "Couldn't get the title! Error at ACPGetTitleMetaXml"};
            }
            return HttpResponse{200, "text/plain", getTitleLongname(meta)};
        });

        // NOT FOR HOMEBREW TITLES!!!!!!!
        server.when("/title/list")->requested([](const HttpRequest &req) {
            DEBUG_FUNCTION_LINE_INFO("Getting title list.");
            int handle = MCP_Open();
            if (handle < 0) { // some error?
                throw std::runtime_error{"MCP_Open() failed with error " + std::to_string(handle)};
            }

            uint32_t outCount;
            std::vector<MCPTitleListType> titleList(1000); // arbitrary number so we don't overflow
            MCPError error = MCP_TitleList(handle, &outCount, titleList.data(), titleList.size() * sizeof(MCPTitleListType));
            MCP_Close(handle);
            if (error) {
                DEBUG_FUNCTION_LINE_ERR("Error at MCP_TitleList");
                return HttpResponse{500, "text/plain", "Couldn't get the title list! Error at MCP_TitleList"};
            }

            miniJson::Json::_object res;

            // This is my first time trying this with C++ vectors, so lets see what happens.
            // Ideally it will just keep rewriting to meta?
            for (auto &title : titleList) {
                ACPMetaXml meta alignas(0x40);

                // not all titles are actual game titles
                // TODO: For vWii titles, allow it under the condition we are able to
                // send back to the server that Ristretto won't be active.
                // All titles under MCP_APP_TYPE_GAME (or any Wii U system title) will
                // allow for Ristretto control inside of it: not sure about homebrew.
                if (title.appType == MCP_APP_TYPE_GAME ||
                    title.appType == MCP_APP_TYPE_GAME_WII ||
                    title.appType == MCP_APP_TYPE_SYSTEM_MENU ||
                    title.appType == MCP_APP_TYPE_SYSTEM_APPS ||
                    title.appType == MCP_APP_TYPE_ACCOUNT_APPS ||
                    title.appType == MCP_APP_TYPE_SYSTEM_SETTINGS) {
                    ACPResult acpError = ACPGetTitleMetaXml(title.titleId, &meta);
                    if (acpError) {
                        DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleMetaXml. Title ID %d", title.titleId);
                        continue;
                    }

                    // TODO: Consider returning other languages
                    if (meta.longname_en[0] != '\0') {
                        DEBUG_FUNCTION_LINE_INFO("Finished %s", meta.longname_en);
                        try {
                            res[std::to_string(title.titleId)] = getTitleLongname(&meta);
                            DEBUG_FUNCTION_LINE_INFO("Written to JSON");
                        } catch (std::exception &e) {
                            DEBUG_FUNCTION_LINE_ERR("Failed to write title to JSON: %s\n", e.what());
                        }
                    } else {
                        DEBUG_FUNCTION_LINE_INFO("No English longname - not proceeding");
                    }
                }
            }

            return HttpResponse{200, res};
        });

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

        // Switch to the current title's manual.
        //
        // This is interesting! If the title has no manual (because none exists, or like, you aren't
        // supposed to be able to get into the HOME menu to open the manual), the title's e_manual_version
        // field will be set to 0. The version field dictates whether the manual exists. The e_manual
        // field, while an integer, I believe is just a boolean for whether the manual button in the home menu
        // is enabled.
        // Exceptions where we see the manual button enabled but no manual specific to the title is
        // first: Wii U Menu. This is because it redirects to the Wii U Electronic Manual and it unfortunately seems
        // to not be launchable via Ristretto.
        // second: Virtual Console titles. They have an e_manual_version of 0, but they can be launched as usual
        //
        // Cases where we see the manual button disabled and there is no manual are System Settings,
        // H&SA, basically whenever the HOME menu was not intended to be opened.
        //
        // So, check to see if a manual exists via its version. If it does, open it. Otherwise, check to see if it is a game because there
        // most likely has to be a manual. If those conditions fail, we are unable to at the moment.
        server.when("/switch/emanual")->posted([](const HttpRequest &req) {
            DEBUG_FUNCTION_LINE_INFO("Manual switch requested. Checking to see if there is an emanual..");

            ACPTitleId id;
            ACPResult res = ACPGetTitleIdOfMainApplication(&id);
            if (res) {
                DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleIdOfMainApplication");
                return HttpResponse{500, "text/plain", "Couldn't get the current title! Error at ACPGetTitleIdOfMainApplication"};
            }
            ACPMetaXml *meta = new ACPMetaXml;
            res              = ACPGetTitleMetaXml(id, meta);
            if (res) {
                DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleMetaXml");
                return HttpResponse{500, "text/plain", "Couldn't get the title! Error at ACPGetTitleMetaXml"};
            }

            if (meta->e_manual_version > 0) {
                SYSSwitchToEManual();
            } else {
                // Check to see if we are running a game, because virtual console titles (at least Super Mario Kart)
                // have this field set to 0 but it does have a manual
                // Applications with the field being set to 0 mean that there isn't a manual.
                // FIXME: System applications - when requesting to open the manual, just open the Wii U Electronic Manual. Launching via title ID does not seem to work so what will?

                int handle = MCP_Open();
                if (handle < 0) { // some error?
                    throw std::runtime_error{"MCP_Open() failed with error " + std::to_string(handle)};
                }

                MCPTitleListType *titleType = new MCPTitleListType;
                MCPError err                = MCP_GetTitleInfo(handle, id, titleType);
                if (err) {
                    DEBUG_FUNCTION_LINE_ERR("Error at MCP_GetTitleInfo");
                    return HttpResponse{500, "text/plain", "Couldn't get the title type! Error at MCP_GetTitleInfo"};
                }
                MCP_Close(handle);

                if (titleType->appType == MCP_APP_TYPE_GAME) {
                    SYSSwitchToEManual();
                } else {
                    return HttpResponse{409, "text/plain", "The current application has no manual."};
                }
            }
            return HttpResponse{200};
        });

        // TODO: Make the port configurable
        server.startListening(8572);
    } catch (std::exception &e) {
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
    // To use the C++ API, we create new WUPSConfigCategory from the root handle!
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

    // The functions of the Config API come in two variants: One that throws an exception, and another one which doesn't
    // To use the Config API without exception see the example below this try/catch block.
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
