#include "Epever.h"

class Grafana {

  public:
    Grafana();
    void sendData(WifiData wifiData, EpeverData dto);
  private:
    char msg[1000];

};
