
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

NS_LOG_COMPONENT_DEFINE("Task-A2");

std::string dir = "_temp";

std::string _output_file_name = "_details.csv";

int index_value = 0;
void printFlowDetails(FlowMonitorHelper *flowmon, Ptr<FlowMonitor> monitor, std::string flow_file)
{

    uint32_t SentPackets = 0;
    uint32_t ReceivedPackets = 0;
    uint32_t LostPackets = 0;
    uint32_t ThroughPut = 0;
    double Delay = 0;
    int j = 0;
    Ptr<Ipv6FlowClassifier> classifier = DynamicCast<Ipv6FlowClassifier>(flowmon->GetClassifier6());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
    {
        Ipv6FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
        NS_LOG_INFO("\n");
        NS_LOG_INFO("----Flow ID : " << iter->first);
        NS_LOG_INFO(t.sourceAddress << " ===> " << t.destinationAddress);
        NS_LOG_INFO("Sent Packets = " << iter->second.txPackets);
        NS_LOG_INFO("Received Packets = " << iter->second.rxPackets);
        NS_LOG_INFO("Lost Packets =" << iter->second.lostPackets);
        NS_LOG_INFO("Packet delivery ratio = " << iter->second.rxPackets * 100.0 / iter->second.txPackets << "%");
        NS_LOG_INFO("Packet loss ratio = " << (iter->second.txPackets - iter->second.rxPackets) * 100.0 / iter->second.txPackets << "%");

        SentPackets = SentPackets + (iter->second.txPackets);
        ReceivedPackets = ReceivedPackets + (iter->second.rxPackets);
        LostPackets = LostPackets + (iter->second.lostPackets);
        ThroughPut = ThroughPut + iter->second.txBytes /
                                      (iter->second.timeLastRxPacket - iter->second.timeFirstTxPacket).GetSeconds();
        Delay = Delay + iter->second.delaySum.GetSeconds();
        j++;
    }

    NS_LOG_DEBUG("--------Total Results of the simulation----------" << std::endl);
    NS_LOG_DEBUG("Total sent packets  =" << SentPackets);
    NS_LOG_DEBUG("Total Received Packets =" << ReceivedPackets);
    NS_LOG_DEBUG("Total Lost Packets =" << LostPackets);
    NS_LOG_DEBUG("Packet Loss ratio =" << ((LostPackets * 100.0) / SentPackets) << "%");
    NS_LOG_DEBUG("Packet delivery ratio =" << ((ReceivedPackets * 100.0) / SentPackets) << "%");
    NS_LOG_DEBUG("Throughput = " << (ThroughPut / 1024) << " kBps");
    NS_LOG_DEBUG("End-to-end Delay = " << (Delay / ReceivedPackets) << " s");
    NS_LOG_DEBUG("Total Flod id " << j);

    std::ofstream out(dir + _output_file_name, std::ios::app);
    out << index_value;
    out << "," << (ThroughPut / 1024);
    out << "," << (Delay / ReceivedPackets);
    out << "," << ((LostPackets * 100.0) / SentPackets);
    out << "," << ((ReceivedPackets * 100.0) / SentPackets);
    out << std::endl;
}

