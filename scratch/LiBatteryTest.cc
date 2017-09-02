#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/command-line.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/li-ion-battery-model.h"
#include "ns3/energy-source-container.h"
#include "cstdio"

using namespace ns3;

FILE *fileV;
FILE *fileE;

static void
PrintCellInfo (Ptr<LiIonBatteryModel> es)
{
  std::cout << "At " << Simulator::Now ().GetSeconds () << " Cell voltage: " << es->GetSupplyVoltage () << " V Remaining Capacity: " <<
  es->GetRemainingEnergy () << " J" << std::endl;
	fprintf(fileE, "%f \n\r", es->GetRemainingEnergy ());
	fprintf(fileV, "%f \n\r", es->GetSupplyVoltage ());

  if (!Simulator::IsFinished ())
    {
      Simulator::Schedule (Seconds (1),
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

    // uncomment below to see the energy consumption details
  //LogComponentEnable ("LiIonBatteryModel", LOG_LEVEL_DEBUG);

  Ptr<Node> node = CreateObject<Node> ();

  Ptr<SimpleDeviceEnergyModel> sem = CreateObject<SimpleDeviceEnergyModel> ();
  Ptr<EnergySourceContainer> esCont = CreateObject<EnergySourceContainer> ();
  Ptr<LiIonBatteryModel> es = CreateObject<LiIonBatteryModel> ();
  esCont->Add (es);
  es->SetNode (node);
  sem->SetEnergySource (es);
  es->AppendDeviceEnergyModel (sem);
  sem->SetNode (node);
  node->AggregateObject (esCont);

  es->SetC(1-0.166);
  es->SetK(0.0169);
  es->SetInitialEnergy(31752);
  Time now = Simulator::Now ();

  sem->SetCurrentA (1);
  now += Seconds (1);
  for (int i = 0; i < 360; i++)
  {
	  Simulator::Schedule (now,
	                       &SimpleDeviceEnergyModel::SetCurrentA,
	                       sem,
	                               0.05);
	  now += Seconds (600);
	  Simulator::Schedule (now,
	                       &SimpleDeviceEnergyModel::SetCurrentA,
	                       sem,
	                               0);
	  now += Seconds (60);
  }


  PrintCellInfo (es);

  Simulator::Stop (now);
  Simulator::Run ();
  Simulator::Destroy ();
}
