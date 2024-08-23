#include "utils/logger.h"
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
bool server_made = false;

#define ENABLE_SERVER_DEFAULT_VALUE true
#define ENABLE_SERVER_CONFIG_ID     "enableServer"

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
            return HttpResponse{200};
        });

        // Shuts down the console regardless of what state it currently is in.
        server.when("/shutdown")->posted([](const HttpRequest &req) {
            OSShutdown();

            return HttpResponse{200};
        });

        // Reboot the console regardless of what state it currently is in.
        server.when("/reboot")->posted([](const HttpRequest &req) {
            OSForceFullRelaunch();
            SYSLaunchMenu();

            return HttpResponse{200};
        });

        // Gets the device serial number.
        server.when("/serial")->requested([](const HttpRequest &req) {
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
                return HttpResponse{500, "text/plain", "Couldn't get the serial!"};
            }

            DEBUG_FUNCTION_LINE_INFO("Obtained serial: %s", settings.serial_id);
            return HttpResponse{200, "text/plain", settings.serial_id};
        });

        // Gets the device model
        server.when("/model")->requested([](const HttpRequest &req) {
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
                return HttpResponse{500, "text/plain", "Couldn't get the model number!"};
            }

            DEBUG_FUNCTION_LINE_INFO("Obtained model number: %s", settings.model_number);
            return HttpResponse{200, "text/plain", settings.model_number};
        });

        // Gets the device version.
        server.when("/system_version")->requested([](const HttpRequest &req) {
            int handle = MCP_Open();
            if (handle < 0) {
                throw std::runtime_error{"MCP_Open() failed with error " + std::to_string(handle)};
            }

            MCPSystemVersion* version = new MCPSystemVersion;
            DEBUG_FUNCTION_LINE_INFO("pre-call system ver");
            MCPError error = MCP_GetSystemVersion(handle, version);
            DEBUG_FUNCTION_LINE_INFO("post-call system ver");
            MCP_Close(handle);
            if (error) {
                DEBUG_FUNCTION_LINE_ERR("Error at MCP_SystemVersion");
                return HttpResponse{500, "text/plain", "Couldn't get the system version!"};
            }

            // DEBUG_FUNCTION_LINE_INFO("Obtained version: %d . %d . %d%s", version->major, version->minor, version->patch, version->region);
            std::string ret = std::format("{:d}.{:d}.{:d}{}", version->major, version->minor, version->patch, version->region);
            return HttpResponse{200, "text/plain", ret};
        });

        server.when("/currenttitle")->requested([](const HttpRequest &req) {
            ACPTitleId id;
            ACPResult res = ACPGetTitleIdOfMainApplication(&id);
            if (res) {
                DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleIdOfMainApplication");
                return HttpResponse{500, "text/plain", "Couldn't get the title!"};
            }
            ACPMetaXml *meta = new ACPMetaXml;
            res              = ACPGetTitleMetaXml(id, meta);
            if (res) {
                DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleMetaXml");
                return HttpResponse{500, "text/plain", "Couldn't get the title!"};
            }
            return HttpResponse{200, "text/plain", meta->longname_en};
        });

        // TODO: Do this only when memory is available. If it isn't, use something
        // like the notification module to tell the user that it tried but denied
        // since proceeding would crash the system.
        // Personally, I've gotten this to work in System Settings without crashing.
        server.when("/titles")->requested([](const HttpRequest &req) {
            int handle = MCP_Open();
            if (handle < 0) { // some error?
                throw std::runtime_error{"MCP_Open() failed with error " + std::to_string(handle)};
            }

            // source: https://github.com/dumpling-app/dumpling/blob/83d674f4d6c8348bcdc253a26eb4c2424abe9ef9/source/app/titles.cpp#L153
            uint32_t count = MCP_TitleCount(handle);

            std::vector<MCPTitleListType> titleList(count);
            MCPError error = MCP_TitleList(handle, &count, titleList.data(), count * sizeof(MCPTitleListType));
            MCP_Close(handle);
            if (error) {
                DEBUG_FUNCTION_LINE_ERR("Error at MCP_TitleList");
                return HttpResponse{500, "text/plain", "Couldn't get the title list!"};
            }

            miniJson::Json::_object res;

            // This is my first time trying this with C++ vectors, so lets see what happens.
            // Ideally it will just keep rewriting to meta?
            for(auto& title : titleList) {
                ACPMetaXml meta alignas(0x40);

                // not all titles are actual game titles
                // TODO: For vWii titles, allow it under the condition we are able to
                // send back to the server that Ristretto won't be active.
                // All titles under MCP_APP_TYPE_GAME (or any Wii U system title) will
                // allow for Ristretto control inside of it: not sure about homebrew.
                if(title.appType == MCP_APP_TYPE_GAME || title.appType == MCP_APP_TYPE_GAME_WII || title.appType == MCP_APP_TYPE_SYSTEM_SETTINGS) {
                    try {
                        ACPResult acpError = ACPGetTitleMetaXml(title.titleId, &meta);
                        if (acpError) {
                            DEBUG_FUNCTION_LINE_ERR("Error at ACPGetTitleMetaXml. SKIPPING %d", title.titleId);
                            continue;
                            return HttpResponse{500, "text/plain", "Couldn't get the title list!"};
                        }
                    } catch(std::exception &e) {
                        DEBUG_FUNCTION_LINE_INFO("got error: %s\n", e.what());
                        return HttpResponse{500, "text/plain", "Couldn't get the title list!"};
                    }
                // ugh
                // also from dumpling
                // TODO: Consider returning other languages
                    if(meta.longname_en[0] != '\0') {
                        DEBUG_FUNCTION_LINE_INFO("Finished %s", meta.longname_en);
                        try {
                            res[std::to_string(title.titleId)] = meta.longname_en;
                            DEBUG_FUNCTION_LINE_INFO("Written to JSON");
                        } catch(std::exception &e) {
                            DEBUG_FUNCTION_LINE_INFO("got error: %s\n", e.what());
                        }
                    } else {
                        DEBUG_FUNCTION_LINE_INFO("No longname");
                    }
                }
            }

            return HttpResponse{200, res};
        });

        // Launches the Wii U Menu
        server.when("/launch/menu")->posted([](const HttpRequest &req) {
            // FIXME: May lock up when the plugin is inactive, like in friends list
            SYSLaunchMenu();
            return HttpResponse{200};
        });

        // Launches the current title's manual
        server.when("/launch/emanual")->posted([](const HttpRequest &req) {
            // FIXME: If the title has no manual, DO NOT SWITCH!!!! IT WILL LOCKUP THE SYSTEM! (eg Friends List)
            SYSSwitchToEManual();
            return HttpResponse{200};
        });

        // Launches a title. First it checks if it exists
        server.when("/launch/title")->posted([](const HttpRequest &req) {
            auto titleId = req.json().toObject();
            uint64_t id = stoll(titleId["title"].toString());
            if(SYSCheckTitleExists(id)) {
                DEBUG_FUNCTION_LINE_INFO("Opening requested title");
                SYSLaunchTitle(id);
            } else {
                DEBUG_FUNCTION_LINE_INFO("Title ID doesn't exist!");
                return HttpResponse{404};
            }
            return HttpResponse{200};
        });

        // TODO: Make the port configurable
        server.startListening(8572);
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_INFO("got error: %s\n", e.what());
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
        DEBUG_FUNCTION_LINE_INFO("got error: %s\n", e.what());
    }
}

