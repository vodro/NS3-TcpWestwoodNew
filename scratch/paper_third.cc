

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include <stdlib.h>
#include <time.h>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

#include <typeinfo>

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6 (s)n7   n0 -------------- n1 n2(si) n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("paper_third");
class _Edge
{
public:
    _Edge() {}

    _Edge(int first, int second, const std::string &dataRate, const std::string &delay) : first(first), second(second),
                                                                                          dataRate(dataRate),
                                                                                          delay(delay) {}

    int first;
    int second;
    std::string dataRate;
    std::string delay;
};

std::string dir;
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
    double _total_cumulative_delay = 0;
    uint32_t _total_cumulative_received_packets = 0;
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

        // cumulative delay
        _total_cumulative_delay += (itr->second.delaySum.GetSeconds());
        _total_cumulative_received_packets += itr->second.rxPackets;

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
    // std::cout << _tota << std::endl;
    std::ofstream out(dir + _output_file_name, std::ios::out | std::ios::app);
    double _dx_time = curTime.GetSeconds() - prevTime.GetSeconds();
    out << curTime.GetSeconds();
    out << "," << 8 * _total_throughput / (1024 * _dx_time);                        // throughput
    out << "," << 8 * _total_cumulative_throughput / (1024 * curTime.GetSeconds()); // cumulative
    out << "," << (_total_recived_packets > 0 ? _total_delay : 1);                  // delay
    out << "," << (_total_cumulative_received_packets > 0 ? _total_cumulative_delay : 1);
    out << "," << _total_lost_packets;
    out << "," << _total_cumulative_lost_packets; // cumul lost packets
    out << std::endl;                             //

    prevTime = curTime;

    Simulator::Schedule(Seconds(0.1), &TraceThroughput, monitor);
}

