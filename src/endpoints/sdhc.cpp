#include "sdhc.h"
#include <sdutils/sdutils.h>

void registerSDHCEndpoints(HttpServer &server) {
    server.when("/sdhc/mounted")->requested([](const HttpRequest &req) {
        bool mounted;
        SDUtils_IsSdCardMounted(&mounted);

        std::string ret = std::format("{:d}", mounted);
        return HttpResponse{200, "text/plain", ret};
    });
}
