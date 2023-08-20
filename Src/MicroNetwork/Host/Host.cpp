#include "Host.h"
#include <MicroNetwork/Host/ITaskContext.h>
namespace MicroNetwork::Host {

LFramework::ComPtr<Common::IDataReceiver> NodeContext::startTask(LFramework::Guid taskId, LFramework::ComPtr<Common::IDataReceiver> userDataReceiver) {
    if (!isReady()) {
        return nullptr;
    }

      auto a0 = userDataReceiver->addRef();
    auto a1 = userDataReceiver->release();


    {
        std::lock_guard<std::recursive_mutex> lock(_taskMutex);
        if((_currentTask != nullptr) || (_nextTask != nullptr)){
            return nullptr;
        }
        _nextTask = std::make_shared<TaskContextConstructor>();
    }

    Common::MaxPacket packet;
    packet.header.id = Common::PacketId::TaskStart;
    packet.setData(taskId);
    //lfDebug() << "Sending task start...";
    _host->blockingWritePacket(packet.header, packet.payload.data());

    bool started = _nextTask->wait();

    std::lock_guard<std::recursive_mutex> lock(_taskMutex);
    _nextTask = nullptr;
    if(!started) {
        return nullptr;
    }else{
        _currentTask.reset();
        auto obj = new TaskContext(this);
        _currentTask = LFramework::makeComDelegate<ITaskContext>(obj, &TaskContext::onNetworkRelease);
        _currentTask->setUserDataReceiver(userDataReceiver);
        return LFramework::makeComDelegate<Common::IDataReceiver>(obj, &TaskContext::onUserRelease);
    }
}


bool NodeContext::handleUserPacket(Common::PacketHeader header, const void* data) {
    return _host->blockingWritePacket(header, data);
}


}
