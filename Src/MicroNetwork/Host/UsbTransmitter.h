#pragma once

#include <memory>
#include <vector>
#include <MicroNetwork/Common/DataStream.h>
#include <LFramework/USB/Host/IUsbDevice.h>
#include <LFramework/Threading/Semaphore.h>
#include <LFramework/Threading/CriticalSection.h>
#include <thread>
#include <functional>
#include <LFramework/Debug.h>

namespace MicroNetwork::Host {

class UsbTransmitter : public Common::DataStream {
public:
    UsbTransmitter(std::shared_ptr<LFramework::USB::IUsbDevice> device) : _device(device) {
        auto usbInterface = _device->getInterface(0);
        _txEndpoint = usbInterface->getEndpoint(false, 0);
        _rxEndpoint = usbInterface->getEndpoint(true, 0);
    }

    ~UsbTransmitter() {
        _running = false;
        if(_rxThread.joinable()){
            _rxJob.give();
            _rxThread.join();
        }
        if(_txThread.joinable()){
             _txJob.give();
            _txThread.join();
        }
    }


    bool start() override {
        //Fill read chain
        for (size_t i = 0; i < 8; ++i) {
            auto chainItem = std::make_shared<ReadChainItem>(_rxEndpoint->getDescriptor().wMaxPacketSize);
            chainItem->readAsync(_rxEndpoint);
            _readChain.push_back(chainItem);
        }

        reset();
        _running = true;

        _rxThread = std::thread(std::bind(&UsbTransmitter::rxThreadHandler, this));
        _txThread = std::thread(std::bind(&UsbTransmitter::txThreadHandler, this));

        lfDebug() << "USB transmitter started";
        return true;
    }

    bool running() const {
        return _running;
    }
private:
    struct ReadChainItem {
       explicit ReadChainItem(size_t bufferSize) {
           buffer.resize(bufferSize);
       }
       std::shared_ptr<LFramework::USB::IUsbTransfer> asyncResult;
       std::vector<uint8_t> buffer;

       void readAsync(LFramework::USB::IUsbHostEndpoint* ep) {
            asyncResult = ep->transferAsync(buffer.data(), buffer.size());
       }
    };

    void onRemoteDisconnect() override {

    }
    void onRemoteReset() override {

    }
    void onRemoteDataAvailable() override {
        _txJob.give();
    }
    void onReadBytes() override {
        _rxJob.give();
    }

    size_t usbTransmit(void* data, uint32_t size) {
        auto result = _txEndpoint->transferAsync(data, size)->wait();
        return result;
    }

    void sendSyncPacket() {
        usbTransmit(nullptr, 0);
    }

    void rxThreadHandler() {
        while(_running){
            try {
                for(auto& item : _readChain){
                    auto rxSize = item->asyncResult->wait();

                    //lfDebug() << "Received USB packet: size=" << rxSize;
                    if(rxSize == 0){
                        _synchronized = true;
                        //lfDebug() << "Sync received";
                    }else {
                        if(_synchronized) {
                            std::size_t doneRxSize = 0;
                            while(true){
                                doneRxSize += write(item->buffer.data() + doneRxSize, rxSize - doneRxSize);
                                if(_running && (doneRxSize != rxSize)){
                                    lfDebug() << "RX buffer stall";
                                    _rxJob.take();
                                }else{
                                    break;
                                }
                            }
                        }else{
                            //lfDebug() << "USB transmitter drop packet" << rxSize;
                        }
                    }

                    if(_running){
                        item->readAsync(_rxEndpoint);
                    }
                }

            }catch (std::exception& ex){
                _running = false;
            }catch(...){
                _running = false;
            }
        }
        notifyDisconnect();
    }

    void txThreadHandler() {
        std::vector<uint8_t> txBuffer;
        txBuffer.resize(_txEndpoint->getDescriptor().wMaxPacketSize);

        try {
            sendSyncPacket();
            lfDebug() << "Sync sent";
            while(_running){
                _txJob.take();

                while(true){
                    auto size = _remote->read(txBuffer.data(), txBuffer.size());
                    if(size == 0){
                        break;
                    }

                    if(usbTransmit(txBuffer.data(), size) != size){
                        throw std::runtime_error("USB TX fail");
                    }else{
                        lfDebug() << "USB transmitted packet: " << size;
                    }
                }

            }
        } catch (const std::exception & ex) {
            lfDebug() << "TX thread exception: " << ex.what();
            _running = false;
        }
        notifyDisconnect();
    }


    bool _synchronized = false;

    bool _running = false;
    std::thread _rxThread;
    std::thread _txThread;

    LFramework::Threading::BinarySemaphore _rxJob;
    LFramework::Threading::BinarySemaphore _txJob;
    LFramework::USB::IUsbHostEndpoint* _txEndpoint = nullptr;
    LFramework::USB::IUsbHostEndpoint* _rxEndpoint = nullptr;
    std::shared_ptr<LFramework::USB::IUsbDevice> _device;
    std::vector<std::shared_ptr<ReadChainItem>> _readChain;
};

}
