#include "li-ion-battery-model.h"

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"

#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LiIonBatteryModel");

NS_OBJECT_ENSURE_REGISTERED (LiIonBatteryModel);

TypeId
LiIonBatteryModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LiIonBatteryModel")
    .SetParent<EnergySource> ()
    .SetGroupName ("Energy")
    .AddConstructor<LiIonBatteryModel> ()
    .AddAttribute ("LiIonBatteryModelInitialEnergyJ",
                   "Initial energy stored in basic energy source.",
                   DoubleValue (31752.0),  // in Joules
                   MakeDoubleAccessor (&LiIonBatteryModel::SetInitialEnergy,
                                       &LiIonBatteryModel::GetInitialEnergy),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("LiIonEnergyLowBatteryThreshold",
                   "Low battery threshold for LiIon energy source.",
                   DoubleValue (0.10), // as a fraction of the initial energy
                   MakeDoubleAccessor (&LiIonBatteryModel::m_lowBatteryTh),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ConstantC",
                   "ConstantC",
                   DoubleValue (1 - 0.166),
                   MakeDoubleAccessor (&LiIonBatteryModel::SetC),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ConstantK",
                   "ConstantK",
                   DoubleValue (0.0169),  // in Volts
                   MakeDoubleAccessor (&LiIonBatteryModel::SetK),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("InitialCellVoltage",
                   "Initial (maximum) voltage of the cell (fully charged).",
                   DoubleValue (4.05), // in Volts
                   MakeDoubleAccessor (&LiIonBatteryModel::SetInitialSupplyVoltage,
                                       &LiIonBatteryModel::GetSupplyVoltage),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NominalCellVoltage",
                   "Nominal voltage of the cell.",
                   DoubleValue (3.6),  // in Volts
                   MakeDoubleAccessor (&LiIonBatteryModel::m_eNom),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ExpCellVoltage",
                   "Cell voltage at the end of the exponential zone.",
                   DoubleValue (3.6),  // in Volts
                   MakeDoubleAccessor (&LiIonBatteryModel::m_eExp),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RatedCapacity",
                   "Rated capacity of the cell.",
                   DoubleValue (2.45),   // in Ah
                   MakeDoubleAccessor (&LiIonBatteryModel::m_qRated),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NomCapacity",
                   "Cell capacity at the end of the nominal zone.",
                   DoubleValue (1.1),  // in Ah
                   MakeDoubleAccessor (&LiIonBatteryModel::m_qNom),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ExpCapacity",
                   "Cell Capacity at the end of the exponential zone.",
                   DoubleValue (1.2),  // in Ah
                   MakeDoubleAccessor (&LiIonBatteryModel::m_qExp),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("InternalResistance",
                   "Internal resistance of the cell",
                   DoubleValue (0.083),  // in Ohms
                   MakeDoubleAccessor (&LiIonBatteryModel::m_internalResistance),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TypCurrent",
                   "Typical discharge current used to fit the curves",
                   DoubleValue (2.33), // in A
                   MakeDoubleAccessor (&LiIonBatteryModel::m_typCurrent),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ThresholdVoltage",
                   "Minimum threshold voltage to consider the battery depleted.",
                   DoubleValue (3.3), // in Volts
                   MakeDoubleAccessor (&LiIonBatteryModel::m_minVoltTh),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("PeriodicEnergyUpdateInterval",
                   "Time between two consecutive periodic energy updates.",
                   TimeValue (Seconds (0.1)),
                   MakeTimeAccessor (&LiIonBatteryModel::SetEnergyUpdateInterval,
                                     &LiIonBatteryModel::GetEnergyUpdateInterval),
                   MakeTimeChecker ())
    .AddTraceSource ("RemainingEnergy",
                     "Remaining energy at BasicEnergySource.",
                     MakeTraceSourceAccessor (&LiIonBatteryModel::m_remainingEnergyJ),
                     "ns3::TracedValueCallback::Double")
  ;
  return tid;
}

LiIonBatteryModel::LiIonBatteryModel ()
  : m_drainedCapacity (0.0),
    m_lastUpdateTime (Seconds (0.0))
{
  NS_LOG_FUNCTION (this);
  SetInitialEnergy(m_initialEnergyJ);
}

LiIonBatteryModel::~LiIonBatteryModel ()
{
  NS_LOG_FUNCTION (this);
}

void
LiIonBatteryModel::SetC (double c)
{
  m_c = c;
  SetInitialEnergy(m_initialEnergyJ);
}

void
LiIonBatteryModel::SetK (double k)
{
  m_k = k;
  SetInitialEnergy(m_initialEnergyJ);
}

void
LiIonBatteryModel::SetInitialEnergy (double initialEnergyJ)
{
  NS_LOG_FUNCTION (this << initialEnergyJ);
  NS_ASSERT (initialEnergyJ >= 0);
  m_initialEnergyJ = m_c * initialEnergyJ;
  m_remainingEnergyJ = m_initialEnergyJ;
  m_y1b = m_remainingEnergyJ / m_supplyVoltageV;
  m_y2b = (1 - m_c) * m_y1b / m_c ;
  I1 = CalculateTotalCurrent();
  I2 = CalculateTotalCurrent();
}

double
LiIonBatteryModel::GetInitialEnergy (void) const
{
  NS_LOG_FUNCTION (this);
  return m_initialEnergyJ;
}

void
LiIonBatteryModel::SetInitialSupplyVoltage (double supplyVoltageV)
{
  NS_LOG_FUNCTION (this << supplyVoltageV);
  m_eFull = supplyVoltageV;
  m_supplyVoltageV = supplyVoltageV;
}

double
LiIonBatteryModel::GetSupplyVoltage (void) const
{
  NS_LOG_FUNCTION (this);
  return m_supplyVoltageV;
}

void
LiIonBatteryModel::SetEnergyUpdateInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_energyUpdateInterval = interval;
}

Time
LiIonBatteryModel::GetEnergyUpdateInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_energyUpdateInterval;
}

double
LiIonBatteryModel::GetRemainingEnergy (void)
{
  NS_LOG_FUNCTION (this);
  // update energy source to get the latest remaining energy.
  UpdateEnergySource ();
  return m_remainingEnergyJ;
}

double
LiIonBatteryModel::GetEnergyFraction (void)
{
  NS_LOG_FUNCTION (this);
  // update energy source to get the latest remaining energy.
  UpdateEnergySource ();
  return m_remainingEnergyJ / m_initialEnergyJ;
}

void
LiIonBatteryModel::DecreaseRemainingEnergy (double energyJ)
{
  NS_LOG_FUNCTION (this << energyJ);
  NS_ASSERT (energyJ >= 0);
  m_remainingEnergyJ -= energyJ;

  // check if remaining energy is 0
  if (m_supplyVoltageV <= m_minVoltTh)
    {
      HandleEnergyDrainedEvent ();
    }
}

void
LiIonBatteryModel::IncreaseRemainingEnergy (double energyJ)
{
  NS_LOG_FUNCTION (this << energyJ);
  NS_ASSERT (energyJ >= 0);
  m_remainingEnergyJ += energyJ;
}

void
LiIonBatteryModel::UpdateEnergySource (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LiIonBatteryModel:Updating remaining energy at node #" <<
                GetNode ()->GetId ());

  // do not update if simulation has finished
  if (Simulator::IsFinished ())
    {
      return;
    }

  m_energyUpdateEvent.Cancel ();

  CalculateRemainingEnergy ();

  m_lastUpdateTime = Simulator::Now ();

  if (m_remainingEnergyJ <= m_lowBatteryTh * m_initialEnergyJ)
    {
      HandleEnergyDrainedEvent ();
      return; // stop periodic update
    }

  m_energyUpdateEvent = Simulator::Schedule (m_energyUpdateInterval,
                                             &LiIonBatteryModel::UpdateEnergySource,
                                             this);
}

