#include <stdexcept>
#include "Epever.h"

ModbusMaster epevermb;

Epever::Epever() {
};

uint8_t Epever::_readDI(int reg) {
  uint8_t result = epevermb.readDiscreteInputs(reg, 1);
  if (result != epevermb.ku8MBSuccess) {
    Serial1.print("Errore lettura indirizzo ");
    Serial1.print(reg);
    Serial1.println(" riavvio applicazione...");
    throw std::runtime_error("Errore lettura epever");
  }
  return epevermb.getResponseBuffer(0);
}

uint16_t Epever::_readIR(int reg) {
  uint8_t result = epevermb.readInputRegisters(reg, 1);
  if (result != epevermb.ku8MBSuccess) {
    Serial1.print("Errore lettura indirizzo ");
    Serial1.print(reg);
    Serial1.println(" riavvio applicazione...");
    throw std::runtime_error("Errore lettura epever");
  }
  return epevermb.getResponseBuffer(0);
}

EpeverData Epever::read() {

  Serial1.print("Leggo Epever...");
  watchdog_update();

  Serial2.begin(115200);
  epevermb.begin(1, Serial2);

  EpeverData dto;

  dto.overTemp =  _readDI(0x2000);
  dto.dayNight =  _readDI(0x200C);

  // Pannelli
  dto.pvVolt = _readIR(0x3100) / 100.0;
  dto.pvAmpere = _readIR(0x3101) / 100.0;
  int32_t p = _readIR(0x3103) << 16;
  dto.pvWatt =  (p + _readIR(0x3102)) / 100.0;

  // Carico
  dto.loadVolt =  _readIR(0x310C) / 100.0;
  dto.loadAmpere =  _readIR(0x310D) / 100.0;
  p = _readIR(0x310F) << 16;
  dto.loadWatt =  (p + _readIR(0x310E)) / 100.0;

  // Batteria  
  dto.battTemp =  _readIR(0x3110) / 100.0;
  dto.battSOC =  _readIR(0x311A);
  dto.battRatedVolt = _readIR(0x311D) / 100.0;
  dto.battVolt =  _readIR(0x331A) / 100.0;

  p = _readIR(0x331C) << 16;
  dto.battAmpere = (p + _readIR(0x331B)) / 100.0;

  // Stati
  dto.battStatus = _readIR(0x3200);
  dto.battWrongIdentification = (dto.battStatus & 0b1000000000000000) > 0;
  dto.battInternarResAbnormal = (dto.battStatus & 0b0000000010000000) > 0;
  dto.battTempStat = (dto.battStatus & 0b0000000001111000) >> 3;
  dto.battVoltStat = (dto.battStatus & 0b0000000000000111);

  dto.chargeEquipStatus = _readIR(0x3201);
  dto.chargeEquipStatusInputVoltage = (dto.chargeEquipStatus & 0b1100000000000000) >> 14;
  dto.chargeEquipStatusChargingStatus = (dto.chargeEquipStatus & 0b0000000000001100) >> 2;
  dto.chargeEquipStatusFault = (dto.chargeEquipStatus & 0b0000000000000010) > 0;
  dto.chargeEquipStatusRunning = (dto.chargeEquipStatus & 0b0000000000000001) > 0;

  dto.dischargeEquipStatus = _readIR(0x3202);

  Serial1.print("Letto da Epever: v: ");
  Serial1.print(dto.pvVolt);
  Serial1.print("a: ");
  Serial1.print(dto.pvAmpere);
  Serial1.print("w: ");
  Serial1.print(dto.pvWatt);
  Serial1.println("...etc...");
  return dto;

}