void shFlow(FlowMonitorHelper *flowmon, Ptr<FlowMonitor> monitor, std::string flow_file)
{
    // AsciiTraceHelper ascii;
    // static Ptr<OutputStreamWrapper> flowStream = ascii.CreateFileStream(flow_file);

    // *flowStream->GetStream() << "Session DeliveryRatio LossRatio\n";
    uint32_t SentPackets = 0;
    uint32_t ReceivedPackets = 0;
    uint32_t LostPackets = 0;
    int j = 0;
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon->GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
        NS_LOG_UNCOND("\n");
        NS_LOG_UNCOND("----Flow ID : " << iter->first);
        NS_LOG_UNCOND(t.sourceAddress << " ===> " << t.destinationAddress);
        NS_LOG_UNCOND("Sent Packets = " << iter->second.txPackets);
        NS_LOG_UNCOND("Received Packets = " << iter->second.rxPackets);
        NS_LOG_UNCOND("Lost Packets =" << iter->second.txPackets - iter->second.rxPackets);
        NS_LOG_UNCOND("Packet delivery ratio = " << iter->second.rxPackets * 100.0 / iter->second.txPackets << "%");
        NS_LOG_UNCOND("Packet loss ratio = " << (iter->second.txPackets - iter->second.rxPackets) * 100.0 / iter->second.txPackets << "%");

        // *flowStream->GetStream() << t.sourceAddress << "-" << t.destinationAddress << " " << iter->second.rxPackets * 100.0 / iter->second.txPackets << " " << (iter->second.txPackets - iter->second.rxPackets) * 100.0 / iter->second.txPackets << "\n";

        SentPackets = SentPackets + (iter->second.txPackets);
        ReceivedPackets = ReceivedPackets + (iter->second.rxPackets);
        LostPackets = LostPackets + (iter->second.txPackets - iter->second.rxPackets);

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

int main(int argc, char *argv[])
{

    // LogComponentEnable("paper_third", LOG_DEBUG);
    // LogComponentEnable("TcpWestwoodNew", LOG_DEBUG);

    bool verbose = true;
    uint32_t nCsma = 3;
    uint32_t nWifi = 3;
    bool tracing = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    std::string transport_prot = "TcpWestwoodNew";
    // transport_prot = "TcpWestwood";

    // std::string tcpTypeId = "TcpBbr";
    //    std::string queueDisc = "FifoQueueDisc";
    uint32_t delAckCount = 2;
    //    bool bql = true;
    bool enablePcap = false;
    Time stopTime = Seconds(30);
    int _number_of_flows = 2;

    // CommandLine cmd(__FILE__);

    cmd.AddValue("transport_prot", "Transport protocol to use: TcpNewReno, TcpWestwood, TcpWestwoodNew", transport_prot);
    cmd.AddValue("delAckCount", "Delayed ACK count", delAckCount);
    cmd.AddValue("enablePcap", "Enable/Disable pcap file generation", enablePcap);
    cmd.AddValue("stopTime", "Stop time for applications / simulation time will be stopTime + 1", stopTime);
    cmd.Parse(argc, argv);

    cmd.Parse(argc, argv);

    std::cout << "our transport : " << transport_prot << std::endl;
    _output_file_name = transport_prot + _output_file_name;

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TypeId::LookupByName(std::string("ns3::") + transport_prot)));

    // The underlying restriction of 18 is due to the grid position
    // allocator's configuration; the grid layout will exceed the
    // bounding box if more than 18 nodes are provided.
    if (nWifi > 18)
    {
        std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
        return 1;
    }

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    // Configure the error model
    // Here we use RateErrorModel with packet error rate
    double error_p = 0.05;

    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    uv->SetStream(50);
    RateErrorModel error_model;
    error_model.SetRandomVariable(uv);
    error_model.SetUnit(RateErrorModel::ERROR_UNIT_PACKET);
    error_model.SetRate(error_p);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));
    pointToPoint.SetDeviceAttribute("ReceiveErrorModel", PointerValue(&error_model));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1));
    csmaNodes.Create(nCsma);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWifi);
    NodeContainer wifiApNode = p2pNodes.Get(0);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, mac, wifiApNode);

    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(wifiStaNodes);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);

    InternetStackHelper stack;
    stack.Install(csmaNodes);
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address.Assign(csmaDevices);

    address.SetBase("10.1.3.0", "255.255.255.0");
    address.Assign(staDevices);
    address.Assign(apDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Ptr<Node> sourceNode = wifiStaNodes.Get(1);
    Ptr<Node> sinkNode = csmaNodes.Get(1);

    Ptr<Ipv4> _ipv4_source = sourceNode->GetObject<Ipv4>();                                                              // Get Ipv4 instance of the node
                                                                                                                         //
    Ipv4Address addr_source = _ipv4_source->GetAddress(1 + rand() % (_ipv4_source->GetNInterfaces() - 1), 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
                                                                                                                         //

    Ptr<Ipv4> _ipv4_sink = sinkNode->GetObject<Ipv4>();                                                            // Get Ipv4 instance of the node
                                                                                                                   //
    Ipv4Address addr_sink = _ipv4_sink->GetAddress(1 + rand() % (_ipv4_sink->GetNInterfaces() - 1), 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

    NS_LOG_DEBUG(addr_sink << addr_source);

    int uniquePort = 44000;
    ++uniquePort;
    BulkSendHelper source("ns3::TcpSocketFactory",
                          InetSocketAddress(addr_sink, uniquePort));
    source.SetAttribute("MaxBytes", UintegerValue(0));

    ApplicationContainer sourceApps = source.Install(sourceNode);
    sourceApps.Start(Seconds(0.0));
    sourceApps.Stop(stopTime - Seconds(1));

    // Install application on the receiver
    PacketSinkHelper sink("ns3::TcpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), uniquePort));
    ApplicationContainer sinkApps = sink.Install(sinkNode);
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(stopTime);

    // Create a new directory to store the output of the program
    dir = "_temp/" + std::string("paper") + "/";
    // std::string dirToRem = "rm -R " + dir;
    // system(dirToRem.c_str());
    std::string dirToSave = "mkdir -p " + dir;
    if (system(dirToSave.c_str()) == -1)
    {
        exit(1);
    }

    // Check for dropped packets using Flow Monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    std::ofstream out(dir + _output_file_name, std::ios::out);
    out.clear();
    out << "time(s), "
        << "instantaneous_throughput(kbps), "
        << "avg_throughput(kbps), "
        << "instantaneous_delay(s),"
        << "avg_delay(s),"
        << "lost_packets, "
        << "_total_lost_packets" << std::endl;
    out.flush();
    out.close();
    Simulator::Schedule(Seconds(0 + 0.01), &TraceThroughput, monitor);
    Simulator::Schedule(Seconds(0 + 0.00), &_initialize_prevs, _number_of_flows);

    Simulator::Stop(stopTime + TimeStep(2));
    Simulator::Run();

    // flowmon.SerializeToXmlFile("mytest.flowmonitor", true, true);
    Simulator::Destroy();

    shFlow(&flowmon, monitor, dir + transport_prot + "_flowmon_stats.data");

    return 0;
}
