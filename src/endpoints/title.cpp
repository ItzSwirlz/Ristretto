#include "title.h"
#include "../languages.h" // for access to titleLang

#include <rpxloader/rpxloader.h>

inline char *getTitleLongname(ACPMetaXml *meta) {
    char *ret;
    switch (titleLang) {
        case LANG_JAPANESE:
            ret = meta->longname_ja;
            break;
        case LANG_FRENCH:
            ret = meta->longname_fr;
            break;
        case LANG_GERMAN:
            ret = meta->longname_de;
            break;
        case LANG_ITALIAN:
            ret = meta->longname_it;
            break;
        case LANG_SPANISH:
            ret = meta->longname_es;
            break;
        case LANG_SIMPLIFIED_CHINESE:
            ret = meta->longname_zhs;
            break;
        case LANG_KOREAN:
            ret = meta->longname_ko;
            break;
        case LANG_DUTCH:
            ret = meta->longname_nl;
            break;
        case LANG_PORTUGUESE:
            ret = meta->longname_pt;
            break;
        case LANG_RUSSIAN:
            ret = meta->longname_ru;
            break;
        case LANG_TRADITIONAL_CHINESE:
            ret = meta->longname_zht;
            break;
        case LANG_ENGLISH:
        default:
            ret = meta->longname_en;
            break;
    }

    // Fallback for titles which don't have a language-specific translation
    if (ret != NULL && ret[0] == '\0') ret = meta->longname_en;
    return ret;
}

void registerTitleEndpoints(HttpServer &server) {
    // Returns the current title - for both Wii U applications and homebrew.
    // The two are together so integrations can only refer to one endpoint.
    server.when("/title/current")->requested([](const HttpRequest &req) {
        // Check if we are running a Homebrew application
        // Preallocate a string for the path
        std::string hb_path(1024, '\0');
        int size    = 1024;
        int rpx_res = RPXLoader_GetPathOfRunningExecutable((char *) hb_path.c_str(), size);
        if (rpx_res == RPX_LOADER_RESULT_SUCCESS) {
            // Resize the string to save memory
            hb_path.resize(strlen(hb_path.c_str()));
            return HttpResponse{200, "text/plain", hb_path};
        }

        // Therefore we are running an actual application.
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
            //
            // MCP_APP_TYPE_ACCOUNT_APPS do not work: these things like notifications, account settings,
            // user settings, etc. will not launch or throw an error. (System Transfer for some reason
            // is in this category??? But for console security it should not be exposed anyways).
            if (title.appType == MCP_APP_TYPE_GAME ||
                title.appType == MCP_APP_TYPE_GAME_WII ||
                title.appType == MCP_APP_TYPE_SYSTEM_MENU ||
                title.appType == MCP_APP_TYPE_SYSTEM_APPS ||
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
}
