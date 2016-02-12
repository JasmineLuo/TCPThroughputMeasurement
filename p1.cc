/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/random-variable-stream.h"
#include <iostream>
#include <string>
#include <fstream>

using namespace ns3;
using std::fstream;
NS_LOG_COMPONENT_DEFINE ("ECE6110Project1");

int main(int argc, char *argv[])
{
    RngSeedManager::SetSeed(11223344);
    
    Time::SetResolution (Time::NS);
    LogComponentEnable ("BulkSendApplication", LOG_LEVEL_INFO);
    
    /* ---------------Initialization and get command input----------*/
    // Initialize in case not input from cmd
    uint32_t windowSize = 2000;
    uint32_t segSize = 128;
    uint32_t nFlows = 10;
    uint32_t queueSize = 2000; //optimal value in case of 1 flow
    uint16_t port=9;
    uint16_t tcpType=0;
    
    // Enable command line
    CommandLine cmd;
    cmd.AddValue("nFlows", "Number of Flows", nFlows);
    cmd.AddValue("segSize", "Segment Size", segSize);
    cmd.AddValue("queueSize", "Queue Size", queueSize);
    cmd.AddValue("windowSize", "Window Size", windowSize);
    cmd.AddValue("tcpType", "Tcp Type", tcpType);
    //cmd.AddValue("rngRun", "Rng Run", rngRun);
    cmd.Parse(argc,argv);
    
    
    /* ------------for first part 48 cases ---------------*/
    /*uint32_t windowSize[4]={ 2000, 8000, 32000, 64000};
    uint32_t queueSize[4]={ 2000, 8000, 32000, 64000};
    uint32_t segSize[3]={128, 256, 512};
    uint32_t nFlows=10;
    uint16_t port=9;
    
    fstream myfile;
    myfile.open ("/Users/luozhongyi/Desktop/Tools/ns-allinone-3.24.1/ns-3.24.1/scratch/TenFlowTcp-Reno.txt");
    //myfile << "Writing this to a file.\n";
    
    for(uint16_t win=0; win<4; win++){
        for(uint16_t que=0; que<4; que++ ){
            for(uint16_t seg=0; seg<3; seg++){
    */
    /* -------------end of setting 48 cases-----------*/
    
    // Set attributes for TCP
    Config::SetDefault("ns3::DropTailQueue::Mode", EnumValue(DropTailQueue::QUEUE_MODE_BYTES));
    Config::SetDefault("ns3::DropTailQueue::MaxBytes", UintegerValue(queueSize));
    
    Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue(windowSize));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segSize));
    // refer to the input
    
    if (tcpType==0)
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
    else
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpReno"));
    
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue(false));

    // fixed
    
    /* -----------------Generate Random startin time-----------------*/
    // Random stream for starting time
    
    Ptr<UniformRandomVariable> S = CreateObject<UniformRandomVariable>();
    S -> SetAttribute ("Stream", IntegerValue (6110));
    S -> SetAttribute("Min", DoubleValue(0.0));
    S -> SetAttribute("Max", DoubleValue(0.1));
    
    
    //set starting time as uncorrelated random numbers
    double setTime[nFlows];
    for(uint16_t k = 0; k < nFlows; k++) {
        setTime[k] = S -> GetValue();
    }
    
    /* ------------------Building Topology--------------------*/
    PointToPointHelper n0n1, n1n2, n2n3;
    // three links in total
    
    n0n1.SetDeviceAttribute("DataRate", StringValue ("5Mbps"));
    n0n1.SetChannelAttribute("Delay", StringValue ("10ms"));
    // left link for n0 and n1
    
    n1n2.SetDeviceAttribute("DataRate", StringValue ("1Mbps"));
    n1n2.SetChannelAttribute("Delay", StringValue ("20ms"));
    // middle bottleneck link for n1 and n2
    
    n2n3.SetDeviceAttribute("DataRate", StringValue ("5Mbps"));
    n2n3.SetChannelAttribute("Delay", StringValue ("10ms"));
    // right link for n2 and n3
    
    PointToPointDumbbellHelper total(nFlows, n0n1, nFlows, n2n3, n1n2);
    // use DumbbellHelper for three link(with one bottle neclk) and bottle neck
    // both sides has the nodes the same number as flow tcp number
    
    
    /* -------------------Adding protocol and ip address---------------*/
    InternetStackHelper stack;
    total.InstallStack(stack);
    // intal protocol stack on nodes
    
    Ipv4AddressHelper lip = Ipv4AddressHelper("128.196.0.0", "255.255.255.0");
    Ipv4AddressHelper rip = Ipv4AddressHelper("128.196.16.0", "255.255.255.0");
    Ipv4AddressHelper bip = Ipv4AddressHelper("128.196.32.0", "255.255.255.0");
    total.AssignIpv4Addresses(lip, rip, bip);
    // assign ip address
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    // establish routing table for all
    
    /* ------------------Adding application to source and sink--------------*/
    /* ------------Bulk send helper for source & packet sink helper for sink -------*/
    ApplicationContainer sendBox, sinkBox;
    for(uint16_t i = 0; i < nFlows; i++) {
        
        BulkSendHelper sender("ns3::TcpSocketFactory",InetSocketAddress(total.GetRightIpv4Address(i), port));
        // create new bulk sender in every loop and each has a different port
        sendBox.Add(sender.Install(total.GetLeft(i)));
        sender.SetAttribute("SendSize", UintegerValue(512));
        sender.SetAttribute("MaxBytes", UintegerValue(100000000));
        
        (sendBox.Get(i)) -> SetStartTime(Seconds(setTime[i]));
        (sendBox.Get(i)) -> SetStopTime(Seconds(10.0));
        // auto add to event schedule
        
        PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
        // create new sink in every loop and each has the same port number as first
        sinkBox.Add(sink.Install(total.GetRight(i)));
        
        (sinkBox.Get(i)) -> SetStartTime(Seconds(0.0));
        (sinkBox.Get(i)) -> SetStopTime(Seconds(10.0));
        // auto add to event schedule
    }
    
    /* ----------Simulation and output----------*/
    
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    
    
    Ptr<PacketSink> temp;
    //Ptr<BulkSendApplication> temp2;
    double Rxrecord[nFlows];
    double Rate[nFlows];
    
    for(uint16_t i = 0; i < nFlows; i++) {
        temp = DynamicCast<PacketSink>(sinkBox.Get(i));
        //temp2 = DynamicCast<Pa>(sendBox.Get(i));
        Rxrecord[i] = temp -> GetTotalRx(); //
        Rate[i] = Rxrecord[i]/(10.0-setTime[i]);////////////////////////////////////////////////////////////
        //Txrecord[i] = temp -> GetAcceptedSockets()/(10.0 - setTime[i]);
    }
    
    //std::cout <<  "Related Parameter: " << std::endl;
    //std::cout <<  "Flow number:" << nFlows << ";  WindowSize:" << windowSize << "; QueueSize:" << queueSize << "; Segment Size:" << segSize  << " Using Tcp-Tahoe. "<< std::endl;
    // export parameter with cout:
    
                for(uint16_t i=0; i < nFlows; i++){
                    std::cout <<"tcp,"<< tcpType<<",flow,"<< i << ",windowSize," <<windowSize<<",queueSize,"<<queueSize<<",segSize,"<<segSize<<",goodput,"<<Rate[i] << std::endl;
                    // export result for every flow
                    /*myfile << i+1 <<","<< windowSize[win]<<","<<queueSize[que]<<","<<segSize[seg]<<","<<Rxrecord[i]<<","<<Rate[i]<<"\n";*/
                }
         /*   }
        }
    }*/
    
    
    //myfile.close();
    
    Simulator::Destroy();
    return 0;
}
