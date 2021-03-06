/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of Kansas
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
 * Author: Justin Rohrer <rohrej@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

/*
 * This example program allows one to run ns-3 DSDV, AODV, or OLSR under
 * a typical random waypoint mobility model.
 *
 * By default, the simulation runs for 200 simulated seconds, of which
 * the first 50 are used for start-up time.  The number of nodes is 50.
 * Nodes move according to RandomWaypointMobilityModel with a speed of
 * 20 m/s and no pause time within a 300x1500 m region.  The WiFi is
 * in ad hoc mode with a 2 Mb/s rate (802.11b) and a Friis loss model.
 * The transmit power is set to 7.5 dBm.
 *
 * It is possible to change the mobility and density of the network by
 * directly modifying the speed and the number of nodes.  It is also
 * possible to change the characteristics of the network by changing
 * the transmit power (as power increases, the impact of mobility
 * decreases and the effective density increases).
 *
 * By default, OLSR is used, but specifying a value of 2 for the protocol
 * will cause AODV to be used, and specifying a value of 3 will cause
 * DSDV to be used.
 *
 * By default, there are 10 source/sink data pairs sending UDP data
 * at an application rate of 2.048 Kb/s each.    This is typically done
 * at a rate of 4 64-byte packets per second.  Application data is
 * started at a random time between 50 and 51 seconds and continues
 * to the end of the simulation.
 *
 * The program outputs a few items:
 * - packet receptions are notified to stdout such as:
 *   <timestamp> <node-id> received one packet from <src-address>
 * - each second, the data reception statistics are tabulated and output
 *   to a comma-separated value (csv) file
 * - some tracing and flow monitor configuration that used to work is
 *   left commented inline in the program
 */

#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/iolsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/energy-module.h"
#include "ns3/li-ion-battery-model.h"
#include "ns3/buildings-module.h"
#include "ns3/wifi-radio-energy-model.h"
#include "ns3/simple-device-energy-model.h"

#define nWifis 50

using namespace ns3;
using namespace dsr;

NS_LOG_COMPONENT_DEFINE ("manet-routing-compare");


class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run (int nSinks, double txp, std::string CSVfileName);
  //static void SetMACParam (ns3::NetDeviceContainer & devices,
  //                                 int slotDistance);
  std::string CommandSetup (int argc, char **argv);

private:
  Ptr<Socket> SetupPacketReceive (Ipv4Address addr, Ptr<Node> node);
  Ptr<Socket> SetupPacketSend (Ipv4Address addr, Ptr<Node> node);
  void ReceivePacket (Ptr<Socket> socket);
  void SendPacket (Ptr<Socket> socket, uint32_t);
  void CheckThroughput ();

  uint32_t port;
  uint32_t bytesTotal;
  uint32_t packetsReceived;
  uint32_t packetsSended;

  std::string m_CSVfileName;
  int m_nSinks;
  std::string m_protocolName;
  double m_txp;
  bool m_traceMobility;
  uint32_t m_protocol;
};

RoutingExperiment::RoutingExperiment ()
  : port (9),
    bytesTotal (0),
    packetsReceived (0),
	 packetsSended (0),
    m_CSVfileName ("manet-routing.output.csv"),
    m_traceMobility (false),
    m_protocol (0) // AODV
{
}

