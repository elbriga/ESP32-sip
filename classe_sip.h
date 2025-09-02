#include <WiFi.h>
#include <AsyncUDP.h>
#include <Ticker.h>
#include <MD5Builder.h>

#include "classe_rtp.h"

extern const int DEBUG;

class SIPClient {
private:
  String user, pass, server;
  int localPort, serverPort;

  IPAddress serverIP;
  AsyncUDP udp;

  int rtpPort;
  RTP rtp;

  uint32_t cSeq;
  bool registered, emLigacao;
  String localIP, callID;

  Ticker registerTimer;

public:
  SIPClient(const char* user, const char* pass, const char* server, int rtpPort, int serverPort = 5060, int localPort = 5060);
  void begin();
  bool getEmLigacao();
  bool getRegistered();

private:
  void sendRegister(String authLine);
  String md5(String s);
  String buildAuthLine(String nonce);
  void handlePacket(AsyncUDPPacket packet);
};
