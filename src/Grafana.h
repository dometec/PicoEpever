#include "Epever.h"

class Grafana {

  public:
    Grafana();
    void sendData(EpeverData dto);
  private:
    char msg[1000];

};