static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (InetSocketAddress::IsMatchingType (senderAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (senderAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}

void
RoutingExperiment::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address senderAddress;
  while ((packet = socket->RecvFrom (senderAddress)))
    {
      bytesTotal += packet->GetSize ();
      packetsReceived += 1;
      NS_LOG_UNCOND (PrintReceivedPacket (socket, packet, senderAddress));
    }
}

void
RoutingExperiment::SendPacket (Ptr<Socket> socket, uint32_t a)
{
	std::ostringstream oss;
	packetsSended += 1;
	std::cout << "Sended one packet, " << packetsSended << "in total" << std::endl;
}

void
RoutingExperiment::CheckThroughput ()
{
  double kbs = (bytesTotal * 8.0) / 1000;
  bytesTotal = 0;

  std::ofstream out (m_CSVfileName.c_str (), std::ios::app);

  out << (Simulator::Now ()).GetSeconds () << ","
      << kbs << ","
      << packetsReceived << ","
      << m_nSinks << ","
      << m_protocolName << ","
      << m_txp << ""
      << std::endl;

  out.close ();
  packetsReceived = 0;
  Simulator::Schedule (Seconds (1.0), &RoutingExperiment::CheckThroughput, this);
}

Ptr<Socket>
RoutingExperiment::SetupPacketReceive (Ipv4Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));
  return sink;
}
Ptr<Socket>
RoutingExperiment::SetupPacketSend (Ipv4Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, port);
  sink->Bind (local);
  sink->SetSendCallback (MakeCallback (&RoutingExperiment::SendPacket, this));
  return sink;
}

std::string
RoutingExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd;
  cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
  cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
  cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=DSR", m_protocol);
  cmd.Parse (argc, argv);
  //LogComponentEnable ("OlsrRoutingProtocol", LOG_LEVEL_DEBUG);
  //LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
  return m_CSVfileName;
}

int
main (int argc, char *argv[])
{
  RoutingExperiment experiment;
  std::string CSVfileName = experiment.CommandSetup (argc,argv);

  //blank out the last output file and write the column headers
  std::ofstream out (CSVfileName.c_str ());
  out << "SimulationSecond," <<
  "ReceiveRate," <<
  "PacketsReceived," <<
  "NumberOfSinks," <<
  "RoutingProtocol," <<
  "TransmissionPower" <<
  std::endl;
  out.close ();

  int nSinks = 10;
  double txp = 7.5;

  experiment.Run (nSinks, txp, CSVfileName);
}

void
RemainingEnergy (std::string context, double oldValue, double remainingEnergy)
{
	if(remainingEnergy == 0)
	{
		std::cout << "At " << Simulator::Now ().GetSeconds () << "s Node " << context << " died.";
		//NS_LOG_UNCOND ("At " << Simulator::Now ().GetSeconds () << "s Node " << context << " died.");
	}
}
void
TotalEnergy (std::string context, double oldValue, double totalEnergy)
{
	std::cout << Simulator::Now ().GetSeconds () << "s Node " << context << " Total energy consumed by radio = " << totalEnergy << "J";
}

