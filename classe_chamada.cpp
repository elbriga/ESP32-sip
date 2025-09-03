#include <Arduino.h> // para Serial

#include "classe_chamada.h"
#include "classe_rtp.h"

Chamada::Chamada() : ativa(false) {}

void Chamada::iniciar(String callID, int localRtpPort, String remoteIP, int remoteRtpPort, unsigned char *audio, int audioLength) {
  this->callID = callID;
  this->ativa = true;

  rtpPlayer.start(localRtpPort, remoteIP, remoteRtpPort, audio, audioLength);

  Serial.println("Chamada iniciada na porta RTP " + String(localRtpPort) + " enviando para " + remoteIP + ":" + String(remoteRtpPort));
}

void Chamada::encerrar() {
  ativa = false;
  rtpPlayer.stop();
}

bool Chamada::estaAtiva() {
  return ativa;
}

String Chamada::getCallID() {
  return callID;
}
