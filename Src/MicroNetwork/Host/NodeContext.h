#pragma once

#include <MicroNetwork.Common.h>
#include <MicroNetwork/Common/Packet.h>
#include <LFramework/Threading/Semaphore.h>
#include <LFramework/Threading/CriticalSection.h>
#include <LFramework/Debug.h>
#include <vector>
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
    NodeContext(std::uint8_t realId, std::uint32_t tasksCount, Host* host) : _realId(realId),  _tasksCount(tasksCount), _host(host){

    }
    ~NodeContext() {

    }
    void handleNetworkPacket(Common::PacketHeader header, const void* data) {
        //lfDebug() << "Node context received packet: id=" << header.id << " size=" << header.size;

        if(header.id == Common::PacketId::TaskStop){
            //lfDebug() << "Received TaskStop";
            if(_currentTask != nullptr){
                _currentTask.reset();
            }
        }else if(header.id == Common::PacketId::TaskStart){
            //lfDebug() << "Received TaskStart";
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


    LFramework::ComPtr<Common::IDataReceiver> startTask(LFramework::Guid taskId, LFramework::ComPtr<Common::IDataReceiver> userDataReceiver);

    void addTask(LFramework::Guid taskId) {
        std::lock_guard<std::recursive_mutex> lock(_taskMutex);
        _tasks.push_back(taskId);
    }

    bool isReady() const {
        return _tasks.size() == _tasksCount;
    }

    bool isTaskLaunched() {
        return _currentTask != nullptr;
    }

    bool isTaskSupported(LFramework::Guid taskId) const {
        std::lock_guard<std::recursive_mutex> lock(_taskMutex);
        for(auto& task : _tasks){
            if(task == taskId){
                return true;
            }
        }
        return false;
    }

    void requestTaskStop() {
        lfDebug() << "requestTaskStop";
        Common::PacketHeader packet;
        packet.id = Common::PacketId::TaskStop;
        packet.size = 0;
        handleUserPacket(packet, nullptr);
    }
private:
    std::uint8_t _realId;
    std::uint32_t _tasksCount;
    std::vector<LFramework::Guid> _tasks;
    Host* _host = nullptr;
    mutable std::recursive_mutex _taskMutex;
    std::shared_ptr<TaskContextConstructor> _nextTask = nullptr;
    LFramework::ComPtr<ITaskContext> _currentTask = nullptr;
};

}
