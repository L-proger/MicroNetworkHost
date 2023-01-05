#pragma once

#include <MicroNetwork/Host/LinkProvider.h>
#include <optional>
#include <LFramework/USB/Host/IUsbService.h>
#include <LFramework/USB/Host/IUsbDevice.h>
#include <MicroNetwork/Host/UsbTransmitter.h>

namespace MicroNetwork::Host {

class UsbLinkProvider : public LinkProvider {
public:
    UsbLinkProvider(std::optional<std::uint16_t> vid, std::optional<std::uint16_t> pid, ILinkCallback* linkCallback) : LinkProvider(linkCallback),
     _vid(vid), _pid(pid){
        _usbService = std::shared_ptr<LFramework::USB::IUsbService>(LFramework::USB::createUsbService());
        _usbService->startEventsListening(std::bind(&UsbLinkProvider::onUsbDevicesChange, this));
    }

    ~UsbLinkProvider(){
        _usbService->stopEventsListening();
    }
private:
    std::optional<std::uint16_t> _vid;
    std::optional<std::uint16_t> _pid;

    void onUsbDevicesChange() {
        onLinksUpdated();
    }

    std::vector<std::string> getLinks() override {
        std::vector<std::string> result;
        auto devices = _usbService->enumerateDevices();
        for(auto& device : devices){
            if(!_vid.has_value() || (_vid.value() == device.vid)){
                if(!_pid.has_value() || (_pid.value() == device.pid)){
                    result.push_back(device.path);
                }
            }
        }
        return result;
    }

    std::shared_ptr<Common::DataStream> makeStream(const std::string& linkPath) override {
        std::shared_ptr<LFramework::USB::IUsbDevice> device(LFramework::USB::openUsbDevice(linkPath));
        return std::make_shared<Host::UsbTransmitter>(device);
    }

    std::shared_ptr<LFramework::USB::IUsbService> _usbService;
};

}