int main(int argc, char **argv)
{
    LogComponentEnable("Task-A2", LOG_DEBUG);
    // LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    // LogComponentEnable("LrWpanHelper", LOG_ALL);

    int first_data = 0;
    // Naming the output directory using local system time

    Time stopTime = Seconds(25);
    std::string changing_parameter = "node";
    int changed_value = 20;

    int number_of_nodes = 20;
    int number_of_flows = 10;
    int packet_size = 512;
    int packet_rate = 100;
    int coverage = 10;
    srand(0);

    CommandLine cmd(__FILE__);
    cmd.AddValue("first_data", "is it the first data? should we initiate our file", first_data);
    cmd.AddValue("stopTime", "Stop time for applications / simulation time will be stopTime + 1", stopTime);
    cmd.AddValue("changing_parameter", "what parameter are we changing", changing_parameter);
    cmd.AddValue("changed_value", "what is changed parameters value", changed_value);
    cmd.AddValue("number_of_nodes", "number of nodes", number_of_nodes);
    cmd.AddValue("number_of_flows", "number of flows", number_of_flows);
    cmd.AddValue("packet_rate", "packet rate ", packet_rate);
    cmd.AddValue("coverage", "coverage", coverage);

    cmd.Parse(argc, argv);

    if (changing_parameter == "node")
    {
        index_value = number_of_nodes;
    }
    else if (changing_parameter == "flow")
    {
        index_value = number_of_flows;
    }
    else if (changing_parameter == "packet_rate")
    {
        index_value = packet_rate;
    }
    else if (changing_parameter == "coverage")
    {
        index_value = coverage;
    }
    else
    {
        NS_LOG_UNCOND("NO PARAMETER ALTERED!");
    }

    NS_LOG_DEBUG(number_of_nodes << " " << number_of_flows << " " << packet_rate << " " << coverage);
    int datarate = packet_size * packet_rate;

    int _num_left_nodes = number_of_nodes;

    NodeContainer l_nodes;
    l_nodes.Create(_num_left_nodes);

    MobilityHelper l_mobility;
    l_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    l_mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue(0.0),
                                    "MinY", DoubleValue(0.0),
                                    "DeltaX", DoubleValue(.5),
                                    "DeltaY", DoubleValue(1.2),
                                    "GridWidth", UintegerValue(ceil(sqrt(_num_left_nodes))),
                                    "LayoutType", StringValue("RowFirst"));

    l_mobility.Install(l_nodes);

    LrWpanHelper l_lrWpanHelper;

    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<RangePropagationLossModel> propModel = CreateObject<RangePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    propModel->SetAttribute("MaxRange", DoubleValue(coverage * 50));
    channel->AddPropagationLossModel(propModel);

    l_lrWpanHelper.SetChannel(channel);

    NetDeviceContainer l_lrwpanDevices = l_lrWpanHelper.Install(l_nodes);

    l_lrWpanHelper.AssociateToPan(l_lrwpanDevices, 10);

    InternetStackHelper internetv6;
    internetv6.SetIpv4StackInstall(false);
    internetv6.InstallAll();

    SixLowPanHelper l_sixLowPanHelper;
    NetDeviceContainer l_devices = l_sixLowPanHelper.Install(l_lrwpanDevices);

    // SixLowPanHelper r_sixLowPanHelper;
    // NetDeviceContainer r_devices = r_sixLowPanHelper.Install(r_lrWpanDevices);

    Ipv6AddressHelper address;

    // Ipv6StaticRoutingHelper routingHelper;

    // left
    address.SetBase("2001:f00d:cafe::", Ipv6Prefix(64));
    Ipv6InterfaceContainer l_interfaces = address.Assign(l_devices);
    // l_interfaces.SetForwarding(0, true);

    // l_interfaces.SetDefaultRouteInAllNodes(0);

    // for (u_int i = 0; i < l_devices.GetN(); i++)
    // {
    //     Ptr<NetDevice> dev = l_devices.Get(i);
    //     dev->SetAttribute("UseMeshUnder", BooleanValue(true));
    //     dev->SetAttribute("MeshUnderRadius", UintegerValue(10));
    // }

    ApplicationContainer sourceApps;
    ApplicationContainer sinkApps;
    uint_fast32_t unique_port = 45345;

    for (int i = 0; i < number_of_flows / 2; i++)
    {
        // std::cout << i << std::endl;
        int sin = i % _num_left_nodes;
        int ser = rand() % _num_left_nodes;
        if (sin == ser)
        {
            i--;
            continue;
        }
        /* Install TCP Receiver on the access point */
        unique_port++;
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", Inet6SocketAddress(Ipv6Address::GetAny(), unique_port));
        sinkApps.Add(sinkHelper.Install(l_nodes.Get(sin)));

        // /* Install TCP/UDP Transmitter on the station */
        OnOffHelper server("ns3::TcpSocketFactory", (Inet6SocketAddress(l_interfaces.GetAddress(sin, 1), unique_port)));
        server.SetAttribute("PacketSize", UintegerValue(packet_size));
        server.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        server.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        server.SetAttribute("DataRate", DataRateValue(DataRate(datarate)));
        sourceApps.Add(server.Install(l_nodes.Get(ser)));

        // std::cout << " after : " << i << std::endl;
    }

    sourceApps.Start(Seconds(0.0));
    sourceApps.Stop(stopTime);
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(stopTime + Seconds(5));

    // Create a new directory to store the output of the program
    dir = "_temp/" + std::string("a2") + "/";

    std::string dirToSave = "mkdir -p " + dir;
    if (system(dirToSave.c_str()) == -1)
    {
        exit(1);
    }

    // Check for dropped packets using Flow Monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    _output_file_name = changing_parameter + _output_file_name;

    if (first_data)
    {
        std::ofstream out(dir + _output_file_name, std::ios::out);
        out << changing_parameter << ", "
            << "throughput(kBps), "
            << "end-to-end delay(s),"
            << "lost packets ratio(%), "
            << "packet delivery ratio(%)" << std::endl;
        out.flush();
        out.close();
    }

    Simulator::Stop(stopTime + Seconds(2));
    Simulator::Run();

    // flowmon.SerializeToXmlFile("mytest.flowmonitor", true, true);
    Simulator::Destroy();

    printFlowDetails(&flowmon, monitor, _output_file_name);
}