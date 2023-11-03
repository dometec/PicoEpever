#include <stdio.h>
#include <WiFi.h>
#include <WifiData.h>
#include <WebServer.h>
#include <LEAmDNS.h>
#include <StreamString.h>
#include <PolledTimeout.h>

#include <Arduino.h>
#include <PubSubClient.h>

#include "Epever.h"
#include "Grafana.h"

#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"
#include "hardware/adc.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

// L'ultima pagina di 4Kb della memoria flash mi basta...
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

// Pin LED di PicoW
#define CYW43_WL_GPIO_LED_PIN 0

// Pin da mettere a massa per far partire il Pico in modalità configurazione
#define FORCED_WIFIAP_PIN 28

WiFiClient wifiClient;
WebServer server(80);
int wifiStatus = WL_IDLE_STATUS;
const char TEXT_PLAIN[] = "text/plain";

Epever epever;
Grafana grafana;

const char* status_saved = "SAVED";
WifiData wifiData;

// Se true devo avviare access point per la configurazione, se false mi connetto
boolean startupsetup = true;

// Conta cicli
int cycle = 0;

// Pagina root 
void handleRoot() {
  StreamString temp;
  temp.reserve(2500); // Preallocate a large chunk to avoid memory fragmentation
  temp.printf("<html>\
                <head>\
                  <title>Data Logger EPEVER con Pico-W</title>\
                </head>\
                <body>\
                  <h1>Data Logger EPEVER su Grafana Cloud con Raspberry Pico W</h1>\
                  <h3>Realizzato da Domenico Briganti - dometec@gmail.com - Settembre 2023</h3>\
                  <h2>Questa &egrave; la pagina di configurazione</h2>\
                  <p>Se qualche parametro &egrave; errato e dai l'OK, puoi ritornare su questa pagina mettendo a massa il PIN n. 24 (GPIO 28) e resettando la scheda.</p>\
                  <form action=\"/\" method=\"post\">\
                    <p><label for=\"wifisid\">WiFi SID:</label><br>\
                      <input type=\"text\" id=\"wifisid\" name=\"wifisid\" value=\"%s\" size=\"50\" maxlength=\"50\"></p>\
                    <p><label for=\"wifipassword\">WiFi Password:</label><br>\
                      <input type=\"text\" id=\"wifipassword\" name=\"wifipassword\" value=\"%s\" size=\"50\" maxlength=\"50\"></p>\
                    <p><label for=\"grafanaapikey\">Grafana Api Key:</label><br>\
                      <input type=\"text\" id=\"grafanaapikey\" name=\"grafanaapikey\" value=\"%s\" size=\"50\" maxlength=\"200\"></p>\
                    <p><label for=\"grafanahost\">Grafana Host:</label><br>\
                      <input type=\"text\" id=\"grafanahost\" name=\"grafanahost\" value=\"%s\" size=\"50\" maxlength=\"200\"></p>\
                    <p><label for=\"grafanapath\">Grafana Path:</label><br>\
                      <input type=\"text\" id=\"grafanapath\" name=\"grafanapath\" value=\"%s\" size=\"50\" maxlength=\"200\"></p>\
                    <p><input type=\"submit\" value=\"Ok\"></input></p>\
                  </form>\
                </body>\
               </html>", wifiData.wifi_sid, wifiData.wifi_pas, wifiData.grafana_apikey, wifiData.grafana_host, wifiData.grafana_path);
  server.send(200, "text/html", temp);
}

void handleNotFound() {
  server.send(404, "text/plain", "");
}

void handleSave() {

  for (uint8_t i = 0; i < server.args(); i++) {

    const char * name = server.argName(i).c_str();

    if (strcmp(server.argName(i).c_str(), "wifisid") == 0) 
      strcpy(wifiData.wifi_sid, server.arg(i).c_str());
    else if (strcmp(server.argName(i).c_str(), "wifipassword") == 0) 
      strcpy(wifiData.wifi_pas, server.arg(i).c_str());
    else if (strcmp(server.argName(i).c_str(), "grafanaapikey") == 0) 
      strcpy(wifiData.grafana_apikey, server.arg(i).c_str());
    else if (strcmp(server.argName(i).c_str(), "grafanahost") == 0) 
      strcpy(wifiData.grafana_host, server.arg(i).c_str());
    else if (strcmp(server.argName(i).c_str(), "grafanapath") == 0) 
      strcpy(wifiData.grafana_path, server.arg(i).c_str());

  }

  strcpy(wifiData.status, status_saved);

  Serial1.println("Dati arrivati...");
  Serial1.println(wifiData.status);
  Serial1.println(wifiData.wifi_sid);
  Serial1.println(wifiData.wifi_pas);
  Serial1.println(wifiData.grafana_apikey);
  Serial1.println(wifiData.grafana_host);
  Serial1.println(wifiData.grafana_path);

  Serial1.println("Programming flash...");
  uint32_t interrupts = save_and_disable_interrupts();
  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(FLASH_TARGET_OFFSET, (uint8_t*) &wifiData, FLASH_PAGE_SIZE * 3);
  restore_interrupts(interrupts);
  Serial1.println("Write done.");

  StreamString temp;
  temp.reserve(1500); // Preallocate a large chunk to avoid memory fragmentation
  temp.printf("<html>\
                <head>\
                  <title>Data Logger EPEVER con Pico-W</title>\
                </head>\
                <body>\
                  <h1>Data Logger EPEVER su Grafana Cloud con Raspberry Pico W</h1>\
                  <h3>Realizzato da Domenico Briganti - dometec@gmail.com - Settembre 2023</h3>\
                  <h2>Configurazione salvata!</h2>\
                  <p>Adesso viene resettato il Raspberry Pico, se il LED lampegger&agrave; &egrave; tutto pronto, altrimenti serve rifare la procedura.</p>\
                  <p>Se hai problemi puoi ed hai un convertitore seriale->USB puoi leggere sull'uscita al PIN 2 i le operazioni che il Pico sta effettuanto</p>\
                  <p>Dopo il reset, verranno letti alcuni valori istantanei dall'Epever ed inviati a Grafana Cloud ogni 10 secondi circa.</p>\
                </body>\
              </html>");
  server.send(200, "text/html", temp);

  Serial1.println("Reboot.");
  rp2040.reboot();

}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial1.print("SSID: ");
  Serial1.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial1.print("IP Address: ");
  Serial1.println(ip);

  // print where to go in a browser:
  Serial1.print("To see this page in action, open a browser to http://");
  Serial1.println(ip);

}

