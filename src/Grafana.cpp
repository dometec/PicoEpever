#include <WiFi.h>
#include <WifiData.h>
#include "Grafana.h"

BearSSL::WiFiClientSecure client;

Grafana::Grafana() {

};

void Grafana::sendData(WifiData wifiData, EpeverData dto) {

  Serial1.println("Prepare data...");

  snprintf(msg, 990, 
  "epever \
overTemp=%d,dayNight=%d,\
pvVolt=%.4f,pvAmpere=%.4f,pvWatt=%.4f,\
loadVolt=%.4f,loadAmpere=%.4f,loadWatt=%.4f,\
battTemp=%.4f,battSOC=%d,battRatedVolt=%.4f,battVolt=%.4f,battAmpere=%.4f,\
battStatus=%d,battWrongIdentification=%d,battInternarResAbnormal=%d,battTempStat=%d,battVoltStat=%d,\
chargeEquipStatus=%d,chargeEquipStatusInputVoltage=%d,chargeEquipStatusChargingStatus=%d,chargeEquipStatusFault=%d,chargeEquipStatusRunning=%d,\
dischargeEquipStatus=%d", 

    dto.overTemp, dto.dayNight, \
    dto.pvVolt, dto.pvAmpere, dto.pvWatt, \
    dto.loadVolt, dto.loadAmpere, dto.loadWatt, \
    dto.battTemp, dto.battSOC , dto.battRatedVolt, dto.battVolt, dto.battAmpere, \
    dto.battStatus, dto.battWrongIdentification, dto.battInternarResAbnormal, dto.battTempStat, dto.battVoltStat, \
    dto.chargeEquipStatus, dto.chargeEquipStatusInputVoltage, dto.chargeEquipStatusChargingStatus, dto.chargeEquipStatusFault, dto.chargeEquipStatusRunning, \
    dto.dischargeEquipStatus 
    
  );

  Serial1.println(msg);


  Serial1.println("Send data...");

  client.setInsecure();
  client.connect(wifiData.grafana_host, 443);

  if (client.connected()) {
  
    client.print("POST ");
    client.print(wifiData.grafana_path);
    client.println(" HTTP/1.1");

    client.print("Host: ");
    client.println(wifiData.grafana_host);

    client.print("Authorization: Bearer ");
    client.println(wifiData.grafana_apikey);

    client.println("Content-Type: text/plain");

    client.println("User-Agent: Raspberry Pi Pico W");

    client.print("Content-Length: ");
    client.println(strlen(msg));
    
    client.println();

    // Body della richiesta
    client.println(msg);
    client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      Serial1.println(line);
      if (line == "\r")
        break; // Fine lettura header
    }

    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
      char c = client.read();
      Serial1.write(c);
    }

    client.stop();
    Serial1.printf("\n-------\n");

  } else {
    Serial1.print("Errore durante la connessione a ");
    Serial1.println(wifiData.grafana_host);
  }

}
     