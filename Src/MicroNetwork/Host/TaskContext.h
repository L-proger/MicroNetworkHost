#pragma once

#include <MicroNetwork/Common/IDataReceiver.h>
#include <MicroNetwork/Host/ITaskContext.h>
#include <atomic>
#include <mutex>

namespace MicroNetwork::Host {

class NodeContext;
class TaskContext : public LFramework::ComImplement<TaskContext, LFramework::ComObject, Common::IDataReceiver, ITaskContext> {
public:
    TaskContext(NodeContext* node) : _node(node) {

    }
    LFramework::Result onTaskStopped() {
        _userDataReceiver.reset();
        return LFramework::Result::Ok;
    }
    LFramework::Result handleNetworkPacket(Common::PacketHeader header, const void* data) {
        std::lock_guard<std::recursive_mutex> lock(_taskMutex);
        if(_userDataReceiver != nullptr){
            _userDataReceiver->packet(header, data);
        }
        return LFramework::Result::Ok;
    }
    LFramework::Result packet(Common::PacketHeader header, const void* data);

    LFramework::Result setUserDataReceiver(LFramework::ComPtr<Common::IDataReceiver> userDataReceiver) {
        _userDataReceiver = userDataReceiver;
        return LFramework::Result::Ok;
    }
private:
    std::recursive_mutex _taskMutex;
    LFramework::ComPtr<Common::IDataReceiver> _userDataReceiver;
    NodeContext* _node;
};

}
