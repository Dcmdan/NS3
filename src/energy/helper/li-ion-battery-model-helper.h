#ifndef LI_ION_BATTERY_MODEL_HELPER_H_
#define LI_ION_BATTERY_MODEL_HELPER_H_

#include "energy-model-helper.h"
#include "ns3/node.h"

namespace ns3 {

class LiIonBatteryModelHelper: public EnergySourceHelper
{
public:
	LiIonBatteryModelHelper ();
  ~LiIonBatteryModelHelper ();

  void Set (std::string name, const AttributeValue &v);

private:
  virtual Ptr<EnergySource> DoInstall (Ptr<Node> node) const;

private:
  ObjectFactory m_liIonBatteryModel;

};
}
#endif
