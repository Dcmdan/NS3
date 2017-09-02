#include "li-ion-battery-model-helper.h"
#include "ns3/energy-source.h"

namespace ns3 {

LiIonBatteryModelHelper::LiIonBatteryModelHelper ()
{
	m_liIonBatteryModel.SetTypeId ("ns3::LiIonEnergySource");
}

LiIonBatteryModelHelper::~LiIonBatteryModelHelper ()
{
}

void 
LiIonBatteryModelHelper::Set (std::string name, const AttributeValue &v)
{
	m_liIonBatteryModel.Set (name, v);
}

Ptr<EnergySource> 
LiIonBatteryModelHelper::DoInstall (Ptr<Node> node) const
{
  NS_ASSERT (node != NULL);
  Ptr<EnergySource> source = m_liIonBatteryModel.Create<EnergySource> ();
  NS_ASSERT (source != NULL);
  source->SetNode (node);
  return source;
}

} // namespace ns3
