#pragma once
#include <AsyncUDP.h>
#include <Ticker.h>

class RTPPlayer {
private:
  int localPort;
  AsyncUDP udp;
  Ticker rtpTimer;

  IPAddress remoteIP;
  int remotePort;

  unsigned char *audio;
  int audioLength;

  const int chunkSize = 160; // 20ms @ 8000Hz u-law
  size_t pos;

public:
  RTPPlayer();
  void start(int localRtpPort, String remoteIP, int remoteRtpPort, unsigned char *audio, int audioLength);
  void stop();

private:
  void handlePacket(AsyncUDPPacket packet);
  void sendRTP();
};
