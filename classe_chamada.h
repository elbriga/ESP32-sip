#pragma once

#include "classe_rtp.h"

class Chamada {
private:
  RTPPlayer rtpPlayer;

  String callID;
  bool ativa;

public:
  Chamada();

  void iniciar(String callID, int localRtpPort, String remoteIP, int remoteRtpPort, unsigned char *audio, int audioLength);
  void encerrar();

  bool estaAtiva();
  String getCallID();
};
