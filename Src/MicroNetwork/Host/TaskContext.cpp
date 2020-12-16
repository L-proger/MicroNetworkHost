#include <MicroNetwork/Host/TaskContext.h>
#include <MicroNetwork/Host/NodeContext.h>

namespace MicroNetwork::Host {

LFramework::Result TaskContext::packet(Common::PacketHeader header, const void* data) {
    std::lock_guard<std::recursive_mutex> lock(_taskMutex);
    if(!_txClosed && _userDataReceiver != nullptr){
        return _node->handleUserPacket(header, data) ? LFramework::Result::Ok : LFramework::Result::UnknownFailure;
    }else{
        return LFramework::Result::UnknownFailure;
    }
}


void TaskContext::onNetworkRelease() {
    std::lock_guard<std::recursive_mutex> lock(_taskMutex);
    _txClosed = true;
    _userDataReceiver.reset();
}
void TaskContext::onUserRelease() {
    std::lock_guard<std::recursive_mutex> lock(_taskMutex);
    _node->requestTaskStop();
    _txClosed = true;
}

}