void
RoutingExperiment::Run (int nSinks, double txp, std::string CSVfileName)
{
  Packet::EnablePrinting ();
  m_nSinks = nSinks;
  m_txp = txp;
  m_CSVfileName = CSVfileName;

  double TotalTime = 200.0;
  std::string rate ("2048bps");
  std::string phyMode ("DsssRate11Mbps");
  std::string tr_name ("manet-routing-compare");
  int nodeSpeed = 5; //in m/s
  int nodePause = 0; //in s
  m_protocolName = "protocol";

  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));

  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

  NodeContainer adhocNodes;
  adhocNodes.Create (nWifis);

  // setting up wifi phy and channel using helpers
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.Set ("RxGain", DoubleValue (3) );
  wifiPhy.Set ("TxGain", DoubleValue (3) );
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));

  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);

  MobilityHelper mobilityAdhoc;
  int64_t streamIndex = 0; // used to get consistent mobility across scenarios

  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));

  Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
  streamIndex += taPositionAlloc->AssignStreams (streamIndex);

  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
  std::stringstream ssPause;
  ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
  mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                  "Speed", StringValue (ssSpeed.str ()),
                                  "Pause", StringValue (ssPause.str ()),
                                  "PositionAllocator", PointerValue (taPositionAlloc));
  mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
  mobilityAdhoc.Install (adhocNodes);
  streamIndex += mobilityAdhoc.AssignStreams (adhocNodes, streamIndex);

  AodvHelper aodv;
  OlsrHelper olsr;
  IOlsrHelper iolsr;
  DsdvHelper dsdv;
  DsrHelper dsr;
  DsrMainHelper dsrMain;
  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;

  switch (m_protocol)
    {
    case 0:
      list.Add (iolsr, 100);
      m_protocolName = "IOLSR";
      NS_LOG_INFO ("Protocol: IOLSR");
      break;
    case 1:
      list.Add (olsr, 100);
      m_protocolName = "OLSR";
      NS_LOG_INFO ("Protocol: OLSR");
      break;
    case 2:
      list.Add (aodv, 100);
      m_protocolName = "AODV";
      NS_LOG_INFO ("Protocol: AODV");
      break;
    case 3:
      list.Add (dsdv, 100);
      m_protocolName = "DSDV";
      NS_LOG_INFO ("Protocol: DSDV");
      break;
    case 4:
      m_protocolName = "DSR";
      NS_LOG_INFO ("Protocol: DSR");
      break;
    default:
      NS_FATAL_ERROR ("No such protocol:" << m_protocol);
    }

  if (m_protocol < 4)
    {
      internet.SetRoutingHelper (list);
      internet.Install (adhocNodes);
    }
  else if (m_protocol == 4)
    {
      internet.Install (adhocNodes);
      dsrMain.Install (dsr, adhocNodes);
    }

  NS_LOG_INFO ("assigning ip address");

  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (adhocDevices);

  LiIonBatteryModelHelper basicSourceHelper;
  // configure energy source
  basicSourceHelper.Set ("LiIonBatteryModelInitialEnergyJ", DoubleValue (31752)); //1W-h
  // install source
  EnergySourceContainer sources = basicSourceHelper.Install (adhocNodes);
  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.380));
  radioEnergyHelper.Set ("RxCurrentA", DoubleValue (0.313));
  radioEnergyHelper.Set ("IdleCurrentA", DoubleValue (0.273));
  // install device model
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (adhocDevices, sources);
  int index = 0;
  Ptr<LiIonBatteryModel> basicSourcePtr[nWifis];
  for(NodeContainer::Iterator j= adhocNodes.Begin(); j!= adhocNodes.End(); ++j)
    {

      std::string context = static_cast<std::ostringstream*>( &(std::ostringstream() << index) )->str();
      basicSourcePtr[index] = DynamicCast<LiIonBatteryModel> (sources.Get (index));
      basicSourcePtr[index]->SetInitialEnergy(15000 + index * ((31752 - 15000) / nWifis));
      basicSourcePtr[index]->TraceConnect ("RemainingEnergy", context, MakeCallback (&RemainingEnergy));
      // device energy model
      Ptr<DeviceEnergyModel> basicRadioModelPtr =
    		  basicSourcePtr[index]->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);

      NS_ASSERT (basicSourcePtr[index] != NULL);
      basicSourcePtr[index]->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&TotalEnergy));
      //nodes.Get (index)->AggregateObject (sources.Get (index));
      index++;
  }



  OnOffHelper onoff1 ("ns3::UdpSocketFactory",Address ());
  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

  for (int i = 0; i < nSinks; i++)
    {
      Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (i), adhocNodes.Get (i));
      Ptr<Socket> sink2 = SetupPacketSend (adhocInterfaces.GetAddress (i + nSinks), adhocNodes.Get (i + nSinks));

      AddressValue remoteAddress (InetSocketAddress (adhocInterfaces.GetAddress (i), port));
      onoff1.SetAttribute ("Remote", remoteAddress);

      Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
      ApplicationContainer temp = onoff1.Install (adhocNodes.Get (i + nSinks));
      temp.Start (Seconds (var->GetValue (100.0,101.0)));
      temp.Stop (Seconds (TotalTime));
    }

  std::stringstream ss;
  ss << nWifis;
  std::string nodes = ss.str ();

  std::stringstream ss2;
  ss2 << nodeSpeed;
  std::string sNodeSpeed = ss2.str ();

  std::stringstream ss3;
  ss3 << nodePause;
  std::string sNodePause = ss3.str ();

  std::stringstream ss4;
  ss4 << rate;
  std::string sRate = ss4.str ();

  AsciiTraceHelper ascii;
  MobilityHelper::EnableAsciiAll (ascii.CreateFileStream (tr_name + ".mob"));

  NS_LOG_INFO ("Run Simulation.");

  CheckThroughput ();

  uint32_t rxPacketsum = 0;
     double Delaysum = 0;
     uint32_t txPacketsum = 0;
     uint32_t txBytessum = 0;
     uint32_t rxBytessum = 0;
     uint32_t txTimeFirst = 0;
     uint32_t rxTimeLast = 0;
     uint32_t lostPacketssum = 0;
     double lastdelaysum = 0;
     double jittersum = 0;

     FlowMonitorHelper flowmon;
     Ptr<FlowMonitor> monitor = flowmon.InstallAll();

     Simulator::Stop (Seconds (TotalTime));
     Simulator::Run ();

     monitor->CheckForLostPackets ();

     Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
     std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

     for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
     {
         rxPacketsum += i->second.rxPackets;
         txPacketsum += i->second.txPackets;
         txBytessum += i->second.txBytes;
         rxBytessum += i->second.rxBytes;
         Delaysum += i->second.delaySum.GetSeconds();
         lostPacketssum += i->second.lostPackets;
         txTimeFirst += i->second.timeFirstTxPacket.GetSeconds();
         rxTimeLast += i->second.timeLastRxPacket.GetSeconds();
         jittersum += i->second.jitterSum.GetSeconds();
         lastdelaysum += i->second.lastDelay.GetSeconds();

     }
     index = 0;
     double remainEnergysum = 0;
     double remainEnergy[nWifis];
     double consumedEnergysum = 0;
     for(NodeContainer::Iterator j= adhocNodes.Begin(); j!= adhocNodes.End(); ++j)
       {
    	 remainEnergy[index] = basicSourcePtr[index]->GetRemainingEnergy();
    	 remainEnergysum += remainEnergy[index];
    	 consumedEnergysum += basicSourcePtr[index]->GetInitialEnergy() - remainEnergy[index];
       index++;
     }
    for (int i = 0 ; i < nWifis - 1; i++)
    {
    	for (int j = i + 1; j < nWifis; j++)
    	{
    		if (remainEnergy[i] > remainEnergy[j])
    		{
    			double t = remainEnergy[i];
    			remainEnergy[i] = remainEnergy[j];
    			remainEnergy[j] = t;
    		}
    	}
    }

     uint64_t timeDiff = (rxTimeLast - txTimeFirst);
     std::cout << "\n\n";
     std::cout << "  Total Tx Packets: " << txPacketsum << "\n";
     std::cout << "  Total Rx Packets: " << rxPacketsum << "\n";
     std::cout << "  Total Packets Lost: " << lostPacketssum << "\n";
     std::cout << "  Average Delay: " << Delaysum / rxPacketsum * 1000 << " ms\n";
     std::cout << "  Throughput: " << ((rxBytessum * 8.0) / timeDiff)/1024<<" Kbps"<<"\n";
     std::cout << "  Packets Delivery Ratio: " << (((txPacketsum - lostPacketssum) * 100) /txPacketsum) << "%" << "\n";
     std::cout << "  Total Jitter: " << jittersum * 1000<< " ms\n";
     std::cout << "  Total Delay: " << lastdelaysum * 1000 << " ms\n";
     std::cout << "  Average Remaining Energy: " << remainEnergysum / nWifis << " J\n";
     std::cout << "  Max Remaining Energy: " << remainEnergy[nWifis - 1] << " J\n";
     std::cout << "  Min Remaining Energy: " << remainEnergy[0] << " J\n";
     std::cout << "  Median Remaining Energy: " << remainEnergy[(int)(nWifis / 2)] << " J\n";
     std::cout << "  Average Consumed Energy: " << consumedEnergysum / nWifis << " J\n";

  Simulator::Destroy ();
}

