#pragma once

#include <MicroNetwork/Common/IDataReceiver.h>
#include <LFramework/Guid.h>
#include <MicroNetwork/Host/INetwork.h>
#include <MicroNetwork/Host/UsbLinkProvider.h>
#include <MicroNetwork/Host/NodeContext.h>
#include <vector>
#include <unordered_map>
#include <MicroNetwork/Host/Host.h>
#include <algorithm>

namespace MicroNetwork::Host {


class INodeContainer {
public:
    virtual ~INodeContainer() = default;
    virtual void addNode(std::shared_ptr<NodeContext> node) = 0;
    virtual void removeNode(std::shared_ptr<NodeContext> node) = 0;
};

class LinkProviderContext : public ILinkCallback{
public:
    LinkProviderContext(std::function<std::shared_ptr<LinkProvider>(ILinkCallback*)> providerConstructor, INodeContainer* nodeContainer) :_nodeContainer(nodeContainer){
        _provider = providerConstructor(this);

        linksChanged();
    }
    void linksChanged() override {
        auto newLinks = _provider->getLinks();

        //remove deleted links
        auto it = _hosts.begin();
        while (it != _hosts.end()) {
            if(std::find(newLinks.begin(), newLinks.end(), (*it)->getPath()) == newLinks.end()){
                it = _hosts.erase(it);
            } else {
                ++it;
            }
        }

        //add new links
        for(const auto& link : newLinks){
            if(!hasHost(link)){
                try{
                    auto stream = _provider->makeStream(link);
                    auto host = std::make_shared<Host>(link, stream);
                    stream->start();
                    lfDebug() << "Host created for path: " << link.c_str();
                    _hosts.push_back(host);
                }catch(const std::exception& ex){
                    lfDebug() << "Failed to create stream for path: " << link.c_str();
                }
            }
        }

    }
private:
    bool hasHost(const std::string& path){
        for(auto h : _hosts){
            if(h->getPath() == path){
                return true;
            }
        }
        return false;
    }
    std::vector<std::shared_ptr<Host>> _hosts;
    std::shared_ptr<LinkProvider> _provider;
    INodeContainer* _nodeContainer;
};

class Network : public LFramework::ComImplement<Network, LFramework::ComObject, INetwork>, public INodeContainer {
public:
    Network(std::uint16_t vid, std::uint16_t pid){
        _linkProviders.push_back(std::make_shared<LinkProviderContext>(
                                     [=](ILinkCallback* callback){ return std::make_shared<UsbLinkProvider>(vid, pid, callback); },
                                     this)
                                 );
    }
    LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> startTask(std::uint32_t node, LFramework::Guid taskId, LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> userDataReceiver){
        return nullptr;
    }
    bool isTaskSupported(std::uint32_t node, LFramework::Guid taskId){
        std::lock_guard<std::mutex> lock(_nodesMutex);
        for(auto nodeRecord : _nodes){
            if(nodeRecord.first == node){
                return nodeRecord.second->isTaskSupported(taskId);
            }
        }
        return false;
    }
    std::vector<std::uint32_t> getNodes(){
        std::lock_guard<std::mutex> lock(_nodesMutex);
        std::vector<std::uint32_t> result;
        for(auto nodeRecord : _nodes){
            result.push_back(nodeRecord.first);
        }
        return result;
    }
    std::uint32_t getStateId(){
        return _stateId.load();
    }

    void addNode(std::shared_ptr<NodeContext> node) override{
        std::lock_guard<std::mutex> lock(_nodesMutex);
        _nodes[_lastNodeId++] = node;
        ++_stateId;
    }
    void removeNode(std::shared_ptr<NodeContext> node) override{
        std::lock_guard<std::mutex> lock(_nodesMutex);
        for(auto nodeRecord : _nodes){
            if(nodeRecord.second == node){
                _nodes.erase(nodeRecord.first);
            }
        }
        ++_stateId;
    }
private:

    std::mutex _nodesMutex;
    std::uint32_t _lastNodeId = 0;
    std::atomic<std::uint32_t> _stateId = 0;
    std::unordered_map<std::uint32_t, std::shared_ptr<NodeContext>> _nodes;
    std::vector<std::shared_ptr<LinkProviderContext>> _linkProviders;
};

}
