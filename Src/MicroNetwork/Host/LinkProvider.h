#pragma once

#include <MicroNetwork/Common/DataStream.h>
#include <string>
#include <vector>
#include <mutex>
#include <memory>

namespace MicroNetwork::Host {

class ILinkCallback {
public:
    virtual ~ILinkCallback() = default;
    virtual void linksChanged() = 0;
};

class LinkProvider{
public:
    virtual ~LinkProvider() = default;
    LinkProvider(ILinkCallback* linkCallback) : _linkCallback(linkCallback){

    }
    virtual std::vector<std::string> getLinks() = 0;
    virtual std::shared_ptr<Common::DataStream> makeStream(const std::string& linkPath) = 0;
protected:
    void onLinksUpdated() {
        if(_linkCallback != nullptr){
            _linkCallback->linksChanged();
        }
    }
private:
    ILinkCallback* _linkCallback;
};

}
