#pragma once

#include <ModbusMaster.h>

struct EpeverData {

  int  overTemp;
  int  dayNight;

  float pvVolt;
  float pvAmpere;
  float pvWatt;

  float loadVolt;
  float loadAmpere;
  float loadWatt;

  float battTemp;
  int   battSOC;
  float battRatedVolt;
  float battVolt;
  float battAmpere;
  
  int   battStatus;
  // D15: 1-Wrong identification for rated voltage
  bool  battWrongIdentification;
  // D8: Battery inner resistance abnormal 1, normal 0
  bool  battInternarResAbnormal;
  // D7-D4: 00H Normal, 01H Over Temp.(Higher than the warning settings), 02H Low Temp.(Lower than the warning settings)
  int   battTempStat;
  // D3-D0: 00H Normal ,01H Over Voltage. , 02H Under Voltage, 03H Over discharge, 04H Fault 
  int   battVoltStat;

  int   chargeEquipStatus;
  // D15-D14: Input voltage status. 00H normal, 01H No input power connected, 02H Higher input voltage , 03H Input voltage error.
  int   chargeEquipStatusInputVoltage;
  // D3-D2: Charging status. 00H No charging, 01H Float, 02H Boost, 03H Equalization.
  int   chargeEquipStatusChargingStatus;
  // D1: 0 Normal, 1 Fault.
  int   chargeEquipStatusFault;
  // D0: 1 Running, 0 Standby.
  int   chargeEquipStatusRunning;

  int   dischargeEquipStatus;

  float eneryGenerated;
  float eneryConsumed;

}; 

class Epever {

  public:
    Epever();
    EpeverData read();
  private:
    uint8_t _readDI(int reg);
    uint16_t _readIR(int reg);
    ModbusMaster epevermb;

};
 