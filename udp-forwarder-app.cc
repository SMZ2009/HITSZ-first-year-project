#include "udp-forwarder-app.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/udp-socket-factory.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("UdpForwarderApp");
NS_OBJECT_ENSURE_REGISTERED(UdpForwarderApp);

TypeId UdpForwarderApp::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::UdpForwarderApp")
        .SetParent<Application>()
        .SetGroupName("Applications");
    return tid;
}

UdpForwarderApp::UdpForwarderApp()
    : m_socket(nullptr),
      m_localIp(Ipv4Address::GetAny()),
      m_localPort(0),
      m_remoteIp(Ipv4Address::GetAny()),
      m_remotePort(0),
      m_linkName("UNNAMED")
{}

UdpForwarderApp::~UdpForwarderApp()
{
    if (m_socket) m_socket->Close();
}

void UdpForwarderApp::StartApplication()
{
    if (!m_socket) {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());

        // 绑定到指定本地 IP + 端口
        InetSocketAddress localAddr(m_localIp, m_localPort);
        if (m_socket->Bind(localAddr) == -1) {
            NS_LOG_UNCOND("[Forwarder][" << m_linkName << "] ERROR: Cannot bind to " 
                          << m_localIp << ":" << m_localPort);
            return;
        }

        m_socket->SetRecvCallback(MakeCallback(&UdpForwarderApp::HandleRead, this));

        NS_LOG_UNCOND("[Forwarder][" << m_linkName << "] Node=" << GetNode()->GetId()
                        << " listening on " << m_localIp << ":" << m_localPort
                        << " → forwarding to " << m_remoteIp << ":" << m_remotePort);
    }
}

void UdpForwarderApp::StopApplication()
{
    if (m_socket) { m_socket->Close(); m_socket = nullptr; }
}

void UdpForwarderApp::HandleRead(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from))) {
        InetSocketAddress src = InetSocketAddress::ConvertFrom(from);
        uint32_t uid = packet->GetUid();
        uint32_t size = packet->GetSize();

        // === 新增每个包的详细接收日志 ===
        NS_LOG_UNCOND("[Forwarder][" << m_linkName << "] Node=" << GetNode()->GetId()
                        << " RX UID=" << uid
                        << " size=" << size
                        << " from " << src.GetIpv4() << ":" << src.GetPort());

        if (m_remoteIp != Ipv4Address("0.0.0.0") && m_remotePort != 0) {
            Ptr<Packet> copy = packet->Copy();
            int sent = socket->SendTo(copy, 0, InetSocketAddress(m_remoteIp, m_remotePort));

            if (sent <= 0) {
                NS_LOG_UNCOND("[Forwarder][" << m_linkName << "] ERROR: SendTo UID=" << uid << " failed");
            } else {
                // === 新增每个包的转发日志 ===
                NS_LOG_UNCOND("[Forwarder][" << m_linkName << "] Forwarded UID=" << uid
                                << " to " << m_remoteIp << ":" << m_remotePort
                                << " (" << sent << " bytes)");
            }
        } else {
            NS_LOG_UNCOND("[Forwarder][" << m_linkName << "] WARNING: remote IP/port not set. UID=" << uid << " dropped.");
        }
    }
}


} // namespace ns3
