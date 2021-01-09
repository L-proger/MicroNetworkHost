#pragma once

#include <MicroNetwork.Common.h>

namespace MicroNetwork::Host {
    class ITaskContext;
}

namespace LFramework {
template<>
struct InterfaceAbi<MicroNetwork::Host::ITaskContext> : public InterfaceAbi<IUnknown> {
    using Base = InterfaceAbi<IUnknown>;
    static constexpr InterfaceID ID() { return { 0xb5d3122a, 0x4b98, 0x4203, { 0xa8, 0xac, 0x80, 0xa7, 0x58, 0x5d, 0xaf, 0xea } };
; }
    virtual Result setUserDataReceiver(LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> userDataReceiver) = 0;
    virtual Result handleNetworkPacket(MicroNetwork::Common::PacketHeader header, const void* data) = 0;
};

template<class TImplementer>
struct InterfaceRemap<MicroNetwork::Host::ITaskContext, TImplementer> : public InterfaceRemap<IUnknown, TImplementer> {
public:
    virtual Result setUserDataReceiver(LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> userDataReceiver) { return this->implementer()->setUserDataReceiver(userDataReceiver); }
    virtual Result handleNetworkPacket(MicroNetwork::Common::PacketHeader header, const void* data) { return this->implementer()->handleNetworkPacket(header, data); }
};
}
