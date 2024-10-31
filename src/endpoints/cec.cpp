#include "device.h"

#include <avm/cec.h>
#include <tve/cec.h>

void registerCECEndpoints(HttpServer &server) {
    // Returns whether CEC is enabled and running.
    server.when("/cec/enabled")->requested([](const HttpRequest &req) {
        return HttpResponse{200, "text/plain", std::to_string(TVEIsCECEnable())};
    });

    // Get the latest CEC request
    server.when("/cec/latest")->requested([](const HttpRequest &req) {
        TVECECLogicalAddress outInitiator;
        TVECECOpCode outOpCode;
        uint8_t outNumParams;
        std::vector<uint8_t> outParams(100);

        TVECECReceiveCommand(&outInitiator, &outOpCode, outParams.data(), &outNumParams);
        std::string ret = std::format("initiator {:d} opcode {:d} numparams {:d}", (uint) outInitiator, (uint) outOpCode, (uint) outNumParams);
        for (int i = 0; i < outNumParams; i++) {
            ret.append(std::format("\n param {:d} - {:d}", i, outParams.at(i)));
        }

        return HttpResponse{200, "text/plain", ret};
    });

    server.when("/cec/request_tv_active")->posted([](const HttpRequest &req) {
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

    server.when("/cec/request_tv_on")->posted([](const HttpRequest &req) {
        uint8_t params = 0;
        bool b         = TVECECSendCommand(TVE_CEC_DEVICE_TV, TVE_CEC_OPCODE_TEXT_VIEW_ON, &params, 0);
        return HttpResponse{200, "text/plain", std::to_string(b)};
    });

    server.when("/cec/tv_vol_down")->posted([](const HttpRequest &req) {
        uint8_t params = 66;
        bool b         = TVECECSendCommand(TVE_CEC_DEVICE_TV, TVE_CEC_OPCODE_USER_CONTROL_PRESSED, &params, 1);
        return HttpResponse{200, "text/plain", std::to_string(b)};
    });

    server.when("/cec/tv_vol_up")->posted([](const HttpRequest &req) {
        uint8_t params = 65;
        bool b         = TVECECSendCommand(TVE_CEC_DEVICE_TV, TVE_CEC_OPCODE_USER_CONTROL_PRESSED, &params, 1);
        return HttpResponse{200, "text/plain", std::to_string(b)};
    });
}
