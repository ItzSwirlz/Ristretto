#include "utils/logger.h"
#include <coreinit/filesystem.h>
#include <coreinit/launch.h>
#include <coreinit/mcp.h>
#include <coreinit/thread.h>
#include <cstddef>
#include <http/http.hpp>
#include <malloc.h>
#include <nn/ac.h>
#include <nn/act.h>
#include <sysapp/launch.h>
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
WUPS_PLUGIN_NAME("SmartEspresso")
WUPS_PLUGIN_DESCRIPTION("home automation attempt");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("ItzSwirlz");
WUPS_PLUGIN_LICENSE("BSD");

#define LOG_FS_OPEN_CONFIG_ID             "logFSOpen"
#define OTHER_EXAMPLE_BOOL_CONFIG_ID      "otherBoolItem"
#define OTHER_EXAMPLE2_BOOL_CONFIG_ID     "other2BoolItem"
#define INTEGER_RANGE_EXAMPLE_CONFIG_ID   "intRangeExample"
#define MULTIPLE_VALUES_EXAMPLE_CONFIG_ID "multValueExample"

/**
    All of this defines can be used in ANY file.
    It's possible to split it up into multiple files.

**/

WUPS_USE_WUT_DEVOPTAB();           // Use the wut devoptabs
WUPS_USE_STORAGE("smartespresso"); // Unique id for the storage api

enum ExampleOptions {
    EXAMPLE_OPTION_1 = 0,
    EXAMPLE_OPTION_2 = 1,
    EXAMPLE_OPTION_3 = 2,
};

#define LOF_FS_OPEN_DEFAULT_VALUE     true
#define INTEGER_RANGE_DEFAULT_VALUE   10
#define MULTIPLE_VALUES_DEFAULT_VALUE EXAMPLE_OPTION_2

bool sLogFSOpen                    = LOF_FS_OPEN_DEFAULT_VALUE;
int sIntegerRangeValue             = INTEGER_RANGE_DEFAULT_VALUE;
ExampleOptions sExampleOptionValue = MULTIPLE_VALUES_DEFAULT_VALUE;
HttpServer server;
bool server_made = false;
static std::vector<std::string> messages;

/**
 * Callback that will be called if the config has been changed
 */
void boolItemChanged(ConfigItemBoolean *item, bool newValue) {
    DEBUG_FUNCTION_LINE_INFO("New value in boolItemChanged: %d", newValue);
    if (std::string_view(LOG_FS_OPEN_CONFIG_ID) == item->identifier) {
        sLogFSOpen = newValue;
        // If the value has changed, we store it in the storage.
        WUPSStorageAPI::Store(item->identifier, newValue);
    } else if (std::string_view(OTHER_EXAMPLE_BOOL_CONFIG_ID) == item->identifier) {
        DEBUG_FUNCTION_LINE_ERR("Other bool value has changed to %d", newValue);
    } else if (std::string_view(OTHER_EXAMPLE2_BOOL_CONFIG_ID) == item->identifier) {
        DEBUG_FUNCTION_LINE_ERR("Other2 bool value has changed to %d", newValue);
    }
}

void integerRangeItemChanged(ConfigItemIntegerRange *item, int newValue) {
    DEBUG_FUNCTION_LINE_INFO("New value in integerRangeItemChanged: %d", newValue);
    // If the value has changed, we store it in the storage.
    if (std::string_view(LOG_FS_OPEN_CONFIG_ID) == item->identifier) {
        sIntegerRangeValue = newValue;
        // If the value has changed, we store it in the storage.
        WUPSStorageAPI::Store(item->identifier, newValue);
    }
}

