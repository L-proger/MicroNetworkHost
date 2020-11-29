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
#include <MicroNetwork/Host/TaskContext.h>

namespace MicroNetwork::Host {

class Host;


class TaskContextConstructor {
public:
    void finalize(bool success){
        _success = success;
        _completionSemaphore.give();
    }
    bool wait() {
        _completionSemaphore.take();
        return _success;
    }
private:
    bool _success = false;
    LFramework::Threading::BinarySemaphore _completionSemaphore;
};

class NodeContext {
public:
    NodeContext(std::uint8_t realId, std::uint32_t virtualId, std::uint32_t tasksCount, Host* host) : _realId(realId), _virtualId(virtualId), _tasksCount(tasksCount), _host(host){

    }
    void handleNetworkPacket(Common::PacketHeader header, const void* data) {
        lfDebug() << "Node context received packet: id=" << header.id << " size=" << header.size;

        if(header.id == Common::PacketId::TaskStop){
            if(_currentTask != nullptr){
                _currentTask->onTaskStopped();
                _currentTask.reset();
            }
        }else if(header.id == Common::PacketId::TaskStart){
            std::lock_guard<std::recursive_mutex> lock(_taskMutex);
            if(_nextTask != nullptr){
                _nextTask->finalize(true);
            }
        }else{
            std::lock_guard<std::recursive_mutex> lock(_taskMutex);
            _currentTask->handleNetworkPacket(header, data);
        }
    }
    bool handleUserPacket(Common::PacketHeader header, const void* data);
    std::uint8_t getRealId() const {
        return _realId;
    }
    std::uint32_t getVirtualId() const {
        return _virtualId;
    }

    LFramework::ComPtr<Common::IDataReceiver> startTask(LFramework::ComPtr<Common::IDataReceiver> userDataReceiver);

    void addTask(LFramework::Guid taskId) {
        _tasks.push_back(taskId);
    }

    bool initialized() const {
        return _tasksCount == _tasks.size();
    }
private:
    std::uint8_t _realId;
    std::uint32_t _virtualId;
    std::uint32_t _tasksCount;
    std::vector<LFramework::Guid> _tasks;
    Host* _host = nullptr;
    std::recursive_mutex _taskMutex;
    std::shared_ptr<TaskContextConstructor> _nextTask = nullptr;
    LFramework::ComPtr<ITaskContext> _currentTask = nullptr;
};




class Host : public Common::DataStream {
public:

    bool start() override {
        _txAvailable.give();
        return true;
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
protected:
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
