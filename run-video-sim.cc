// run-video-sim.cc
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"

#include "../contrib/satellite-network/model/gsl-channel.h"
#include "../contrib/satellite-network/model/gsl-net-device.h"
#include "../contrib/satellite-network/model/point-to-point-laser-net-device.h"
#include "../contrib/satellite-network/model/point-to-point-laser-channel.h"

#include "../src/applications/video-sender-app.h"
#include "../src/applications/video-receiver-app.h"
#include "../src/applications/udp-forwarder-app.h"

#include <unistd.h>
#include <limits.h>
#include <fstream>
#include <vector>

using namespace ns3;

// ------------------------- Trace Callbacks -------------------------
void VideoTxTrace(Ptr<const Packet> p)
{
    NS_LOG_UNCOND("[H265-TX] UID=" << p->GetUid() << " size=" << p->GetSize());
}

void VideoRxTrace(Ptr<const Packet> p, Address addr)
{
    NS_LOG_UNCOND("[H265-RX] UID=" << p->GetUid() << " size=" << p->GetSize()
                     << " from=" << addr);
}

// Physical layer trace callbacks
void TraceTxGs1S1(Ptr<const Packet> p) { NS_LOG_UNCOND("[GS1->S1] PhyTxEnd UID=" << p->GetUid()); }
void TraceRxGs1S1(Ptr<const Packet> p) { NS_LOG_UNCOND("[GS1->S1] PhyRxEnd UID=" << p->GetUid()); }
void TraceTxS1S2(Ptr<const Packet> p)  { NS_LOG_UNCOND("[S1->S2] PhyTxEnd UID=" << p->GetUid()); }
void TraceRxS1S2(Ptr<const Packet> p)  { NS_LOG_UNCOND("[S1->S2] PhyRxEnd UID=" << p->GetUid()); }
void TraceTxS2Gs2(Ptr<const Packet> p) { NS_LOG_UNCOND("[S2->GS2] PhyTxEnd UID=" << p->GetUid()); }
void TraceRxS2Gs2(Ptr<const Packet> p) { NS_LOG_UNCOND("[S2->GS2] PhyRxEnd UID=" << p->GetUid()); }

// ------------------------- Create GSL Link -------------------------
NetDeviceContainer CreateGslLink(Ptr<Node> a, Ptr<Node> b)
{
    Ptr<GSLNetDevice> devA = CreateObject<GSLNetDevice>();
    Ptr<GSLNetDevice> devB = CreateObject<GSLNetDevice>();
    Ptr<GSLChannel> channel = CreateObject<GSLChannel>();

    Ptr<DropTailQueue<Packet>> queueA = CreateObject<DropTailQueue<Packet>>();
    Ptr<DropTailQueue<Packet>> queueB = CreateObject<DropTailQueue<Packet>>();
    queueA->SetMaxSize(QueueSize("2MiB"));
    queueB->SetMaxSize(QueueSize("2MiB"));

    devA->SetQueue(queueA);
    devB->SetQueue(queueB);

    devA->Attach(channel);
    devB->Attach(channel);

    devA->SetAddress(Mac48Address::Allocate());
    devB->SetAddress(Mac48Address::Allocate());

    a->AddDevice(devA);
    b->AddDevice(devB);

    NetDeviceContainer ndc;
    ndc.Add(devA); ndc.Add(devB);
    return ndc;
}

// ------------------------- Create Laser Link -------------------------
NetDeviceContainer CreateLaserLink(Ptr<Node> a, Ptr<Node> b)
{
    Ptr<PointToPointLaserNetDevice> devA = CreateObject<PointToPointLaserNetDevice>();
    Ptr<PointToPointLaserNetDevice> devB = CreateObject<PointToPointLaserNetDevice>();
    Ptr<PointToPointLaserChannel> channel = CreateObject<PointToPointLaserChannel>();

    Ptr<DropTailQueue<Packet>> queueA = CreateObject<DropTailQueue<Packet>>();
    Ptr<DropTailQueue<Packet>> queueB = CreateObject<DropTailQueue<Packet>>();
    queueA->SetMaxSize(QueueSize("2MiB"));
    queueB->SetMaxSize(QueueSize("2MiB"));

    devA->SetQueue(queueA);
    devB->SetQueue(queueB);

    devA->Attach(channel);
    devB->Attach(channel);

    a->AddDevice(devA);
    b->AddDevice(devB);

    devA->SetDestinationNode(b);
    devB->SetDestinationNode(a);

    devA->SetMtu(1500);
    devB->SetMtu(1500);
    devA->SetInterframeGap(MilliSeconds(0.1));
    devB->SetInterframeGap(MilliSeconds(0.1));

    NetDeviceContainer ndc;
    ndc.Add(devA); ndc.Add(devB);
    return ndc;
}

