#include "endpoints/remote.h"

void registerRemoteEndpoints(HttpServer &server) {
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
}
