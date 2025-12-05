#ifndef VIDEO_RECEIVER_APP_H
#define VIDEO_RECEIVER_APP_H

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include <fstream>
#include <vector>
#include <string>
#include <mutex>
#include <unordered_set>

namespace ns3 {

class VideoReceiverApp : public Application
{
public:
    static TypeId GetTypeId(void);

    VideoReceiverApp();
    virtual ~VideoReceiverApp();

    void SetLocalPort(uint16_t port) { m_localPort = port; }
    void SetOutputFile(const std::string &file) { m_outputFile = file; }
    void SetTotalNalus(uint32_t total) { m_totalNalus = total; }
    uint32_t GetReceivedNaluCount() const { std::lock_guard<std::mutex> lock(m_mutex); return m_receivedNalus; }

protected:
    virtual void StartApplication() override;
    virtual void StopApplication() override;

private:
    void HandleRead(Ptr<Socket> socket);
    void WriteNaluToFile(const std::vector<uint8_t> &nalu);

    Ptr<Socket> m_socket;
    uint16_t m_localPort;
    std::ofstream m_output;
    std::string m_outputFile;
    std::vector<uint8_t> m_currentNaluBuffer;
    uint32_t m_totalNalus;
    uint32_t m_receivedNalus;
    bool m_reported;
    std::unordered_set<uint32_t> m_receivedUids;
    mutable std::mutex m_mutex;

public:
    TracedCallback<Ptr<const Packet>, Address> m_rxTrace;
};

} // namespace ns3

#endif // VIDEO_RECEIVER_APP_H
