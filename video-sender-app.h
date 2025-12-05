#ifndef VIDEO_SENDER_APP_H
#define VIDEO_SENDER_APP_H

#include "ns3/application.h"
#include "ns3/address.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>

namespace ns3 {

class H265VideoSenderApp : public Application
{
public:
    static TypeId GetTypeId(void);

    H265VideoSenderApp();
    virtual ~H265VideoSenderApp();

    void SetRemote(Ipv4Address addr, uint16_t port);
    void SetFilePath(const std::string &path);
    void SetMaxPayload(uint32_t size);

protected:
    virtual void StartApplication() override;
    virtual void StopApplication() override;

private:
    void SendNextNalu();
    bool LoadFileAndParse();
    static std::string GetNaluTypeName(uint8_t naluType);

    Ipv4Address m_remoteIp;
    uint16_t m_port;
    Ptr<Socket> m_socket;

    std::ifstream m_file;
    std::vector<std::vector<uint8_t>> m_nalus;
    uint32_t m_currentNaluIndex;
    uint32_t m_currentFragmentOffset;

    uint32_t m_maxPayload;
};

} // namespace ns3

#endif // VIDEO_SENDER_APP_H
