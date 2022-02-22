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

    Simulator::Schedule(Seconds(0.1), &TraceThroughput, monitor);
}

// Check the queue size
void CheckQueueSize(Ptr<QueueDisc> qd)
{
    uint32_t qsize = qd->GetCurrentSize().GetValue();
    Simulator::Schedule(Seconds(0.2), &CheckQueueSize, qd);
    std::ofstream q(dir + "/queueSize.dat", std::ios::out | std::ios::app);
    q << Simulator::Now().GetSeconds() << " " << qsize << std::endl;
    q.close();
}

// Trace congestion window
static void CwndTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newval / 1448.0 << std::endl;
}

void TraceCwnd(uint32_t nodeId, uint32_t socketId)
{
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(dir + "/cwnd.dat");
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string(socketId) + "/CongestionWindow", MakeBoundCallback(&CwndTracer, stream));
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

NS_LOG_COMPONENT_DEFINE("_topology");

int main(int argc, char *argv[])
{

    // LogComponentEnable("_topology", LOG_LEVEL_INFO);
    // LogComponentEnable("TcpWestwoodNew", LOG_DEBUG);
    // LogComponentEnable("TcpWestwood", LOG_LEVEL_ALL);
    // LogComponentEnable("TcpSocketBase", LOG_FUNCTION);

    double error_p = 0.00;

    // Naming the output directory using local system time
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%d-%m-%Y-%I-%M-%S", timeinfo);
    std::string currentTime(buffer);

    std::string transport_prot = "TcpWestwoodNew";
    // transport_prot = "TcpWestwood";

    // std::string tcpTypeId = "TcpBbr";
    //    std::string queueDisc = "FifoQueueDisc";
    uint32_t delAckCount = 2;
    //    bool bql = true;
    bool enablePcap = false;
    Time stopTime = Seconds(22);
    int _number_of_flows = 5;

    CommandLine cmd(__FILE__);

    cmd.AddValue("transport_prot", "Transport protocol to use: TcpNewReno, TcpWestwood, TcpWestwoodNew", transport_prot);
    cmd.AddValue("delAckCount", "Delayed ACK count", delAckCount);
    cmd.AddValue("enablePcap", "Enable/Disable pcap file generation", enablePcap);
    cmd.AddValue("stopTime", "Stop time for applications / simulation time will be stopTime + 1", stopTime);
    cmd.Parse(argc, argv);

    std::cout << "our transport : " << transport_prot << std::endl;
    _output_file_name = transport_prot + _output_file_name;
    // transport_prot = ;
    // NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe(transport_prot, &tcpTid), "TypeId " << tcpTypeId << " not found");
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TypeId::LookupByName(std::string("ns3::") + transport_prot)));

    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(41943));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(62914));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(delAckCount));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));

    int numberOfNodes = 5;

    std::vector<_Edge *> _edges;
    //    std::vector<std::vector< Ipv4InterfaceContainer *> > interfaces(numberOfNodes);
    _edges.push_back(new _Edge(0, 1, "100Mbps", "1ms"));
    _edges.push_back(new _Edge(1, 2, "5Mbps", "35ms"));
    _edges.push_back(new _Edge(2, 3, "100Mbps", "20ms"));
    _edges.push_back(new _Edge(2, 4, "100Mbps", "1ms"));

    NodeContainer allNodes;
    allNodes.Create(numberOfNodes);

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
    ipv4.SetBase("10.0.0.0", "255.255.255.0");

    InternetStackHelper internet;
    internet.Install(allNodes);
    for (int _i = 0; _i < numberOfNodes - 1; _i++)
    {
        _Edge *edge = _edges.at(_i);
        tempLink.SetDeviceAttribute("DataRate", StringValue(edge->dataRate));
        tempLink.SetChannelAttribute("Delay", StringValue(edge->delay));

        tempLink.SetDeviceAttribute("ReceiveErrorModel", PointerValue(&error_model));

        NetDeviceContainer tempEdge = tempLink.Install(allNodes.Get(edge->first), allNodes.Get(edge->second));
        ipv4.NewNetwork();
        Ipv4InterfaceContainer tempConatainer = ipv4.Assign(tempEdge);
    }

    AsciiTraceHelper ascii;

    // Populate routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    //    Ipv4GlobalRoutingHelper::PrintRoutingTableAllAt(Seconds(5), ascii.CreateFileStream("_topology_table"));

    NS_LOG_INFO(" FLOW NOW");

    bool isFixed = true;
    if (isFixed)
    {
        _number_of_flows = 3;
    }
    int _number_of_sources = 1;
    int _sources[] = {0, 4, 3};
    int _number_of_sinks = 3;
    int _sinks[] = {4, 3, 2};
    srand(0);

    int uniquePort = 44000;
    for (int i = 0; i < _number_of_flows; i++)
    {
        int f = _sources[rand() % _number_of_sources];
        // f = 0;
        int s = _sinks[i % (_number_of_sinks)];
        // s = 4;
        if (isFixed)
            f = _sources[i], s = _sinks[i];

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
        sourceApps.Stop(stopTime - Seconds(1));

        // Install application on the receiver
        PacketSinkHelper sink("ns3::TcpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), uniquePort));
        ApplicationContainer sinkApps = sink.Install(sinkNode);
        sinkApps.Start(Seconds(0.0));
        sinkApps.Stop(stopTime);
    }
    // Simulator::Schedule(Seconds(0.2), &TraceCwnd, 0, 0);

    //    for(int )

    //    NS_LOG_INFO(source- );
    //    Address sourceLocalAddress(InetSocketAddress(source->Ge))

    // Create a new directory to store the output of the program
    dir = "_temp/" + std::string("now") + "/";
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
        << "throughput(Mbps), "
        << "cumlative_throughput(Mbps), "
        << "avg_delay(s),"
        << "lost_packets, "
        << "_total_lost_packets" << std::endl;
    out.flush();
    out.close();
    Simulator::Schedule(Seconds(0 + 0.05), &TraceThroughput, monitor);
    Simulator::Schedule(Seconds(0 + 0.04), &_initialize_prevs, _number_of_flows);

    Simulator::Stop(stopTime + TimeStep(2));
    Simulator::Run();

    // flowmon.SerializeToXmlFile("mytest.flowmonitor", true, true);
    Simulator::Destroy();

    shFlow(&flowmon, monitor, dir + transport_prot + "_flowmon_stats.data");

    return 0;
}
