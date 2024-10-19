#include "device.h"

#include <avm/cec.h>
#include <tve/cec.h>

void registerCECEndpoints(HttpServer &server) {
    server.when("/tve/cec/enabled")->requested([](const HttpRequest &req) {
        return HttpResponse{200, "text/plain", std::to_string(TVEIsCECEnable())};
    });

    server.when("/tve/cec/vol_up")->posted([](const HttpRequest &req) {
        uint8_t params = 65;
        bool b         = TVECECSendCommand(TVE_CEC_DEVICE_TV, TVE_CEC_OPCODE_USER_CONTROL_PRESSED, &params, 1);
        return HttpResponse{200, "text/plain", std::to_string(b)};
    });

    server.when("/tve/cec/tvo")->posted([](const HttpRequest &req) {
        uint8_t params = 0;
        bool b         = TVECECSendCommand(TVE_CEC_DEVICE_TV, TVE_CEC_OPCODE_TEXT_VIEW_ON, &params, 0);
        return HttpResponse{200, "text/plain", std::to_string(b)};
    });

    server.when("/tve/cec/reqactive")->posted([](const HttpRequest &req) {
        // Request the physical address
        uint8_t params = 0;
        bool b         = TVECECSendCommand(TVE_CEC_DEVICE_TV, TVE_CEC_OPCODE_GIVE_PHYSICAL_ADDRESS, &params, 0);

        // See what we got back for that address
        TVECECLogicalAddress outInitiator;
        TVECECOpCode outOpCode;
        uint8_t tvAddress;
        uint8_t outNumParams;
        TVECECReceiveCommand(&outInitiator, &outOpCode, &tvAddress, &outNumParams);

        // Request we turn on TV
        TVECECSendCommand(TVE_CEC_DEVICE_TV, TVE_CEC_OPCODE_TEXT_VIEW_ON, &params, 0);

        // Switch to our source
        b = TVECECSendCommand(TVE_CEC_DEVICE_TV, TVE_CEC_OPCODE_ACTIVE_SOURCE, &tvAddress, 1);

        return HttpResponse{200, "text/plain", std::to_string(b)};
    });

    server.when("/tve/cec/latest")->requested([](const HttpRequest &req) {
        TVECECLogicalAddress outInitiator;
        TVECECOpCode outOpCode;
        uint8_t outParams;
        uint8_t outNumParams;

        TVECECReceiveCommand(&outInitiator, &outOpCode, &outParams, &outNumParams);
        std::string ret = std::format("initiator {:d} opcode {:d} outParams {:d}", (int) outInitiator, (int) outOpCode, outParams);
        return HttpResponse{200, "text/plain", ret};
    });
}
