#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#define M 5

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AdHocExample");

Ptr<Socket>
SetupPacketReceive (Ipv4Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, 80);
  sink->Bind (local);
  return sink;
}

int main (int argc, char *argv[])
{
  double simtime = 100.0;
  std::string phyMode ("DsssRate11Mbps");

  CommandLine cmd;
  cmd.Parse (argc, argv);

  LogComponentEnable ("IOlsrRoutingProtocol", LOG_LEVEL_DEBUG);
  LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  LogComponentEnable ("AdHocExample", LOG_LEVEL_ALL);
  //LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  NodeContainer olsrNodes;
  olsrNodes.Create (M*M);
  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;

  //wifi.EnableLogComponents ();  // Turn on all Wifi logging

  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac", "Slot", StringValue ("16us"));
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, olsrNodes);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint32_t i = 0; i<M*M; i++ )
  {
	  positionAlloc->Add (Vector (80 * (i%M) + 80 * (i/M), 80 * (i%M), 0.0));
  }
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (olsrNodes);

  AodvHelper iolsr;
  Ipv4ListRoutingHelper list;
  InternetStackHelper internet_olsr;

  list.Add (iolsr, 10);
  internet_olsr.SetRoutingHelper (list); // has effect on the next Install ()
  internet_olsr.Install (olsrNodes);

  NS_LOG_INFO ("Assign IP Addresses.");

  Ipv4AddressHelper ipv4;
  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (devices);

  NS_LOG_INFO ("Create Applications.");

  UdpEchoServerHelper echoServer (28);

  ApplicationContainer serverApps = echoServer.Install (olsrNodes.Get (0));
  serverApps.Start (Seconds (11.5));
  serverApps.Stop (Seconds (simtime));

  UdpEchoClientHelper echoClient (adhocInterfaces.GetAddress (0), 28);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (100));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024*2));

  ApplicationContainer clientApps = echoClient.Install (olsrNodes.Get (M*M-1));
  clientApps.Start (Seconds (12.5));
  clientApps.Stop (Seconds (simtime));

  Simulator::Stop (Seconds (simtime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
