#ifndef UDP_FORWARDER_APP_H
#define UDP_FORWARDER_APP_H

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include <string>

namespace ns3 {

class UdpForwarderApp : public Application
{
public:
    static TypeId GetTypeId(void);

    UdpForwarderApp();
    virtual ~UdpForwarderApp();

    void SetLocalIp(Ipv4Address ip) { m_localIp = ip; }
    void SetLocalPort(uint16_t port) { m_localPort = port; }
    void SetRemote(Ipv4Address ip, uint16_t port) { m_remoteIp = ip; m_remotePort = port; }
    void SetLinkName(const std::string &name) { m_linkName = name; }

protected:
    virtual void StartApplication() override;
    virtual void StopApplication() override;

private:
    void HandleRead(Ptr<Socket> socket);

    Ptr<Socket> m_socket;
    Ipv4Address m_localIp;
    uint16_t    m_localPort;
    Ipv4Address m_remoteIp;
    uint16_t    m_remotePort;
    std::string m_linkName;
};

} // namespace ns3

#endif // UDP_FORWARDER_APP_H
