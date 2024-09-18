#include "utils/logger.h"
#include <http/http.hpp>

static uint8_t vpad_battery = 0;

void registerGamepadEndpoints(HttpServer &server);