// ------------------------- Count NALUs -------------------------
static uint32_t CountNalusInAnnexBFile(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return 0;
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    uint32_t count = 0;
    size_t i = 0;
    while (i + 2 < buf.size()) {
        if (buf[i] == 0x00 && buf[i+1] == 0x00 && buf[i+2] == 0x01) { ++count; i += 3; continue; }
        if (i + 3 < buf.size() && buf[i] == 0x00 && buf[i+1] == 0x00 &&
            buf[i+2] == 0x00 && buf[i+3] == 0x01) { ++count; i += 4; continue; }
        ++i;
    }
    return count;
}

// ------------------- Print Video Stats -------------------
void PrintVideoStats(Ptr<VideoReceiverApp> recvApp, uint32_t totalNalus)
{
    uint32_t receivedNalus = recvApp->GetReceivedNaluCount();
    uint32_t lostNalus = (totalNalus > receivedNalus) ? totalNalus - receivedNalus : 0;
    double successRate = (totalNalus > 0) ? (double)receivedNalus / totalNalus * 100.0 : 0.0;

    NS_LOG_UNCOND("=== 视频接收统计 ===");
    NS_LOG_UNCOND("总 NALU 数量: " << totalNalus);
    NS_LOG_UNCOND("收到 NALU 数: " << receivedNalus);
    NS_LOG_UNCOND("丢失 NALU 数: " << lostNalus);
    NS_LOG_UNCOND("接收成功率: " << successRate << "%");
}

