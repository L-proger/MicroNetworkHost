#pragma once

#include <MicroNetwork/Common/IDataReceiver.h>
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


    LFramework::ComPtr<Common::IDataReceiver> startTask(LFramework::ComPtr<Common::IDataReceiver> userDataReceiver);

    void addTask(LFramework::Guid taskId) {
        std::lock_guard<std::recursive_mutex> lock(_taskMutex);
        _tasks.push_back(taskId);
    }

    bool initialized() const {
        return _tasksCount == _tasks.size();
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