void printMacAddress() {
  // the MAC address of your Wifi shield
  byte mac[6];

  // print your MAC address:
  WiFi.macAddress(mac);
  Serial1.print("MY MAC: ");
  Serial1.print(mac[5], HEX);
  Serial1.print(":");
  Serial1.print(mac[4], HEX);
  Serial1.print(":");
  Serial1.print(mac[3], HEX);
  Serial1.print(":");
  Serial1.print(mac[2], HEX);
  Serial1.print(":");
  Serial1.print(mac[1], HEX);
  Serial1.print(":");
  Serial1.println(mac[0], HEX);
}

void setup() {

  // Initialize serial and wait for port to open:
  Serial1.begin(115200);

  if (watchdog_caused_reboot()) {
    Serial1.println("Rebooted by Watchdog!\n");
  } else {
    Serial1.println("Clean boot\n");
  }
  Serial1.flush();

  pinMode(FORCED_WIFIAP_PIN, INPUT_PULLUP);
  memcpy(&wifiData, (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET), sizeof(wifiData));

  if (strcmp(wifiData.status, status_saved) == 0 && digitalRead(FORCED_WIFIAP_PIN) == HIGH) {

    Serial1.println("Credenziali WIFI salvate, mi connetto...");
    startupsetup = false;
    Serial1.println(wifiData.wifi_sid);
    Serial1.println(wifiData.wifi_pas);

  } else {

    Serial1.println("Credenziali WIFI non salvate, creo access point per la configurazione...");
    startupsetup = true;

    // Pulisco la memoria se non le ho mai salvate in precedenza per 
    // evidare di presentare sulla pagina spazzatura
    if (strcmp(wifiData.status, status_saved) != 0) {
      strcpy(wifiData.wifi_sid, "");
      strcpy(wifiData.wifi_pas, "");
      strcpy(wifiData.grafana_apikey, "");
      strcpy(wifiData.grafana_host, "");
      strcpy(wifiData.grafana_path, "");
    }

  }
  
  if (startupsetup) {

    Serial1.println("Creo l'access point...");
    printMacAddress();

    wifiStatus = WiFi.beginAP("MisuratoreEpever");
    while (wifiStatus != WL_CONNECTED) {
      Serial1.println("Creating access point failed...");
      Serial1.print(wifiStatus);
      delay(500);
    }
  
    if (MDNS.begin("picow"))
      Serial.println("MDNS responder started");

    // start the web server on port 80
    server.on("/", HTTP_GET, handleRoot);
    server.on("/", HTTP_POST, handleSave);
    server.onNotFound(handleNotFound);
    server.begin();

  } else {

    // Enable the watchdog, requiring the watchdog to be updated every 5s or the chip will reboot
    // second arg is pause on debug which means the watchdog will pause when stepping through code
    watchdog_enable(8000, 1);

    // Connect to Wifi.
    Serial1.println();
    Serial1.print("Connecting to ");
    Serial1.println(wifiData.wifi_sid);

    WiFi.begin(wifiData.wifi_sid, wifiData.wifi_pas);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      watchdog_update();
      Serial1.print(".");
    }

    Serial1.println("IP address: ");
    Serial1.println(WiFi.localIP());

  }

  // you're connected now, so print out the status
  printWiFiStatus();

}

void loop(void) {

  if (startupsetup) {

    server.handleClient();
    MDNS.update();

  } else {

    if (WiFi.status() != WL_CONNECTED) {
      
      Serial1.print("Riconnessione...");

    } else {

      // ArduinoOTA.handle();

      if ((cycle % 1000) == 0) {
        EpeverData dto = epever.read();
        grafana.sendData(dto);
      }

      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, (cycle / 10) % 2);

    }

    watchdog_update();

    cycle++;
    delay(10);

  }

}

