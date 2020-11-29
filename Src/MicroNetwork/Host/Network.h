#pragma once

#include <MicroNetwork/Common/IDataReceiver.h>
#include <LFramework/Guid.h>

#include <vector>

namespace MicroNetwork::Host {

class Network {
public:
     Common::IDataReceiver* startTask(std::uint32_t node, LFramework::Guid taskId, Common::IDataReceiver* userDataReceiver);
     bool isTaskSupported(std::uint32_t node, LFramework::Guid taskId);
     std::vector<std::uint32_t> getNodes();
};

}
