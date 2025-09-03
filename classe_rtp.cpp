#include <Arduino.h>   // para Serial
#include <AsyncUDP.h>
#include <Ticker.h>

#include "classe_rtp.h"

RTPPlayer::RTPPlayer() : remotePort(0), pos(0) {}

void RTPPlayer::start(int localRtpPort, String remoteIP, int remoteRtpPort, unsigned char *audio, int audioLength) {
  this->localPort = localRtpPort;
  this->remoteIP.fromString(remoteIP);
  this->remotePort = remoteRtpPort;
  this->audio = audio;
  this->audioLength = audioLength;

  if (udp.listen(localPort)) {
    Serial.println("RTP UDP escutando na porta " + String(localPort));

    udp.onPacket([this](AsyncUDPPacket packet) {
      this->handlePacket(packet);
    });

    rtpTimer.attach_ms(20, [this]() {
      this->sendRTP();
    });
  }
}

void RTPPlayer::stop() {
  rtpTimer.detach();

  remotePort = 0;
  pos = 0;
}

void RTPPlayer::handlePacket(AsyncUDPPacket packet) {
  if (false) {
    remoteIP   = packet.remoteIP();
    remotePort = packet.remotePort();
  }
}

void RTPPlayer::sendRTP() {
  if (!remotePort) return;
  
  if (pos + chunkSize > audioLength) {
    pos = 0;
  }

  // Monta o pacote RTP (com cabeçalho + payload)
  uint8_t packet[12 + chunkSize];
  static uint16_t seq = 0;
  static uint32_t timestamp = 0;

  // Cabeçalho RTP (12 bytes)
  packet[0] = 0x80;            // V=2, P=0, X=0, CC=0
  packet[1] = 0x00;            // PT=0 (PCMU), M=0
  packet[2] = seq >> 8;
  packet[3] = seq & 0xFF;
  packet[4] = (timestamp >> 24) & 0xFF;
  packet[5] = (timestamp >> 16) & 0xFF;
  packet[6] = (timestamp >> 8) & 0xFF;
  packet[7] = (timestamp) & 0xFF;
  packet[8] = 0x12;            // SSRC (fixo só para teste)
  packet[9] = 0x34;
  packet[10] = 0x56;
  packet[11] = 0x78;

  memcpy(packet + 12, audio + pos, chunkSize);

  Serial.println(String("RTP para ") + remoteIP + ":" + String(remotePort));
  udp.writeTo(packet, sizeof(packet), remoteIP, remotePort);

  pos += chunkSize;
  seq++;
  timestamp += chunkSize; // 160 samples = 20ms @ 8kHz
}
