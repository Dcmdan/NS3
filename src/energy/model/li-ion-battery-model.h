#ifndef LI_ION_BATTERY_MODEL_H
#define LI_ION_BATTERY_MODEL_H

#include "ns3/traced-value.h"
#include "energy-source.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"

namespace ns3 {

class LiIonBatteryModel : public EnergySource
{
public:
  static TypeId GetTypeId (void);
  LiIonBatteryModel ();
  void  SetC (double);
  void  SetK (double);
  virtual ~LiIonBatteryModel ();
  virtual double GetInitialEnergy (void) const;
  void SetInitialEnergy (double initialEnergyJ);
  virtual double GetSupplyVoltage (void) const;
  void SetInitialSupplyVoltage (double supplyVoltageV);
  virtual double GetRemainingEnergy (void);
  virtual double GetEnergyFraction (void);
  virtual void DecreaseRemainingEnergy (double energyJ);
  virtual void IncreaseRemainingEnergy (double energyJ);
  virtual void UpdateEnergySource (void);
  void SetEnergyUpdateInterval (Time interval);
  Time GetEnergyUpdateInterval (void) const;

private:
  void DoInitialize (void);
  void DoDispose (void);
  void HandleEnergyDrainedEvent (void);
  void CalculateRemainingEnergy (void);
  double GetVoltage (double current) const;
  void init(void);


private:
  double m_initialEnergyJ;                // initial energy, in Joules
  TracedValue<double> m_remainingEnergyJ; // remaining energy, in Joules
  double m_drainedCapacity;               // capacity drained from the cell, in Ah
  double m_supplyVoltageV;                // actual voltage of the cell
  double m_lowBatteryTh;                  // low battery threshold, as a fraction of the initial energy
  EventId m_energyUpdateEvent;            // energy update event
  Time m_lastUpdateTime;                  // last update time
  Time m_energyUpdateInterval;            // energy update interval
  double m_eFull;                         // initial voltage of the cell, in Volts
  double m_eNom;                          // nominal voltage of the cell, in Volts
  double m_eExp;                          // cell voltage at the end of the exponential zone, in Volts
  double m_internalResistance;            // internal resistance of the cell, in Ohms
  double m_qRated;                        // rated capacity of the cell, in Ah
  double m_qNom;                          // cell capacity at the end of the nominal zone, in Ah
  double m_qExp;                          // capacity value at the end of the exponential zone, in Ah
  double m_typCurrent;                    // typical discharge current used to fit the curves
  double m_minVoltTh;                     // minimum threshold voltage to consider the battery depleted

  double m_c;
  double m_k;
  double m_y1; //unit is A*s
  double m_y2;
  double m_y1b;
  double m_y2b;
  double I1, I2;

  bool isInit;
};

}

#endif
