#include "../utils/logger.h"
#include "http.hpp"

static uint8_t vpad_battery = 0;

void registerGamepadEndpoints(HttpServer &server);
