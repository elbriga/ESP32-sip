#include <WiFi.h>
#include <AsyncUDP.h>
#include <Ticker.h>
#include <MD5Builder.h>

#include "classe_chamada.h"

extern const int DEBUG;

class SIPClient {
private:
  String user, pass, server;
  int serverPort;
  int localPort, baseRtpPort;

  String localIP;
  IPAddress serverIP;
  AsyncUDP udp;

  String callIDRegister;
  uint32_t cSeq;
  bool registered, emLigacao;

  Ticker registerTimer;

  static const int MAX_CHAMADAS = 10;
  Chamada chamadas[MAX_CHAMADAS];

public:
  SIPClient(const char* user, const char* pass, const char* server, int serverPort = 5060, int localPort = 5060, int baseRtpPort = 4000);
  void begin();
  int receberChamada(const String& callID, String remoteIP, int remoteRtpPort, unsigned char *audio, int audioLength);
  int encerrarChamada(const String& callID);
  bool getEmLigacao();
  bool getRegistered();

private:
  void receberChamada();
  void sendRegister(String authLine);
  String md5(String s);
  String buildAuthLine(String nonce);
  void handlePacket(AsyncUDPPacket packet);
};
