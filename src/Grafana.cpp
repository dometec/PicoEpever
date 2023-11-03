#include <WiFi.h>
#include "Grafana.h"

#define GRAFANA_API_KEY "1034137:glc_eyJvIjoiODc2MTY4IiwibiI6InBpY28tZXBldmVyIiwiayI6ImVkcUl0NE41OXREM3lsazQzdDNvQzI3OCIsIm0iOnsiciI6InByb2QtZXUtd2VzdC0yIn19"
#define GRAFANA_HOST "influx-prod-24-prod-eu-west-2.grafana.net"
#define GRAFANA_PATH "/api/v1/push/influx/write"

BearSSL::WiFiClientSecure client;

Grafana::Grafana() {

};

void Grafana::sendData(EpeverData dto) {

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
  client.connect(GRAFANA_HOST, 443);

  if (client.connected()) {
  
    client.print("POST ");
    client.print(GRAFANA_PATH);
    client.println(" HTTP/1.1");

    client.print("Host: ");
    client.println(GRAFANA_HOST);

    client.print("Authorization: Bearer ");
    client.println(GRAFANA_API_KEY);

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

  }

}
     