// ------------------------- Main -------------------------
int main(int argc, char *argv[])
{
    char absPath[PATH_MAX];
    if (realpath("./data/sample.h265", absPath) == nullptr) {
        std::cerr << "realpath failed for ./data/sample.h265" << std::endl;
        return 1;
    }

    LogComponentEnable("H265VideoSenderApp", LOG_LEVEL_INFO);
    LogComponentEnable("VideoReceiverApp", LOG_LEVEL_INFO);
    LogComponentEnable("UdpForwarderApp", LOG_LEVEL_INFO);

    // -------------------- Nodes --------------------
    Ptr<Node> gs1 = CreateObject<Node>();
    Ptr<Node> s1  = CreateObject<Node>();
    Ptr<Node> s2  = CreateObject<Node>();
    Ptr<Node> gs2 = CreateObject<Node>();
    NodeContainer all; all.Add(gs1); all.Add(s1); all.Add(s2); all.Add(gs2);
    InternetStackHelper stack; stack.Install(all);

    // -------------------- Mobility --------------------
    Ptr<ConstantPositionMobilityModel> gs1Mob = CreateObject<ConstantPositionMobilityModel>();
    gs1Mob->SetPosition(Vector(0,0,0)); gs1->AggregateObject(gs1Mob);
    Ptr<ConstantPositionMobilityModel> gs2Mob = CreateObject<ConstantPositionMobilityModel>();
    gs2Mob->SetPosition(Vector(2000e3,0,0)); gs2->AggregateObject(gs2Mob);
    Ptr<ConstantVelocityMobilityModel> cv1 = CreateObject<ConstantVelocityMobilityModel>();
    cv1->SetPosition(Vector(1000e3,0,630000)); cv1->SetVelocity(Vector(7500,0,0)); s1->AggregateObject(cv1);
    Ptr<ConstantVelocityMobilityModel> cv2 = CreateObject<ConstantVelocityMobilityModel>();
    cv2->SetPosition(Vector(1500e3,0,630000)); cv2->SetVelocity(Vector(7500,0,0)); s2->AggregateObject(cv2);

    // -------------------- 网络拓扑 --------------------
    NetDeviceContainer d_gs1_s1 = CreateGslLink(gs1, s1);
    NetDeviceContainer d_s1_s2  = CreateLaserLink(s1, s2);
    NetDeviceContainer d_s2_gs2 = CreateGslLink(s2, gs2);

    // -------------------- PHY traces --------------------
    d_gs1_s1.Get(0)->TraceConnectWithoutContext("PhyTxEnd", MakeCallback(&TraceTxGs1S1));
    d_gs1_s1.Get(1)->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&TraceRxGs1S1));
    d_s1_s2.Get(0)->TraceConnectWithoutContext("PhyTxEnd", MakeCallback(&TraceTxS1S2));
    d_s1_s2.Get(1)->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&TraceRxS1S2));
    d_s2_gs2.Get(0)->TraceConnectWithoutContext("PhyTxEnd", MakeCallback(&TraceTxS2Gs2));
    d_s2_gs2.Get(1)->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&TraceRxS2Gs2));

    // -------------------- IP addresses --------------------
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0","255.255.255.0"); Ipv4InterfaceContainer if_gs1_s1 = ipv4.Assign(d_gs1_s1);
    ipv4.SetBase("10.0.1.0","255.255.255.0"); Ipv4InterfaceContainer if_s1_s2 = ipv4.Assign(d_s1_s2);
    ipv4.SetBase("10.0.2.0","255.255.255.0"); Ipv4InterfaceContainer if_s2_gs2 = ipv4.Assign(d_s2_gs2);

    // -------------------- Static routing --------------------
    Ptr<Ipv4StaticRouting> srGs1 = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(gs1->GetObject<Ipv4>()->GetRoutingProtocol());
    Ptr<Ipv4StaticRouting> srS1  = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(s1->GetObject<Ipv4>()->GetRoutingProtocol());
    Ptr<Ipv4StaticRouting> srS2  = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(s2->GetObject<Ipv4>()->GetRoutingProtocol());
    Ptr<Ipv4StaticRouting> srGs2 = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(gs2->GetObject<Ipv4>()->GetRoutingProtocol());

    // GS1 → GS2
    srGs1->AddHostRouteTo(if_s2_gs2.GetAddress(1), if_gs1_s1.GetAddress(1), 1);      // 下一跳 S1 接口
    srS1->AddHostRouteTo(if_s2_gs2.GetAddress(1), if_s1_s2.GetAddress(0), 1);        // 下一跳 S2 接口（Forwarder 远端）
    srS2->AddHostRouteTo(if_s2_gs2.GetAddress(1), if_s2_gs2.GetAddress(0), 1);       // 下一跳 GS2
    srGs2->SetDefaultRoute(if_s2_gs2.GetAddress(0), 1);                              // 默认路由回 S2

    // -------------------- Video & Forwarders --------------------
    uint16_t videoPort = 5000;
    uint32_t totalNalus = CountNalusInAnnexBFile(absPath);

    // S1 Forwarder: 接收 GS1，转发到 S2 接口
    Ptr<UdpForwarderApp> fwd1 = CreateObject<UdpForwarderApp>();
    fwd1->SetLocalIp(if_gs1_s1.GetAddress(1));         // 接收 GS1 的接口
    fwd1->SetLocalPort(videoPort);
    fwd1->SetRemote(if_s1_s2.GetAddress(0), videoPort); // 发送到 S2 接口
    fwd1->SetLinkName("GS1->S2 via S1");
    s1->AddApplication(fwd1);
    fwd1->SetStartTime(Seconds(0.5));
    fwd1->SetStopTime(Seconds(85.0));

    // S2 Forwarder: 接收 S1-S2，转发到 GS2
    Ptr<UdpForwarderApp> fwd2 = CreateObject<UdpForwarderApp>();
    fwd2->SetLocalIp(if_s1_s2.GetAddress(1));         // 接收 S1-S2 的接口
    fwd2->SetLocalPort(videoPort);
    fwd2->SetRemote(if_s2_gs2.GetAddress(1), videoPort); // 发送到 GS2 接口
    fwd2->SetLinkName("S1->GS2 via S2");
    s2->AddApplication(fwd2);
    fwd2->SetStartTime(Seconds(0.5));
    fwd2->SetStopTime(Seconds(90.0));

    // --- Video Receiver: GS2 ---
    Ptr<VideoReceiverApp> recvApp = CreateObject<VideoReceiverApp>();
    recvApp->SetLocalPort(videoPort);
    recvApp->SetOutputFile("./data/received.h265");
    recvApp->SetTotalNalus(totalNalus);
    gs2->AddApplication(recvApp);
    recvApp->SetStartTime(Seconds(1.0));
    recvApp->SetStopTime(Seconds(100.0));
    recvApp->TraceConnectWithoutContext("Rx", MakeCallback(&VideoRxTrace));

    // --- Video Sender: GS1 ---
    Ptr<H265VideoSenderApp> sendApp = CreateObject<H265VideoSenderApp>();
    sendApp->SetRemote(if_gs1_s1.GetAddress(1), videoPort);
    sendApp->SetFilePath(absPath);
    sendApp->SetMaxPayload(1400);
    gs1->AddApplication(sendApp);
    sendApp->SetStartTime(Seconds(2.0));
    sendApp->SetStopTime(Seconds(80.0));
    sendApp->TraceConnectWithoutContext("Tx", MakeCallback(&VideoTxTrace));

    // -------------------- Schedule Stats --------------------
    Simulator::Schedule(Seconds(120.0), &PrintVideoStats, recvApp, totalNalus);

    Simulator::Stop(Seconds(120.0));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}