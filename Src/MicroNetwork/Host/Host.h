#pragma once

#include <memory>
#include <vector>
#include <MicroNetwork/Common/DataStream.h>
#include <MicroNetwork/Common/IDataReceiver.h>
#include <LFramework/USB/Host/UsbHDevice.h>
#include <LFramework/Threading/Semaphore.h>
#include <LFramework/Threading/CriticalSection.h>
#include <MicroNetwork/Common/Packet.h>
#include <LFramework/Assert.h>
#include <thread>
#include <mutex>
#include <functional>
#include <LFramework/Debug.h>
#include <LFramework/Guid.h>
#include <MicroNetwork/Host/NodeContext.h>

namespace MicroNetwork::Host {

class Host : public Common::DataStream {
public:
    Host(std::string path, std::shared_ptr<DataStream> remoteStream) : _remoteStream(remoteStream), _path(path) {
        remoteStream->bind(this);
        bind(remoteStream.get());
    }
    bool start() override {
        _txAvailable.give();
        return true;
    }

    ~Host(){

    }

    LFramework::ComPtr<Common::IDataReceiver> startTask(std::uint32_t node, LFramework::ComPtr<Common::IDataReceiver> userDataReceiver){
        if(_node != nullptr){
            return _node->startTask(userDataReceiver);
        }
        return nullptr;
    }

    std::vector<std::uint32_t> getNodes() {
        std::vector<std::uint32_t> result;
        std::lock_guard<std::mutex> lock(_nodeContextMutex);
        if(_node != nullptr) {
            result.push_back(_node->getVirtualId());
        }
        return result;
    }

    std::uint32_t getState() {
        return _state;
    }

    bool blockingWritePacket(Common::PacketHeader header, const void* data) {
        while(true){
            _txAvailable.take();
            LFramework::Threading::CriticalSection lock;
            if(freeSpace() >= (sizeof(header) + header.size)){
                write(&header, sizeof(header));
                write(data, header.size);
                return true;
            }
        }
    }

    const std::string& getPath() const {
        return _path;
    }
protected:
    std::shared_ptr<DataStream> _remoteStream;
    std::string _path;
    std::atomic<std::uint32_t> _state = 0;
    std::mutex _nodeContextMutex;
    NodeContext* _node = nullptr;

    bool readPacket(Common::MaxPacket& packet) {
        LFramework::Threading::CriticalSection lock;
        if(_remote == nullptr){
            return false;
        }
        if(!_remote->peek(&packet.header, sizeof(packet.header))){
            return false;
        }

        auto packetFullSize = sizeof(packet.header) + packet.header.size;

        if(_remote->bytesAvailable() < packetFullSize){
            return false;
        }

        lfAssert(_remote->read(&packet, packetFullSize) == packetFullSize);
        return true;
    }

    void onRemoteReset() override {
        //Send bind packet
        Common::MaxPacket packet;
        packet.header.id = Common::PacketId::Bind;
        packet.header.size = 0;
        write(&packet, sizeof(packet.header));
    }
    void onRemoteDataAvailable() override {
        Common::MaxPacket packet;
        if(readPacket(packet)){
            if(packet.header.id == Common::PacketId::Bind){
                std::lock_guard<std::mutex> lock(_nodeContextMutex);

                lfDebug() << "Bind response received";
                lfAssert(_node == nullptr);
                lfAssert(packet.header.size == 4);
                std::uint32_t tasksCount;
                memcpy(&tasksCount, packet.payload.data(), sizeof(tasksCount));
                _node = new NodeContext(0, 0, tasksCount, this);
                lfDebug() << "Node context created";
                _state++;

            }else if(_node != nullptr){
                if(packet.header.id == Common::PacketId::TaskDescription){
                    lfAssert(packet.header.size == sizeof(LFramework::Guid));
                    LFramework::Guid taskId;
                    memcpy(&taskId, packet.payload.data(), sizeof(taskId));
                    _node->addTask(taskId);
                    lfDebug() << "Received task ID";
                }else{
                    _node->handleNetworkPacket(packet.header, packet.payload.data());
                }
            }else{
                lfDebug() << "Drop packet";
            }
        }
    }
    void onReadBytes() override {
        _txAvailable.give();
    }

private:

    LFramework::Threading::BinarySemaphore _txAvailable;
};

}
