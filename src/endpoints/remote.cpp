#include "remote.h"

void registerRemoteEndpoints(HttpServer &server) {
    // To prevent potential bad behavior, only allow for specific buttons.

    // A - 0x8000
    server.when("/remote/key/a")->posted([](const HttpRequest &req) {
        button_value = 0x8000;
        return HttpResponse{200};
    });

    // B - 0x4000
    server.when("/remote/key/b")->posted([](const HttpRequest &req) {
        button_value = 0x4000;
        return HttpResponse{200};
    });

    // Left - 0x0800
    server.when("/remote/key/left")->posted([](const HttpRequest &req) {
        button_value = 0x0800;
        return HttpResponse{200};
    });

    // Right - 0x0400
    server.when("/remote/key/right")->posted([](const HttpRequest &req) {
        button_value = 0x0400;
        return HttpResponse{200};
    });

    // Up - 0x0200
    server.when("/remote/key/up")->posted([](const HttpRequest &req) {
        button_value = 0x0200;
        return HttpResponse{200};
    });

    // Down - 0x0100
    server.when("/remote/key/down")->posted([](const HttpRequest &req) {
        button_value = 0x0100;
        return HttpResponse{200};
    });
}
