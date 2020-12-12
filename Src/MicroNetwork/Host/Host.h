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

class INodeContainer {
public:
    virtual ~INodeContainer() = default;
    virtual void addNode(std::shared_ptr<NodeContext> node) = 0;
    virtual void removeNode(std::shared_ptr<NodeContext> node) = 0;
};

class Host : public Common::DataStream {
public:
    Host(std::string path, std::shared_ptr<DataStream> remoteStream, INodeContainer* nodeContainer) : _remoteStream(remoteStream), _path(path), _nodeContainer(nodeContainer) {
        remoteStream->bind(this);
        bind(remoteStream.get());
    }
    bool start() override {
        _txAvailable.give();
        return true;
    }

    ~Host(){
        notifyDisconnect();
        clearNodes();
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

    bool isConnected() {
        return _connected;
    }

    const std::string& getPath() const {
        return _path;
    }
protected:
    std::shared_ptr<DataStream> _remoteStream;
    std::string _path;
    std::atomic<std::uint32_t> _state = 0;
    std::mutex _nodeContextMutex;
    std::unordered_map<std::uint8_t, std::shared_ptr<NodeContext>> _nodes;

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

    void onRemoteDisconnect() override {
        _connected = false;
    }

    void clearNodes() {
        for(auto node : _nodes){
            _nodeContainer->removeNode(node.second);
        }
        _nodes.clear();
    }

    void addNode(std::uint8_t nodeId, std::shared_ptr<NodeContext> node) {
        if(_nodes.find(nodeId) != _nodes.end()){
            lfDebug() << "Node exists!!!!";
            for(;;);
        }
        _nodes[nodeId] = node;
        _nodeContainer->addNode(node);
    }

    std::shared_ptr<NodeContext> getNode(std::uint8_t nodeId){
        auto resultIt = _nodes.find(nodeId);
        if(resultIt == _nodes.end()){
            return nullptr;
        }
        return resultIt->second;
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

                lfAssert(packet.header.size == 4);
                std::uint32_t tasksCount;
                memcpy(&tasksCount, packet.payload.data(), sizeof(tasksCount));

                auto nodeId = 0;
                auto nodeContext = std::make_shared<NodeContext>(nodeId, tasksCount, this);
                addNode(nodeId, nodeContext);
                lfDebug() << "Node context created";
                _state++;

            }else{
                auto nodeId = 0;
                auto node = getNode(nodeId);
                if(node != nullptr){
                    if(packet.header.id == Common::PacketId::TaskDescription){
                        lfAssert(packet.header.size == sizeof(LFramework::Guid));
                        LFramework::Guid taskId;
                        memcpy(&taskId, packet.payload.data(), sizeof(taskId));
                        node->addTask(taskId);
                        lfDebug() << "Received task ID";
                    }else{
                        node->handleNetworkPacket(packet.header, packet.payload.data());
                    }
                }else{
                    lfDebug() << "Drop packet";
                }
            }
        }
    }
    void onReadBytes() override {
        _txAvailable.give();
    }

private:
    bool _connected = true;
    INodeContainer* _nodeContainer = nullptr;
    LFramework::Threading::BinarySemaphore _txAvailable;
};

}
