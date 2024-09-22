#include "switch.h"

void registerSwitchEndpoints(HttpServer &server) {
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

    // Switch to the home button menu.
    // FIXME: This actually causes issues, like when trying to call this while
    // the menu is already open - generally all HBM/overlayed applications
    // because of ProcUI magic or something that I still need to understand.
    // There are weird behaviors that need to be assessed in general about
    // launching and switching titles and applications.
    server.when("/switch/hbm")->posted([](const HttpRequest &req) {
        _SYSSwitchToHBMWithMode(0);
        return HttpResponse{200};
    });
}
