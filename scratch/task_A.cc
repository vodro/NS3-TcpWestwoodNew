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

void shFlow(FlowMonitorHelper *flowmon, Ptr<FlowMonitor> monitor, std::string flow_file)
{
    AsciiTraceHelper ascii;
    static Ptr<OutputStreamWrapper> flowStream = ascii.CreateFileStream(flow_file);

    *flowStream->GetStream() << "Session DeliveryRatio LossRatio\n";
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

        *flowStream->GetStream() << t.sourceAddress << "-" << t.destinationAddress << " " << iter->second.rxPackets * 100.0 / iter->second.txPackets << " " << (iter->second.txPackets - iter->second.rxPackets) * 100.0 / iter->second.txPackets << "\n";

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

NS_LOG_COMPONENT_DEFINE("task-A");

int main(int argc, char *argv[])
{

    // Naming the output directory using local system time

    Time stopTime = Seconds(10);

    CommandLine cmd(__FILE__);

    int numberOfNodes = 5;

    std::vector<_Edge *> _edges;
    _edges.push_back(new _Edge(0, 1, "100Mbps", "1ms"));
    _edges.push_back(new _Edge(1, 2, "5Mbps", "35ms"));
    _edges.push_back(new _Edge(2, 3, "100Mbps", "20ms"));
    _edges.push_back(new _Edge(2, 4, "100Mbps", "1ms"));

    NodeContainer allNodes;
    allNodes.Create(numberOfNodes);

    PointToPointHelper tempLink;
    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");

    InternetStackHelper internet;
    internet.Install(allNodes);

    // to make a ring we need numberOfNodes amount of edge
    for (int _i = 0; _i <= numberOfNodes - 1; _i++)
    {
        _Edge *edge = _edges.at(_i % _edges.size());

        tempLink.SetDeviceAttribute("DataRate", StringValue(edge->dataRate));
        tempLink.SetChannelAttribute("Delay", StringValue(edge->delay));

        NetDeviceContainer tempEdge = tempLink.Install(allNodes.Get(_i % numberOfNodes), allNodes.Get((_i + 1) % numberOfNodes));
        ipv4.NewNetwork();
        Ipv4InterfaceContainer tempConatainer = ipv4.Assign(tempEdge);
    }

    AsciiTraceHelper ascii;

    // Populate routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    int _number_of_flows = 10;

    srand(0);

    int uniquePort = 44000;
    for (int i = 0; i < _number_of_flows; i++)
    {
        int f = rand() % numberOfNodes;
        // f = 0;
        int s = rand() % numberOfNodes;
        // std::cout << f << " " << s << std::endl;
        // s = 4;

        if (s == f)
        {
            i--;
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
        BulkSendHelper source("ns3::TcpSocketFactory",
                              InetSocketAddress(addr_sink, uniquePort));
        source.SetAttribute("MaxBytes", UintegerValue(0));

        ApplicationContainer sourceApps = source.Install(sourceNode);
        sourceApps.Start(Seconds(0.0));
        sourceApps.Stop(stopTime - Seconds(3));

        // Install application on the receiver
        PacketSinkHelper sink("ns3::TcpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), uniquePort));
        ApplicationContainer sinkApps = sink.Install(sinkNode);
        sinkApps.Start(Seconds(0.0));
        sinkApps.Stop(stopTime);
    }
    std::string dir = "_temp/" + std::string("taskA") + "/";

    // Check for dropped packets using Flow Monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(stopTime + TimeStep(2));
    Simulator::Run();

    Simulator::Destroy();

    shFlow(&flowmon, monitor, dir + "_flowmon_stats.data");

    return 0;
}
