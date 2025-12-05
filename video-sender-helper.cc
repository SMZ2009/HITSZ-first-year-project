#include "video-sender-helper.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

H265VideoSenderHelper::H265VideoSenderHelper(std::string filePath, Ipv4Address remoteAddr, uint16_t port)
    : m_filePath(filePath), m_remoteAddr(remoteAddr), m_port(port)
{
}

ApplicationContainer
H265VideoSenderHelper::Install(Ptr<Node> node)
{
    Ptr<H265VideoSenderApp> app = CreateObject<H265VideoSenderApp>();
    app->SetFilePath(m_filePath);
    app->SetRemote(m_remoteAddr, m_port);
    node->AddApplication(app);
    app->SetStartTime(Seconds(1.0));
    app->SetStopTime(Seconds(100.0));

    ApplicationContainer apps;
    apps.Add(app);
    return apps;
}

ApplicationContainer
H265VideoSenderHelper::Install(NodeContainer nodes)
{
    ApplicationContainer apps;
    for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it)
    {
        apps.Add(Install(*it));
    }
    return apps;
}

} // namespace ns3
