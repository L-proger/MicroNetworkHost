#pragma once

#include <MicroNetwork/Common/IDataReceiver.h>
#include <vector>

namespace MicroNetwork::Host {
    class INetwork;
}

namespace LFramework {
template<>
struct InterfaceAbi<MicroNetwork::Host::INetwork> : public InterfaceAbi<IUnknown> {
    using Base = InterfaceAbi<IUnknown>;
    static constexpr InterfaceID ID() { return { 0x2f1e8f7c, 0x4629, 0x4cce, { 0xad, 0x12, 0xb2, 0x2e, 0x7b, 0x4c, 0x86, 0x74 } }; }

    virtual LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> startTask(std::uint32_t node, LFramework::Guid taskId, LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> userDataReceiver) = 0;
    virtual bool isTaskSupported(std::uint32_t node, LFramework::Guid taskId) = 0;
    virtual std::vector<std::uint32_t> getNodes() = 0;
    virtual std::uint32_t getStateId() = 0;
};

template<class TImplementer>
struct InterfaceRemap<MicroNetwork::Host::INetwork, TImplementer> : public InterfaceRemap<IUnknown, TImplementer> {
public:
    virtual LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> startTask(std::uint32_t node, LFramework::Guid taskId, LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> userDataReceiver) { return this->_implementer->startTask(node, taskId, userDataReceiver); }
    virtual bool isTaskSupported(std::uint32_t node, LFramework::Guid taskId)  { return this->_implementer->isTaskSupported(node, taskId); }
    virtual std::vector<std::uint32_t> getNodes() { return this->_implementer->getNodes(); }
    virtual std::uint32_t getStateId(){ return this->_implementer->getStateId(); }
};
}
