#include "../endpoints/device.h"
#include "../endpoints/launch.h"
#include "../endpoints/power.h"
#include "../endpoints/switch.h"
#include "../endpoints/title.h"
#include "../utils/logger.h"
#include <coreinit/cache.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/systeminfo.h>
#include "http.hpp"
#include <nn/ac.h>

#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_console.h>

#include <thread>

HttpServer server;
bool server_made = false;

int
create_server()
{
    if (server_made) {
        return 0;
    }

    server_made = true;
    DEBUG_FUNCTION_LINE("Server started.");

    try {
        // Empty endpoint to allow for device discovery.
        server.when("/")->requested([](const HttpRequest &req) {
            return HttpResponse{200, "text/plain", "Ristretto (ECO)"};
        });

        registerDeviceEndpoints(server);
        registerLaunchEndpoints(server);
        registerPowerEndpoints(server);
        registerSwitchEndpoints(server);
        registerTitleEndpoints(server);

        // these dont apply in ECO mode (FIXME: remote might?)
        // registerGamepadEndpoints(server);
        // registerRemoteEndpoints(server);

        // TODO: Make the port configurable
        server.startListening(8572);
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_INFO("Exception thrown in the HTTP server: %s\n", e.what());
    }

    return 0;
}

int
main(int argc, char **argv)
{
    nn::ac::Initialize();
    nn::ac::ConnectAsync();
    WHBProcInit();
    WHBLogConsoleInit();

    try {
        std::thread t(create_server);
        t.join();
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_INFO("Exception thrown trying to make the server thread: %s\n", e.what());
    }

    nn::ac::Finalize();
    WHBLogConsoleFree();
    WHBProcShutdown();
   return 0;
}
