#include "video-sender-app.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/udp-socket-factory.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("H265VideoSenderApp");
NS_OBJECT_ENSURE_REGISTERED(H265VideoSenderApp);

TypeId H265VideoSenderApp::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::H265VideoSenderApp")
        .SetParent<Application>()
        .SetGroupName("Applications");
    return tid;
}

H265VideoSenderApp::H265VideoSenderApp()
    : m_remoteIp(Ipv4Address::GetAny()), m_port(0), m_socket(nullptr),
      m_currentNaluIndex(0), m_currentFragmentOffset(0), m_maxPayload(1200) {}

H265VideoSenderApp::~H265VideoSenderApp() { if(m_socket) m_socket->Close(); if(m_file.is_open()) m_file.close(); }

void H265VideoSenderApp::SetRemote(Ipv4Address addr, uint16_t port) { m_remoteIp = addr; m_port = port; }
void H265VideoSenderApp::SetFilePath(const std::string &path) { m_file.open(path, std::ios::binary); LoadFileAndParse(); }
void H265VideoSenderApp::SetMaxPayload(uint32_t size) { m_maxPayload = size; }

void H265VideoSenderApp::StartApplication()
{
    if (!m_socket) {
        m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::UdpSocketFactory"));
        InetSocketAddress local(Ipv4Address::GetAny(),0);
        m_socket->Bind(local);
    }
    Simulator::Schedule(Seconds(0.1), &H265VideoSenderApp::SendNextNalu, this);
}

void H265VideoSenderApp::StopApplication() { if(m_socket){ m_socket->Close(); m_socket=nullptr;} m_nalus.clear(); m_currentNaluIndex=0; m_currentFragmentOffset=0; }

bool H265VideoSenderApp::LoadFileAndParse()
{
    if (!m_file.is_open()) return false;
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(m_file)), std::istreambuf_iterator<char>());
    m_file.close();

    std::vector<size_t> naluStarts;
    size_t i=0;
    while(i+2<buf.size()){
        if(buf[i]==0 && buf[i+1]==0 && buf[i+2]==1){ naluStarts.push_back(i+3); i+=3; continue; }
        if(i+3<buf.size() && buf[i]==0 && buf[i+1]==0 && buf[i+2]==0 && buf[i+3]==1){ naluStarts.push_back(i+4); i+=4; continue; }
        ++i;
    }

    for(size_t idx=0; idx<naluStarts.size(); ++idx){
        size_t startPos=naluStarts[idx];
        size_t endPos=(idx+1<naluStarts.size())? naluStarts[idx+1]-3 : buf.size();
        if(endPos<=startPos) continue;
        m_nalus.emplace_back(buf.begin()+startPos, buf.begin()+endPos);
    }
    return !m_nalus.empty();
}

void H265VideoSenderApp::SendNextNalu()
{
    if(!m_socket || m_currentNaluIndex>=m_nalus.size()) return;

    const auto &nalu=m_nalus[m_currentNaluIndex];
    size_t offset=m_currentFragmentOffset;
    size_t remain=nalu.size()-offset;
    size_t payloadSize=std::min(remain, static_cast<size_t>(m_maxPayload));

    uint8_t flags=0x00;
    if(offset==0) flags|=0x80;
    if(offset+payloadSize>=nalu.size()) flags|=0x40;

    uint8_t naluType=(nalu[0]>>1)&0x3F;

    std::vector<uint8_t> pkt;
    pkt.push_back(flags);
    pkt.push_back(naluType);
    pkt.insert(pkt.end(), nalu.begin()+offset, nalu.begin()+offset+payloadSize);

    Ptr<Packet> packet=Create<Packet>(pkt.data(), pkt.size());
    m_socket->SendTo(packet,0,InetSocketAddress(m_remoteIp,m_port));

    if(flags&0x40){ ++m_currentNaluIndex; m_currentFragmentOffset=0; }
    else { m_currentFragmentOffset+=payloadSize; }

    Simulator::Schedule(MilliSeconds(10), &H265VideoSenderApp::SendNextNalu, this);
}

} // namespace ns3
