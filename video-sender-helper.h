#ifndef H265_VIDEO_SENDER_HELPER_H
#define H265_VIDEO_SENDER_HELPER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/application-container.h"
#include "video-sender-app.h" // H265VideoSenderApp

namespace ns3 {

class H265VideoSenderHelper
{
public:
    H265VideoSenderHelper(std::string filePath, Ipv4Address remoteAddr, uint16_t port);

    ApplicationContainer Install(Ptr<Node> node);
    ApplicationContainer Install(NodeContainer nodes);

private:
    std::string m_filePath;
    Ipv4Address m_remoteAddr;
    uint16_t m_port;
};

} // namespace ns3

#endif // H265_VIDEO_SENDER_HELPER_H