/*
 * Private functions start here.
 */
void
LiIonBatteryModel::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  UpdateEnergySource ();  // start periodic update
}

void
LiIonBatteryModel::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  BreakDeviceEnergyModelRefCycle ();  // break reference cycle
}


void
LiIonBatteryModel::HandleEnergyDrainedEvent (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LiIonBatteryModel:Energy depleted at node #" <<
                GetNode ()->GetId ());
  NotifyEnergyDrained (); // notify DeviceEnergyModel objects
}


void
LiIonBatteryModel::CalculateRemainingEnergy (void)
{
  NS_LOG_FUNCTION (this);
  I2 = CalculateTotalCurrent ();

  Time duration = Simulator::Now () - m_lastUpdateTime;
  double step = duration.GetSeconds ();
  NS_ASSERT (step >= 0);
  // energy = current * voltage * time
  double f1 = -I1 + m_k * (m_y2b / (1 - m_c) - m_y1b / m_c);
  double g1 = -m_k * (m_y2b / (1 - m_c) - m_y1b / m_c);
  double f2 = -(I1 + I2) / 2 + m_k * ((m_y2b + step / 2 * f1) / (1 - m_c) - (m_y1b + g1 / 2) / m_c);
  double g2 = -m_k * ((m_y2b + step / 2 * f1) / (1 - m_c) - (m_y1b + step / 2 * g1) / m_c);
  double f3 = -(I1 + I2) / 2 + m_k * ((m_y2b + step / 2 * f2) / (1 - m_c) - (m_y1b + step / 2 * g2) / m_c);
  double g3 = -m_k * ((m_y2b + step / 2 * f2) / (1 - m_c) - (m_y1b + step / 2 * g2) / m_c);
  double f4 = -(I2) / 2 + m_k * ((m_y2b + step * f3) / (1 - m_c) - (m_y1b + step * g3) / m_c);
  double g4 = -m_k * ((m_y2b + step * f3) / (1 - m_c) - (m_y1b + step * g3) / m_c);
  m_y1 = m_y1b + (f1 + 2 * f2 + 2 * f3 + f4) * step / 6;
  m_y2 = m_y2b + (g1 + 2 * g2 + 2 * g3 + g4) * step / 6;
  double energyToDecreaseJ = (m_y1b - m_y1) * m_supplyVoltageV;

   if (m_remainingEnergyJ < energyToDecreaseJ)
    {
      m_remainingEnergyJ = 0; // energy never goes below 0
      m_y1 = 0;
    } 
  else 
    {
      m_remainingEnergyJ -= energyToDecreaseJ;
      m_drainedCapacity += ((m_y1b - m_y1) / 3600) / m_c;
    }  
  // update the supply voltage
  m_supplyVoltageV = GetVoltage (I2);
  I1 = I2;
  m_y1b = m_y1;
  m_y2b = m_y2;
  NS_LOG_DEBUG ("LiIonBatteryModel:Remaining energy = " << m_remainingEnergyJ);
  NS_LOG_DEBUG ("y1= " << m_y1);
}

double
LiIonBatteryModel::GetVoltage (double i) const
{
  NS_LOG_FUNCTION (this << i);

  // integral of i in dt, drained capacity in Ah
  double it = m_drainedCapacity;

  // empirical factors
  double A = m_eFull - m_eExp;
  double B = 3 / m_qExp;

  // slope of the polarization curve
  double K = std::abs ( (m_eFull - m_eNom + A * (std::exp (-B * m_qNom) - 1)) * (m_qRated - m_qNom) / m_qNom);

  // constant voltage
  double E0 = m_eFull + K + m_internalResistance * m_typCurrent - A;

  double E = E0 - K * m_qRated / (m_qRated - it) + A * std::exp (-B * it);

  // cell voltage
  double V = E - m_internalResistance * i;

  NS_LOG_DEBUG ("Voltage: " << V << " with E: " << E);

  return V;
}


} // namespace ns3
