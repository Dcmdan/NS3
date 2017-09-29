#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/command-line.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/basic-energy-source.h"
#include "ns3/energy-source-container.h"
#include "cstdio"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BasicModelTest");

FILE *fileV;
FILE *fileE;

static void
PrintCellInfo (Ptr<BasicEnergySource> es)
{
	std::cout << "At " << Simulator::Now ().GetSeconds () << " Cell voltage: " << es->GetSupplyVoltage () << " V Remaining Capacity: " <<
	  es->GetRemainingEnergy () << " J" << std::endl;
	fprintf(fileE, "%f \n\r", es->GetRemainingEnergy ());
	fprintf(fileV, "%f \n\r", es->GetSupplyVoltage ());

  if (!Simulator::IsFinished ())
    {
      Simulator::Schedule (Seconds (10),
                           &PrintCellInfo,
                           es);
    }
}

int
main (int argc, char **argv)
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  fileV = fopen("logV.txt", "w+");
  fileE = fopen("logE.txt", "w+");

  LogComponentEnable ("RvBatteryModel", LOG_LEVEL_INFO);

  Ptr<Node> node = CreateObject<Node> ();

  Ptr<SimpleDeviceEnergyModel> sem = CreateObject<SimpleDeviceEnergyModel> ();
  Ptr<EnergySourceContainer> esCont = CreateObject<EnergySourceContainer> ();
  Ptr<BasicEnergySource> es = CreateObject<BasicEnergySource> ();

  esCont->Add (es);
  es->SetNode (node);
  sem->SetEnergySource (es);
  es->AppendDeviceEnergyModel (sem);
  sem->SetNode (node);
  node->AggregateObject (esCont);
es->SetInitialEnergy(20000);
  Time now = Simulator::Now ();

  // discharge at 2.33 A for 1700 seconds
  sem->SetCurrentA (1);
  now += Seconds (6000);


  PrintCellInfo (es);

  Simulator::Stop (now);
  Simulator::Run ();
  Simulator::Destroy ();
}
