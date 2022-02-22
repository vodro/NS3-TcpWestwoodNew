
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

std::string dir = "_temp";

std::string _output_file_name = "_state.csv";

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

    int numberOfNodes = 64;
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
                                    "DeltaX", DoubleValue(.01),
                                    "DeltaY", DoubleValue(.01),
                                    "GridWidth", UintegerValue(ceil(sqrt(_num_left_nodes))),
                                    "LayoutType", StringValue("RowFirst"));

    l_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    l_mobility.Install(l_nodes);

    MobilityHelper r_mobility;
    r_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    r_mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue(35.0),
                                    "MinY", DoubleValue(-50),
                                    "DeltaX", DoubleValue(.01),
                                    "DeltaY", DoubleValue(.01),
                                    "GridWidth", UintegerValue(ceil(sqrt(_num_left_nodes))),
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
    address.SetBase(Ipv6Address("2001:0000:cafe::"), Ipv6Prefix(64));
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
    address.SetBase("2001:f10d:cafe::", Ipv6Prefix(64));
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
    uint_fast32_t unique_port = 45345;
    srand(0);
    // for (int i = 1; i < _num_left_nodes; i++)
    // {
    //     std::cout << i << std::endl;
    //     int sin = i;
    //     int ser = rand() % _num_right_nodes;
    //     /* Install TCP Receiver on the access point */
    //     PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", Inet6SocketAddress(Ipv6Address::GetAny(), 9));
    //     sinkApps.Add(sinkHelper.Install(r_nodes.Get(sin)));

    //     // /* Install TCP/UDP Transmitter on the station */
    //     OnOffHelper server("ns3::TcpSocketFactory", (Inet6SocketAddress(r_interfaces.GetAddress(sin, 1), 9)));
    //     server.SetAttribute("PacketSize", UintegerValue(1024));
    //     server.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    //     server.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    //     server.SetAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
    //     serverApps.Add(server.Install(l_nodes.Get(ser)));

    //     // std::cout << " after : " << i << std::endl;
    // }

    for (int i = 1; i < _num_left_nodes; i++)
    {
        // std::cout << i << std::endl;
        int sin = 0;
        int ser = i;
        unique_port++;
        /* Install TCP Receiver on the access point */
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", Inet6SocketAddress(Ipv6Address::GetAny(), unique_port));
        sinkApps.Add(sinkHelper.Install(r_nodes.Get(sin)));

        // /* Install TCP/UDP Transmitter on the station */
        OnOffHelper server("ns3::TcpSocketFactory", (Inet6SocketAddress(r_interfaces.GetAddress(sin, 1), unique_port)));
        server.SetAttribute("PacketSize", UintegerValue(1024));
        server.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        server.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        server.SetAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
        serverApps.Add(server.Install(l_nodes.Get(ser)));

        // std::cout << " after : " << i << std::endl;
    }

    for (int i = 1; i < _num_left_nodes; i++)
    {
        // std::cout << i << std::endl;
        int sin = i;
        int ser = 0;
        unique_port++;
        /* Install TCP Receiver on the access point */
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", Inet6SocketAddress(Ipv6Address::GetAny(), unique_port));
        sinkApps.Add(sinkHelper.Install(r_nodes.Get(sin)));

        // /* Install TCP/UDP Transmitter on the station */
        OnOffHelper server("ns3::TcpSocketFactory", (Inet6SocketAddress(r_interfaces.GetAddress(sin, 1), unique_port)));
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