void enableServerChanged(ConfigItemBoolean *item, bool newValue) {
    // If the value has changed, we store it in the storage.
    if (newValue != enableServer)
        WUPSStorageAPI::Store(ENABLE_SERVER_CONFIG_ID, newValue);

    enableServer = newValue;
}

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    // To use the C++ API, we create new WUPSConfigCategory from the root handle!
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

    // The functions of the Config API come in two variants: One that throws an exception, and another one which doesn't
    // To use the Config API without exception see the example below this try/catch block.
    try {
        root.add(WUPSConfigItemBoolean::Create(ENABLE_SERVER_CONFIG_ID, "Enable Server", ENABLE_SERVER_DEFAULT_VALUE, enableServer, enableServerChanged));
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

ssize_t
write_to_log(_reent*, void*, const char* ptr, size_t len)
{
    try {
        // only way to guarantee it's null-terminated
        std::string buf{ptr, len};
        if (!WHBLogWrite(buf.c_str()))
            return -1;
        return buf.size();
    }
    catch (...) {
        return -1;
    }
}


__attribute__((__constructor__))
void
init_stdio()
{
    static devoptab_t dev_out;
    dev_out.name = "stdout";
    dev_out.write_r = write_to_log;
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
    if ((storageRes = WUPSStorageAPI::SaveStorage()) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("SaveStorage failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }

    // if (!enableServer || server_made) return;
    // make_server_on_thread();
}

// Gets called when the plugin will be unloaded.
DEINITIALIZE_PLUGIN() {
    stop_server();
    DEBUG_FUNCTION_LINE("Ristretto deinitializing.");
    WHBLogUdpDeinit();
    WHBLogCafeDeinit();
}

ON_APPLICATION_START()
{
    nn::ac::Initialize();
	nn::ac::ConnectAsync();
    if(!enableServer) return;
    make_server_on_thread();
}

ON_APPLICATION_ENDS()
{
    if(!enableServer) return;
    stop_server();
}
