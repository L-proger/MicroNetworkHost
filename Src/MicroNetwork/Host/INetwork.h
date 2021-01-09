#pragma once

#include <MicroNetwork.Common.h>
#include <vector>
#include <cstdint>

namespace MicroNetwork::Host {
    class INetwork;

    enum class NodeState {
        NotReady,
        Idle,
        TaskLaunched,
        InvalidNode
    };

    using NodeHandle = std::uint32_t;
}

namespace LFramework {
template<>
struct InterfaceAbi<MicroNetwork::Host::INetwork> : public InterfaceAbi<IUnknown> {
    using Base = InterfaceAbi<IUnknown>;
    static constexpr InterfaceID ID() { return { 0x2f1e8f7c, 0x4629, 0x4cce, { 0xad, 0x12, 0xb2, 0x2e, 0x7b, 0x4c, 0x86, 0x74 } }; }

    virtual LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> startTask(std::uint32_t node, LFramework::Guid taskId, LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> userDataReceiver) = 0;
    virtual bool isTaskSupported(std::uint32_t node, LFramework::Guid taskId) = 0;
    virtual std::vector<MicroNetwork::Host::NodeHandle> getNodes() = 0;
    virtual MicroNetwork::Host::NodeState getNodeState(MicroNetwork::Host::NodeHandle node) = 0;
    virtual std::uint32_t getStateId() = 0;
};

template<class TImplementer>
struct InterfaceRemap<MicroNetwork::Host::INetwork, TImplementer> : public InterfaceRemap<IUnknown, TImplementer> {
public:
    virtual LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> startTask(std::uint32_t node, LFramework::Guid taskId, LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> userDataReceiver) { return this->implementer()->startTask(node, taskId, userDataReceiver); }
    virtual bool isTaskSupported(std::uint32_t node, LFramework::Guid taskId)  { return this->implementer()->isTaskSupported(node, taskId); }
    virtual std::vector<std::uint32_t> getNodes() { return this->implementer()->getNodes(); }
    virtual MicroNetwork::Host::NodeState getNodeState(MicroNetwork::Host::NodeHandle node) { return this->implementer()->getNodeState(node); }
    virtual std::uint32_t getStateId(){ return this->implementer()->getStateId(); }
};
}
