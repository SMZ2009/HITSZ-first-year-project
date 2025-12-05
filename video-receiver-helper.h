// video-receiver-helper.h
#ifndef H265_VIDEO_RECEIVER_HELPER_H
#define H265_VIDEO_RECEIVER_HELPER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/application-container.h"
#include "video-receiver-app.h" // VideoReceiverApp

namespace ns3 {

class H265VideoReceiverHelper
{
public:
    H265VideoReceiverHelper(uint16_t port, const std::string &outputFile = "./data/received.h265")
        : m_port(port), m_outputFile(outputFile) {}

    ApplicationContainer Install(Ptr<Node> node);
    ApplicationContainer Install(NodeContainer nodes);

private:
    uint16_t m_port;
    std::string m_outputFile;
};

} // namespace ns3

#endif // H265_VIDEO_RECEIVER_HELPER_H
