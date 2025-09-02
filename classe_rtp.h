#pragma once
#include <AsyncUDP.h>
#include <Ticker.h>

// forward declaration do teste_audio
extern const unsigned char teste_ulaw[];
extern const size_t teste_ulaw_len;

class RTP {
private:
  int localPort;
  AsyncUDP udp;
  Ticker rtpTimer;

  IPAddress lastIP;
  int lastPort;

  const int chunkSize = 160; // 20ms @ 8000Hz u-law
  size_t pos;

public:
  RTP(int localPort = 4000);
  void begin();

private:
  void handlePacket(AsyncUDPPacket packet);
  void sendRTP();
};
