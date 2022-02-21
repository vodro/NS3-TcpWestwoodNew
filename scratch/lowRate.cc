
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/csma-module.h"

#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
// #include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include <stdlib.h>
#include <time.h>

#include <typeinfo>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("_low_rate");
class MyApp : public Application
{
public:
    MyApp();
    virtual ~MyApp();

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId(void);
    void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void ScheduleTx(void);
    void SendPacket(void);

    Ptr<Socket> m_socket;
    Address m_peer;
    uint32_t m_packetSize;
    uint32_t m_nPackets;
    DataRate m_dataRate;
    EventId m_sendEvent;
    bool m_running;
    uint32_t m_packetsSent;
};

MyApp::MyApp()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_nPackets(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_packetsSent(0)
{
}

MyApp::~MyApp()
{
    m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId(void)
{
    static TypeId tid = TypeId("MyApp")
                            .SetParent<Application>()
                            .SetGroupName("Tutorial")
                            .AddConstructor<MyApp>();
    return tid;
}

void MyApp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
}

void MyApp::StartApplication(void)
{
    m_running = true;
    m_packetsSent = 0;
    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        m_socket->Bind();
    }
    else
    {
        m_socket->Bind6();
    }
    m_socket->Connect(m_peer);
    SendPacket();
}

void MyApp::StopApplication(void)
{
    m_running = false;

    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }
}

void MyApp::SendPacket(void)
{
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    m_socket->Send(packet);

    if (++m_packetsSent < m_nPackets)
    {
        ScheduleTx();
    }
}

void MyApp::ScheduleTx(void)
{
    if (m_running)
    {
        Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
        m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
    }
}

// static void
// CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
// {
//     NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\t" << newCwnd);
//     *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
// }

// static void
// RxDrop(Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
// {
//     NS_LOG_UNCOND("RxDrop at " << Simulator::Now().GetSeconds());
//     file->Write(Simulator::Now(), p);
// }

std::string dir = "_temp";
uint32_t *_prev_txBytes;
uint32_t *_prev_rxBytes;
// Maybe Time ?
double *_prev_delaySum;
uint32_t *_prev_rxPackets;
uint32_t *_prev_lostPackets;

Time prevTime = Seconds(0);

void _initialize_prevs(int _number_of_flows)
{
    // prev = new uint32_t[2 * _number_of_flows];
    _prev_txBytes = new uint32_t[2 * _number_of_flows];
    _prev_rxBytes = new uint32_t[2 * _number_of_flows];
    _prev_delaySum = new double[2 * _number_of_flows];
    _prev_rxPackets = new uint32_t[2 * _number_of_flows];
    _prev_lostPackets = new uint32_t[2 * _number_of_flows];

    for (int i = 0; i < 2 * _number_of_flows; i++)
    {
        _prev_txBytes[i] = 0;
        _prev_rxBytes[i] = 0;
        _prev_delaySum[i] = 0;
        _prev_rxPackets[i] = 0;
        _prev_lostPackets[i] = 0;
    }
}

std::string _output_file_name = "_state.csv";

