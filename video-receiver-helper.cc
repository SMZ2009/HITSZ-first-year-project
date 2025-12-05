// video-receiver-helper.cc
#include "video-receiver-helper.h"

namespace ns3 {

ApplicationContainer
H265VideoReceiverHelper::Install(Ptr<Node> node)
{
    ApplicationContainer apps;

    Ptr<VideoReceiverApp> app = CreateObject<VideoReceiverApp>();
    app->SetLocalPort(m_port);
    app->SetOutputFile(m_outputFile);
    node->AddApplication(app);
    app->SetStartTime(Seconds(1.0));
    app->SetStopTime(Seconds(100.0));

    apps.Add(app);
    return apps;
}

ApplicationContainer
H265VideoReceiverHelper::Install(NodeContainer nodes)
{
    ApplicationContainer apps;
    for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it)
    {
        apps.Add(Install(*it));
    }
    return apps;
}

} // namespace ns3
