#include "video-receiver-app.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/udp-socket-factory.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("VideoReceiverApp");
NS_OBJECT_ENSURE_REGISTERED(VideoReceiverApp);

VideoReceiverApp::VideoReceiverApp() : m_localPort(0), m_totalNalus(0), m_receivedNalus(0), m_reported(false) {}
VideoReceiverApp::~VideoReceiverApp() {}

TypeId VideoReceiverApp::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::VideoReceiverApp")
        .SetParent<Application>()
        .SetGroupName("Applications");
    return tid;
}

void VideoReceiverApp::StartApplication()
{
    NS_LOG_UNCOND("[VideoReceiverApp] Starting on port " << m_localPort);
    m_receivedNalus = 0; m_reported = false; m_receivedUids.clear();

    if (!m_outputFile.empty()) { m_output.open(m_outputFile, std::ios::binary); }

    if (!m_socket) {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        InetSocketAddress local(Ipv4Address::GetAny(), m_localPort);
        if (m_socket->Bind(local) == -1) { NS_LOG_ERROR("Failed to bind socket"); return; }
        m_socket->SetRecvCallback(MakeCallback(&VideoReceiverApp::HandleRead, this));
    }
}

void VideoReceiverApp::StopApplication()
{
    if (m_socket) { m_socket->Close(); m_socket = nullptr; }
    if (m_output.is_open()) m_output.close();
    m_reported = true;
}

void VideoReceiverApp::WriteNaluToFile(const std::vector<uint8_t> &nalu)
{
    if (!m_output.is_open() || nalu.empty()) return;
    uint8_t startCode[4] = {0x00,0x00,0x00,0x01};
    m_output.write(reinterpret_cast<char*>(startCode),4);
    m_output.write(reinterpret_cast<const char*>(nalu.data()), nalu.size());
    m_output.flush();
}

void VideoReceiverApp::HandleRead(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from))) {
        m_rxTrace(packet, from);

        uint32_t size = packet->GetSize();
        if (size < 2) continue;
        std::vector<uint8_t> buffer(size);
        packet->CopyData(buffer.data(), size);

        uint8_t flags = buffer[0];
        //uint8_t naluType = buffer[1];
        std::vector<uint8_t> payload(buffer.begin()+2, buffer.end());

        bool isStart = (flags & 0x80)!=0;
        bool isEnd   = (flags & 0x40)!=0;

        if (isStart) m_currentNaluBuffer.clear();
        m_currentNaluBuffer.insert(m_currentNaluBuffer.end(), payload.begin(), payload.end());

        if (isEnd && !m_currentNaluBuffer.empty()) {
            WriteNaluToFile(m_currentNaluBuffer);
            std::lock_guard<std::mutex> lock(m_mutex);
            ++m_receivedNalus;
            m_receivedUids.insert(packet->GetUid());
            m_currentNaluBuffer.clear();
        }
    }
}

} // namespace ns3