// Calculate throughput
static void
TraceThroughput(Ptr<FlowMonitor> monitor)
{
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    Time curTime = Now();

    int cnt = 0;
    uint32_t _total_throughput = 0;
    double _total_delay = 0;
    uint32_t _total_recived_packets = 0;
    uint32_t _total_lost_packets = 0;
    uint32_t _total_cumulative_lost_packets = 0;
    double _total_cumulative_throughput = 0;
    for (auto itr = stats.begin(); itr != stats.end(); itr++)
    {
        // std::cout << cnt << std::endl;
        // std::ofstream thr(dir + "/throughput" + std::to_string(cnt + 1) + ".dat", std::ios::out | std::ios::app);
        // throughput
        uint32_t dx_throughput = (itr->second.txBytes - _prev_txBytes[cnt]);
        _total_throughput += dx_throughput;

        // cumulative throughput
        _total_cumulative_throughput += (itr->second.txBytes);

        // delay
        double dx_delay = (itr->second.delaySum.GetSeconds() - _prev_delaySum[cnt]);
        uint32_t dx_packets_received = (itr->second.rxPackets - _prev_rxPackets[cnt]);

        _total_delay += dx_delay;
        _total_recived_packets += dx_packets_received;

        // packet loss in number of packets, we should change it to byte according to paper
        uint32_t dx_packet_lost = (itr->second.lostPackets - _prev_lostPackets[cnt]);
        _total_lost_packets += dx_packet_lost;

        // cumulative lost packets
        _total_cumulative_lost_packets += itr->second.lostPackets;

        // update of arrays
        _prev_txBytes[cnt] = itr->second.txBytes;
        _prev_rxBytes[cnt] = itr->second.rxBytes;
        _prev_delaySum[cnt] = itr->second.delaySum.GetSeconds();
        _prev_rxPackets[cnt] = itr->second.rxPackets;
        _prev_lostPackets[cnt] = itr->second.lostPackets;

        // thr << curTime.GetSeconds() << " " << 8 * (itr->second.txBytes - prev[cnt]) / (1000 * 1000 * (curTime.GetSeconds() - prevTime.GetSeconds())) << std::endl;
        // prev[cnt] = itr->second.txBytes;
        cnt++;
    }
    // std::cout << cnt << std::endl;
    std::ofstream out(dir + _output_file_name, std::ios::out | std::ios::app);
    double _dx_time = curTime.GetSeconds() - prevTime.GetSeconds();
    out << curTime.GetSeconds();
    out << "," << 8 * _total_throughput / (1000 * 1000 * _dx_time);                         // throughput
    out << "," << 8 * _total_cumulative_throughput / (1000 * 1000 * curTime.GetSeconds());  // cumulative
    out << "," << (_total_recived_packets > 0 ? _total_delay / _total_recived_packets : 1); // delay
    out << "," << _total_lost_packets;
    out << "," << _total_cumulative_lost_packets; // cumul lost packets
    out << std::endl;                             //

    prevTime = curTime;

    Simulator::Schedule(Seconds(0.2), &TraceThroughput, monitor);
}

void shFlow(FlowMonitorHelper *flowmon, Ptr<FlowMonitor> monitor, std::string flow_file)
{
    AsciiTraceHelper ascii;
    static Ptr<OutputStreamWrapper> flowStream = ascii.CreateFileStream(flow_file);

    *flowStream->GetStream() << "Session DeliveryRatio LossRatio\n";
    uint32_t SentPackets = 0;
    uint32_t ReceivedPackets = 0;
    uint32_t LostPackets = 0;
    int j = 0;
    Ptr<Ipv6FlowClassifier> classifier = DynamicCast<Ipv6FlowClassifier>(flowmon->GetClassifier6());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
    {
        Ipv6FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
        NS_LOG_UNCOND("\n");
        NS_LOG_UNCOND("----Flow ID : " << iter->first);
        NS_LOG_UNCOND(t.sourceAddress << " ===> " << t.destinationAddress);
        NS_LOG_UNCOND("Sent Packets = " << iter->second.txPackets);
        NS_LOG_UNCOND("Received Packets = " << iter->second.rxPackets);
        NS_LOG_UNCOND("Lost Packets =" << iter->second.txPackets - iter->second.rxPackets);
        NS_LOG_UNCOND("Packet delivery ratio = " << iter->second.rxPackets * 100.0 / iter->second.txPackets << "%");
        NS_LOG_UNCOND("Packet loss ratio = " << (iter->second.lostPackets) * 100.0 / iter->second.txPackets << "%");

        *flowStream->GetStream() << t.sourceAddress << "-" << t.destinationAddress << " " << iter->second.rxPackets * 100.0 / iter->second.txPackets << " " << (iter->second.txPackets - iter->second.rxPackets) * 100.0 / iter->second.txPackets << "\n";

        SentPackets = SentPackets + (iter->second.txPackets);
        ReceivedPackets = ReceivedPackets + (iter->second.rxPackets);
        LostPackets = LostPackets + (iter->second.lostPackets);

        j++;
    }
    NS_LOG_UNCOND("--------Total Results of the simulation----------" << std::endl);
    NS_LOG_UNCOND("Total sent packets  =" << SentPackets);
    NS_LOG_UNCOND("Total Received Packets =" << ReceivedPackets);
    NS_LOG_UNCOND("Total Lost Packets =" << LostPackets);
    NS_LOG_UNCOND("Packet Loss ratio =" << ((LostPackets * 100.0) / SentPackets) << "%");
    NS_LOG_UNCOND("Packet delivery ratio =" << ((ReceivedPackets * 100.0) / SentPackets) << "%");
    NS_LOG_UNCOND("Total Flod id " << j);
}

