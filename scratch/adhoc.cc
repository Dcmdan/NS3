/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AdHocExample");

int
main(int argc,char *argv[])
{
	Time::SetResolution (Time::NS);

	LogComponentEnable ("AdHocExample", LOG_LEVEL_INFO);
	 // LogComponentEnable ("TcpL4Protocol", LOG_LEVEL_INFO);
	LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);

	uint32_t nAdHoc=8;

	CommandLine cmd;

	cmd.AddValue ("nAdHoc", "Number of wifi ad devices", nAdHoc);

	cmd.Parse (argc,argv);

	NodeContainer AdHocNode;
	AdHocNode.Create(nAdHoc);

	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
	phy.SetChannel (channel.Create ());

	WifiHelper wifi;
	wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager","DataMode",StringValue("DsssRate11Mbps"));

	NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
	mac.SetType ("ns3::AdhocWifiMac",
	               "Slot", StringValue ("16us"));

	NetDeviceContainer AdHocDevices;
	AdHocDevices = wifi.Install(phy,mac,AdHocNode);

	MobilityHelper mobility;
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
	                                 "MinX", DoubleValue (0.0),
	                                 "MinY", DoubleValue (0.0),
	                                 "DeltaX", DoubleValue (5.0),
	                                 "DeltaY", DoubleValue (5.0),
	                                 "GridWidth", UintegerValue (10),
	                                 "LayoutType", StringValue ("RowFirst"));

	mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
	                             "Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)));
	mobility.Install (AdHocNode);

	InternetStackHelper Internet;
	Internet.Install(AdHocNode);

	Ipv4AddressHelper address;
	address.SetBase("195.1.1.0","255.255.255.0");

	Ipv4InterfaceContainer AdHocIp;
	AdHocIp = address.Assign(AdHocDevices);

	NS_LOG_INFO ("Create Applications.");
	uint16_t port = 9999;
	OnOffHelper onOff1("ns3::TcpSocketFactory",Address(InetSocketAddress(AdHocIp.GetAddress(0),port)));
	onOff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
	onOff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

	ApplicationContainer apps1 = onOff1.Install(AdHocNode);
	apps1.Start(Seconds(1.0));
	apps1.Stop(Seconds(500.0));

	PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", Address(InetSocketAddress (Ipv4Address::GetAny(), port)));
	ApplicationContainer apps2 = sinkHelper.Install(AdHocNode.Get(0));

	apps2.Start(Seconds(0.0));
	apps2.Stop(Seconds(500.0));

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	Simulator::Stop(Seconds(500.0));
	Simulator::Run();
	Simulator::Destroy();

	return 0;

}
