#include "Host.h"

namespace MicroNetwork::Host {



LFramework::ComPtr<Common::IDataReceiver> NodeContext::startTask(LFramework::ComPtr<Common::IDataReceiver> userDataReceiver) {
    {
        std::lock_guard<std::recursive_mutex> lock(_taskMutex);
        if((_currentTask != nullptr) || (_nextTask != nullptr)){
            return nullptr;
        }
        _nextTask = std::make_shared<TaskContextConstructor>();
    }

    Common::MaxPacket packet;
    packet.header.id = Common::PacketId::TaskStart;
    packet.header.size = 0;
    _host->blockingWritePacket(packet.header, packet.payload.data());

    bool started = _nextTask->wait();

    std::lock_guard<std::recursive_mutex> lock(_taskMutex);
    _nextTask = nullptr;
    if(!started) {
        return nullptr;
    }else{
        _currentTask.reset();
        auto obj = new TaskContext(this);
        _currentTask.attach(obj->queryInterface<ITaskContext>());
        _currentTask->setUserDataReceiver(userDataReceiver);
        return _currentTask.queryInterface<Common::IDataReceiver>();
    }
}


bool NodeContext::handleUserPacket(Common::PacketHeader header, const void* data) {
    return _host->blockingWritePacket(header, data);
}


}