int main(int argc, char **argv)
{
    LogComponentEnable("_low_rate", LOG_DEBUG);
    // LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    LogComponentEnable("LrWpanHelper", LOG_ALL);
    NS_LOG_DEBUG("Starting !");

    CommandLine cmd(__FILE__);
    // cmd.AddValue("useIpv6", "Use Ipv6", useV6);
    cmd.Parse(argc, argv);

    int numberOfNodes = 6;
    int _num_left_nodes = numberOfNodes / 2;
    int _num_right_nodes = numberOfNodes / 2;
    Time stopTime = Seconds(100);
    srand(0);

    // int _number_of_flows = 2;

    NodeContainer l_nodes;
    l_nodes.Create(_num_left_nodes);

    NodeContainer r_nodes;
    r_nodes.Create(_num_right_nodes);

    NodeContainer w_nodes;
    w_nodes.Add(l_nodes.Get(0));
    w_nodes.Add(r_nodes.Get(0));

    MobilityHelper l_mobility;
    l_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    l_mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue(0.0),
                                    "MinY", DoubleValue(0.0),
                                    "DeltaX", DoubleValue(.5),
                                    "DeltaY", DoubleValue(1),
                                    "GridWidth", UintegerValue(2),
                                    "LayoutType", StringValue("RowFirst"));

    l_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    l_mobility.Install(l_nodes);

    MobilityHelper r_mobility;
    r_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    r_mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue(35.0),
                                    "MinY", DoubleValue(-50),
                                    "DeltaX", DoubleValue(.5),
                                    "DeltaY", DoubleValue(1),
                                    "GridWidth", UintegerValue(2),
                                    "LayoutType", StringValue("RowFirst"));

    r_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    r_mobility.Install(r_nodes);

    LrWpanHelper l_lrWpanHelper;

    NetDeviceContainer l_lrwpanDevices = l_lrWpanHelper.Install(l_nodes);

    l_lrWpanHelper.AssociateToPan(l_lrwpanDevices, 0);

    LrWpanHelper r_lrWpanHelper;

    NetDeviceContainer r_lrWpanDevices = r_lrWpanHelper.Install(r_nodes);

    r_lrWpanHelper.AssociateToPan(r_lrWpanDevices, 1);

    InternetStackHelper internetv6;
    internetv6.SetIpv4StackInstall(false);
    internetv6.InstallAll();

    SixLowPanHelper l_sixLowPanHelper;
    NetDeviceContainer l_devices = l_sixLowPanHelper.Install(l_lrwpanDevices);

    SixLowPanHelper r_sixLowPanHelper;
    NetDeviceContainer r_devices = r_sixLowPanHelper.Install(r_lrWpanDevices);

    PointToPointHelper p2pHelper;
    p2pHelper.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2pHelper.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer w_devices = p2pHelper.Install(w_nodes);

    Ipv6AddressHelper address;
    // wired
    address.SetBase(Ipv6Address("2001:0000:f00d:cafe::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer w_interfaces;
    w_interfaces = address.Assign(w_devices);
    w_interfaces.SetForwarding(1, true);
    w_interfaces.SetForwarding(0, true);
    w_interfaces.SetDefaultRouteInAllNodes(1);

    // Ipv6StaticRoutingHelper routingHelper;

    // left
    address.SetBase("2001:f00d:cafe::", Ipv6Prefix(64));
    Ipv6InterfaceContainer l_interfaces = address.Assign(l_devices);
    l_interfaces.SetForwarding(0, true);

    l_interfaces.SetDefaultRouteInAllNodes(0);

    for (u_int i = 0; i < l_devices.GetN(); i++)
    {
        Ptr<NetDevice> dev = l_devices.Get(i);
        dev->SetAttribute("UseMeshUnder", BooleanValue(true));
        dev->SetAttribute("MeshUnderRadius", UintegerValue(10));
    }

    // right
    address.SetBase("2001:f00e:cafe::", Ipv6Prefix(64));
    Ipv6InterfaceContainer r_interfaces = address.Assign(r_devices);
    r_interfaces.SetForwarding(0, true);
    r_interfaces.SetDefaultRouteInAllNodes(0);

    for (u_int i = 0; i < r_devices.GetN(); i++)
    {
        Ptr<NetDevice> dev = r_devices.Get(i);
        dev->SetAttribute("UseMeshUnder", BooleanValue(true));
        dev->SetAttribute("MeshUnderRadius", UintegerValue(10));
    }

    ApplicationContainer serverApps;
    ApplicationContainer sinkApps;

    srand(0);
    for (int i = 1; i < _num_left_nodes; i++)
    {
        std::cout << i << std::endl;
        int sin = i;
        int ser = rand() % _num_right_nodes;
        /* Install TCP Receiver on the access point */
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", Inet6SocketAddress(Ipv6Address::GetAny(), 9));
        sinkApps.Add(sinkHelper.Install(r_nodes.Get(sin)));

        // /* Install TCP/UDP Transmitter on the station */
        OnOffHelper server("ns3::TcpSocketFactory", (Inet6SocketAddress(r_interfaces.GetAddress(sin, 1), 9)));
        server.SetAttribute("PacketSize", UintegerValue(1024));
        server.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        server.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        server.SetAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
        serverApps.Add(server.Install(l_nodes.Get(ser)));

        // std::cout << " after : " << i << std::endl;
    }

    // /* Start Applications */
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(50.0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(46.0));

    // Ptr<Node> left = w_nodes.Get(0);
    // Ptr<Node> right = w_nodes.Get(1);

    // uint16_t sinkPort = 8080;
    // Address sinkAddress;
    // Address anyAddress;
    // sinkAddress = Inet6SocketAddress(
    //     w_interfaces.GetAddress(1, 1),
    //     sinkPort);

    // anyAddress = Inet6SocketAddress(Ipv6Address::GetAny(), sinkPort);
    // // NS_LOG_DEBUG(r_interfaces.GetAddress(1, 0));
    // // NS_LOG_DEBUG(r_interfaces.GetAddress(1, 1));
    // PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", anyAddress);

    // ApplicationContainer sinkApps = sinkHelper.Install(right);
    // sinkApps.Start(stopTime);
    // sinkApps.Stop(stopTime);

    // AddressValue remoteAddress(Inet6SocketAddress(w_interfaces.GetAddress(1, 1), sinkPort));

    // NS_LOG_DEBUG(w_interfaces.GetAddress(0, 1));
    // NS_LOG_DEBUG(w_interfaces.GetAddress(1, 1));

    // // NS_LOG_DEBUG(l_interfaces.GetAddress(1, 0));
    // // NS_LOG_DEBUG(l_interfaces.GetAddress(1, 1));

    // BulkSendHelper ftp("ns3::TcpSocketFactory", Ipv6Address());
    // ftp.SetAttribute("Remote", remoteAddress);
    // ApplicationContainer sourceApp = ftp.Install(left);
    // sourceApp.Start(stopTime);
    // sourceApp.Stop(stopTime);

    // Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(left, TcpSocketFactory::GetTypeId());

    // Ptr<MyApp> app = CreateObject<MyApp>();
    // app->Setup(ns3TcpSocket, sinkAddress, 1040, 1000, DataRate("10Mbps"));
    // left->AddApplication(app);
    // app->SetStartTime(Seconds(1.));
    // app->SetStopTime(Seconds(20.));

    // uint32_t packetSize = 10;
    // uint32_t maxPacketCount = 5;
    // Time interPacketInterval = Seconds(1.);
    // Ping6Helper ping6;

    // // ping6.SetLocal(wsnDeviceInterfaces.GetAddress(nWsnNodes - 1, 1));
    // // ping6.SetRemote(wiredDeviceInterfaces.GetAddress(0, 1));
    // ping6.SetLocal(r_interfaces.GetAddress(1, 1));
    // ping6.SetRemote(l_interfaces.GetAddress(1, 1));

    // maxPacketCount = 0;
    // ping6.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    // ping6.SetAttribute("Interval", TimeValue(interPacketInterval));
    // ping6.SetAttribute("PacketSize", UintegerValue(packetSize));
    // // ApplicationContainer apps = ping6.Install(wsnNodes.Get(nWsnNodes - 1));
    // ApplicationContainer apps = ping6.Install(r_nodes.Get(1));

    // apps.Start(Seconds(1.0));
    // apps.Stop(Seconds(10.0));
    // Simulator::Run();
    // Simulator::Destroy();
    // NS_LOG_DEBUG("Stopping!");

    // _output_file_name = dir + _output_file_name;
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Simulator::Schedule(Seconds(0 + 0.05), &TraceThroughput, monitor);
    // Simulator::Schedule(Seconds(0 + 0.04), &_initialize_prevs, _number_of_flows);

    Simulator::Stop(stopTime + TimeStep(2));
    Simulator::Run();

    // flowmon.SerializeToXmlFile("mytest.flowmonitor", true, true);
    Simulator::Destroy();

    shFlow(&flowmon, monitor, "_temp/_flowmon_stats.data");
    return 0;
}