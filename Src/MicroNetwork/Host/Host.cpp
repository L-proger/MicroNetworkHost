#include "Host.h"

namespace MicroNetwork::Host {

bool TaskContext::handlePacket(Common::PacketHeader header, const void* data) {
    std::lock_guard<std::recursive_mutex> lock(_taskMutex);
    if(_userDataReceiver != nullptr){
        return _node->handleUserPacket(header, data);
    }else{
        return false;
    }
}

IDataReceiver* NodeContext::startTask(IDataReceiver* userDataReceiver) {
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
        _currentTask = new TaskContext(this);
        _currentTask->setUserDataReceiver(userDataReceiver);
        _currentTask->addref(); //User ref
         return _currentTask;
    }
}


bool NodeContext::handleUserPacket(Common::PacketHeader header, const void* data) {
    return _host->blockingWritePacket(header, data);
}


}
