#pragma once

#include <atomic>
#include <MicroNetwork/Common/IDataReceiver.h>
#include <mutex>

namespace MicroNetwork::Host {
    class ITaskContext;
}

namespace LFramework {
template<>
struct InterfaceAbi<MicroNetwork::Host::ITaskContext> : public InterfaceAbi<IUnknown> {
    using Base = InterfaceAbi<IUnknown>;
    static constexpr InterfaceID ID() { return 8; }
    virtual Result setUserDataReceiver(LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> userDataReceiver) = 0;
    virtual Result handleNetworkPacket(MicroNetwork::Common::PacketHeader header, const void* data) = 0;
    virtual Result onTaskStopped() = 0;
};

template<class TImplementer>
struct InterfaceRemap<MicroNetwork::Host::ITaskContext, TImplementer> : public InterfaceRemap<IUnknown, TImplementer> {
public:
    virtual Result setUserDataReceiver(LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> userDataReceiver) { return this->_implementer->setUserDataReceiver(userDataReceiver); }
    virtual Result handleNetworkPacket(MicroNetwork::Common::PacketHeader header, const void* data) { return this->_implementer->handleNetworkPacket(header, data); }
    virtual Result onTaskStopped() { return this->_implementer->onTaskStopped(); }
};
}




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
    LFramework::ComPtr<Common::IDataReceiver> _userDataReceiver = nullptr;
    NodeContext* _node;
};

}