void multipleValueItemChanged(ConfigItemIntegerRange *item, uint32_t newValue) {
    DEBUG_FUNCTION_LINE_INFO("New value in multipleValueItemChanged: %d", newValue);
    // If the value has changed, we store it in the storage.
    if (std::string_view(MULTIPLE_VALUES_EXAMPLE_CONFIG_ID) == item->identifier) {
        sExampleOptionValue = (ExampleOptions) newValue;
        // If the value has changed, we store it in the storage.
        WUPSStorageAPI::Store(item->identifier, sExampleOptionValue);
    }
}

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    // To use the C++ API, we create new WUPSConfigCategory from the root handle!
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

    // The functions of the Config API come in two variants: One that throws an exception, and another one which doesn't
    // To use the Config API without exception see the example below this try/catch block.
    try {
        // Then we can simply create a new category
        auto functionPatchesCat = WUPSConfigCategory::Create("function patches");

        // Add a boolean item to this newly created category
        functionPatchesCat.add(WUPSConfigItemBoolean::Create(LOG_FS_OPEN_CONFIG_ID, "Log FSOpen calls",
                                                             LOF_FS_OPEN_DEFAULT_VALUE, sLogFSOpen,
                                                             boolItemChanged));

        // And finally move that category to the root category.
        // Note: "functionPatchesCat" can NOT be changed after adding it to root.
        root.add(std::move(functionPatchesCat));

        // We can also add items directly to root!
        root.add(WUPSConfigItemBoolean::Create(OTHER_EXAMPLE_BOOL_CONFIG_ID, "Just another bool item",
                                               false, false,
                                               boolItemChanged));

        // You can also add an item which just displays any text.
        root.add(WUPSConfigItemStub::Create("This item is just displaying some text"));

        // It's also possible to create and item to select an integer from a range.
        root.add(WUPSConfigItemIntegerRange::Create(INTEGER_RANGE_EXAMPLE_CONFIG_ID, "Item for selecting an integer between 0 and 50",
                                                    INTEGER_RANGE_DEFAULT_VALUE, sIntegerRangeValue,
                                                    0, 50,
                                                    &integerRangeItemChanged));


        // To select value from an enum WUPSConfigItemMultipleValues fits the best.
        constexpr WUPSConfigItemMultipleValues::ValuePair possibleValues[] = {
                {EXAMPLE_OPTION_1, "Option 1"},
                {EXAMPLE_OPTION_2, "Option 2"},
                {EXAMPLE_OPTION_3, "Option 3"},
        };

        // It comes in two variants.
        // - "WUPSConfigItemMultipleValues::CreateFromValue" will take a default and current **value**
        // - "WUPSConfigItemMultipleValues::CreateFromIndex" will take a default and current **index**
        root.add(WUPSConfigItemMultipleValues::CreateFromValue(MULTIPLE_VALUES_EXAMPLE_CONFIG_ID, "Select an option!",
                                                               MULTIPLE_VALUES_DEFAULT_VALUE, sExampleOptionValue,
                                                               possibleValues,
                                                               nullptr));

        // It's also possible to have nested categories
        auto nc1 = WUPSConfigCategory::Create("Category inside root");
        auto nc2 = WUPSConfigCategory::Create("Category inside subcategory 1");
        auto nc3 = WUPSConfigCategory::Create("Category inside subcategory 2");

        nc3.add(WUPSConfigItemStub::Create("Item inside subcategory 3"));
        nc2.add(WUPSConfigItemStub::Create("Item inside subcategory 2"));
        nc1.add(WUPSConfigItemStub::Create("Item inside subcategory 1"));

        nc2.add(std::move(nc3));
        nc1.add(std::move(nc2));
        root.add(std::move(nc1));
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_ERR("Creating config menu failed: %s", e.what());
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    // In case we don't like exception, we can use the API as well.
    // If we add a "WUPSConfigAPIStatus" reference to the API calls, the function won't throw an exception.
    // Instead it will return std::optionals and write the result into the WUPSConfigAPIStatus.
    WUPSConfigAPIStatus err;
    auto categoryOpt = WUPSConfigCategory::Create("Just another Category", err);
    if (!categoryOpt) {
        DEBUG_FUNCTION_LINE_ERR("Failed to create category: %s", WUPSConfigAPI_GetStatusStr(err));
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    auto boolItemOpt = WUPSConfigItemBoolean::Create(OTHER_EXAMPLE2_BOOL_CONFIG_ID, "Just another bool item",
                                                     false, false,
                                                     boolItemChanged,
                                                     err);
    if (!boolItemOpt) {
        DEBUG_FUNCTION_LINE_ERR("Failed to create bool item: %s", WUPSConfigAPI_GetStatusStr(err));
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    // Add bool item to category
    if (!categoryOpt->add(std::move(*boolItemOpt), err)) {
        DEBUG_FUNCTION_LINE_ERR("Failed to add bool item to category: %s", WUPSConfigAPI_GetStatusStr(err));
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    // Add category to root.
    if (!root.add(std::move(*categoryOpt), err)) {
        DEBUG_FUNCTION_LINE_ERR("Failed to add category to root: %s", WUPSConfigAPI_GetStatusStr(err));
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();
}

void make_server() {
    try {

        // Empty endpoint to allow for device discovery.
        server.when("/")->posted([](const HttpRequest &req) {
            return HttpResponse{200};
        });

        // Shutsdown the console regardless of what state it currently is in.
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
        server.when("/serial")->posted([](const HttpRequest &req) {
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

        // TODO: Make the port configurable
        server.startListening(8572);
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_INFO("got error: %s\n", e.what());
    }
}

void stop_server() {
    server.shutdown();
}

/**
    Gets called ONCE when the plugin was loaded.
**/
INITIALIZE_PLUGIN() {
    // Logging only works when compiled with `make DEBUG=1`. See the README for more information.
    initLogging();
    DEBUG_FUNCTION_LINE("Hello world! - SmartEspresso");
    try {
        std::jthread thready(make_server);
        thready.detach();
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_INFO("got error: %s\n", e.what());
    }

    WUPSConfigAPIOptionsV1 configOptions = {.name = "example_plugin_cpp"};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback) != WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to init config api");
    }

    WUPSStorageError storageRes;
    if ((storageRes = WUPSStorageAPI::GetOrStoreDefault(LOG_FS_OPEN_CONFIG_ID, sLogFSOpen, LOF_FS_OPEN_DEFAULT_VALUE)) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("GetOrStoreDefault failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }
    if ((storageRes = WUPSStorageAPI::SaveStorage()) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("GetOrStoreDefault failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }
}

/**
    Gets called when the plugin will be unloaded.
**/
DEINITIALIZE_PLUGIN() {
    stop_server();
    deinitLogging();
    DEBUG_FUNCTION_LINE("DEINITIALIZE_PLUGIN of example_plugin!");
}

/**
    Gets called when an application starts.
**/
ON_APPLICATION_START() {
    initLogging();
}

/**
 * Gets called when an application actually ends
 */
//ON_APPLICATION_ENDS() {}

/**
    Gets called when an application request to exit.
**/
//ON_APPLICATION_REQUESTS_EXIT() {
//    DEBUG_FUNCTION_LINE_INFO("ON_APPLICATION_REQUESTS_EXIT of example_plugin!");
//}
