/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018-20 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Aarti Nandagiri <aarti.nandagiri@gmail.com>
 *          Vivek Jain <jain.vivek.anand@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

// This program simulates the following topology:
//
//           1000 Mbps           10Mbps          1000 Mbps
//  Sender -------------- R1 -------------- R2 -------------- Receiver
//              5ms               10ms               5ms
//
// The link between R1 and R2 is a bottleneck link with 10 Mbps. All other
// links are 1000 Mbps.
//
// This program runs by default for 100 seconds and creates a new directory
// called 'bbr-results' in the ns-3 root directory. The program creates one
// sub-directory called 'pcap' in 'bbr-results' directory (if pcap generation
// is enabled) and three .dat files.
//
// (1) 'pcap' sub-directory contains six PCAP files:
//     * bbr-0-0.pcap for the interface on Sender
//     * bbr-1-0.pcap for the interface on Receiver
//     * bbr-2-0.pcap for the first interface on R1
//     * bbr-2-1.pcap for the second interface on R1
//     * bbr-3-0.pcap for the first interface on R2
//     * bbr-3-1.pcap for the second interface on R2
// (2) cwnd.dat file contains congestion window trace for the sender node
// (3) throughput.dat file contains sender side throughput trace
// (4) queueSize.dat file contains queue length trace from the bottleneck link
//
// BBR algorithm enters PROBE_RTT phase in every 10 seconds. The congestion
// window is fixed to 4 segments in this phase with a goal to achieve a better
// estimate of minimum RTT (because queue at the bottleneck link tends to drain
// when the congestion window is reduced to 4 segments).
//
// The congestion window and queue occupancy traces output by this program show
// periodic drops every 10 seconds when BBR algorithm is in PROBE_RTT phase.

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

NS_LOG_COMPONENT_DEFINE("Task-A1");

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
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon->GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
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

int main(int argc, char *argv[])
{

    LogComponentEnable("Task-A1", LOG_DEBUG);
    // LogComponentEnable("TcpWestwoodNew", LOG_DEBUG);
    // LogComponentEnable("TcpWestwood", LOG_LEVEL_ALL);
    // LogComponentEnable("TcpSocketBase", LOG_FUNCTION);

    double error_p = 0.02;

    int first_data = 0;
    // Naming the output directory using local system time

    Time stopTime = Seconds(25);
    std::string changing_parameter = "node";
    int changed_value = 20;

    int number_of_nodes = 20;
    int number_of_flows = 10;
    int packet_size = 512 * 8 / 1024; // 1 KB
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

    NS_LOG_DEBUG("nodes : " << number_of_nodes << "; flows : " << number_of_flows << "; packet_rate : " << packet_rate << "; coverage_area: " << coverage);
    int datarate = packet_size * packet_rate;

    std::vector<_Edge *> _edges;
    _edges.push_back(new _Edge(0, 1, "1Mbps", "2ms"));
    // _edges.push_back(new _Edge(1, 2, "2Mbps", "5ms"));
    // _edges.push_back(new _Edge(1, 2, "1Mbps", "3ms"));

    // _edges.push_back(new _Edge(2, 3, "100Mbps", "20ms"));
    // _edges.push_back(new _Edge(2, 4, "100Mbps", "1ms"));

    NodeContainer allNodes;
    allNodes.Create(number_of_nodes);

    // Configure the error model
    // Here we use RateErrorModel with packet error rate
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    uv->SetStream(50);
    RateErrorModel error_model;
    error_model.SetRandomVariable(uv);
    error_model.SetUnit(RateErrorModel::ERROR_UNIT_PACKET);
    error_model.SetRate(error_p);

    PointToPointHelper tempLink;

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.1.0", "255.255.255.0");

    InternetStackHelper internet;
    internet.Install(allNodes);

    // number of edge is number of edge in ring topology
    for (int _i = 0; _i < number_of_nodes; _i++)
    {
        _Edge *edge = _edges.at(_i % _edges.size());
        tempLink.SetDeviceAttribute("DataRate", StringValue(edge->dataRate));
        tempLink.SetChannelAttribute("Delay", StringValue(edge->delay));

        tempLink.SetDeviceAttribute("ReceiveErrorModel", PointerValue(&error_model));

        NetDeviceContainer tempEdge = tempLink.Install(allNodes.Get(_i), allNodes.Get((_i + 1) % number_of_nodes));
        ipv4.NewNetwork();
        Ipv4InterfaceContainer tempConatainer = ipv4.Assign(tempEdge);
    }

    // Populate routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    //    Ipv4GlobalRoutingHelper::PrintRoutingTableAllAt(Seconds(5), ascii.CreateFileStream("_topology_table"));

    // NS_LOG_INFO(" FLOW NOW");
    ApplicationContainer sourceApps;
    ApplicationContainer sinkApps;
    int uniquePort = 44000;
    for (int i = 0; 2 * i < number_of_flows; i++)
    {
        int f = rand() % number_of_nodes;
        int s = rand() % number_of_nodes;
        s = i % number_of_nodes;
        f = (i + number_of_nodes / 2 - 1) % number_of_nodes;

        if (s == f)
        {
            i++;
            continue;
        }
        Ptr<Node> sourceNode = allNodes.Get(f);
        Ptr<Node> sinkNode = allNodes.Get(s);

        Ptr<Ipv4> _ipv4_source = sourceNode->GetObject<Ipv4>();                                                              // Get Ipv4 instance of the node
                                                                                                                             //
        Ipv4Address addr_source = _ipv4_source->GetAddress(1 + rand() % (_ipv4_source->GetNInterfaces() - 1), 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
                                                                                                                             //

        Ptr<Ipv4> _ipv4_sink = sinkNode->GetObject<Ipv4>();                                                            // Get Ipv4 instance of the node
                                                                                                                       //
        Ipv4Address addr_sink = _ipv4_sink->GetAddress(1 + rand() % (_ipv4_sink->GetNInterfaces() - 1), 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

        // NS_LOG_INFO(" source : " << f << " => " << addr_source);
        // NS_LOG_INFO(" sink   : " << s << " => " << addr_sink);
        NS_LOG_INFO("( " << f << ", " << s << " ) : " << addr_source << " => " << addr_sink);

        ++uniquePort;
        OnOffHelper source("ns3::TcpSocketFactory",
                           InetSocketAddress(addr_sink, uniquePort));

        source.SetAttribute("PacketSize", UintegerValue(packet_size));
        source.SetAttribute("MaxBytes", UintegerValue(0));
        source.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        source.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        // source.SetAttribute("DataRate", )
        source.SetAttribute("DataRate", DataRateValue(DataRate(datarate)));

        sourceApps.Add(source.Install(sourceNode));

        // Install application on the receiver
        PacketSinkHelper sink("ns3::TcpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), uniquePort));
        sinkApps.Add(sink.Install(sinkNode));
    }

    sourceApps.Start(Seconds(0.02));
    sourceApps.Stop(Seconds(10));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(50));
    // Create a new directory to store the output of the program
    dir = "_temp/" + std::string("a1") + "/";

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
            << "packet drop ratio(%), "
            << "packet delivery ratio(%)" << std::endl;
        out.flush();
        out.close();
    }

    Simulator::Stop(Seconds(50));
    Simulator::Run();

    // flowmon.SerializeToXmlFile("mytest.flowmonitor", true, true);
    Simulator::Destroy();

    printFlowDetails(&flowmon, monitor, _output_file_name);

    return 0;
}
