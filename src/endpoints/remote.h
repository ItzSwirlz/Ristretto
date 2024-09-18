#include "utils/logger.h"
#include <http/http.hpp>

static int button_value = 0;

void registerRemoteEndpoints(HttpServer &server);
