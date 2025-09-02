#include <WiFi.h>
#include <AsyncUDP.h>
#include <Ticker.h>
#include <MD5Builder.h>

#include "teste_audio.h"
#include "classe_sip.cpp"

const char* ssid = "Ligga Gabriel 2.4g";
const char* pass = "09876543";

SIPClient sip("esp32micro", "aaa", "192.168.18.10");

const int LED_PIN = 2;
const int DEBUG = 0;

AsyncUDP rtpUDP;   // Porta 4000 RTP/áudio

IPAddress lastIP;
int       lastPort = 0;
size_t    pos = 0;

Ticker rtpTimer;

void sendRTP() {
  if (!lastPort) return;

  const int chunkSize = 160; // 20ms @ 8000Hz u-law
  if (pos + chunkSize > teste_ulaw_len) {
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

  memcpy(packet + 12, teste_ulaw + pos, chunkSize);

  rtpUDP.writeTo(packet, sizeof(packet), lastIP, lastPort);

  pos += chunkSize;
  seq++;
  timestamp += chunkSize; // 160 samples = 20ms @ 8kHz
}





void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());

  sip.begin();

  // --- RTP UDP ---
  if (rtpUDP.listen(4000)) {
    Serial.println("RTP UDP escutando na porta 4000");
    rtpUDP.onPacket([&](AsyncUDPPacket packet) {
      if (!lastPort) {
        lastIP   = packet.remoteIP();
        lastPort = packet.remotePort();
        digitalWrite(LED_PIN, HIGH);
      }
      // Echo: envia de volta para o mesmo endereço e porta
      //rtpUDP.writeTo(packet.data(), packet.length(),
      //               packet.remoteIP(), packet.remotePort());
      //Serial.println(String("RTP echo enviado na porta: ")+packet.remotePort()+", tamanho: " + String(packet.length()));
    });

    // agenda RTP a cada 20ms
    rtpTimer.attach_ms(20, sendRTP);
  }
}

void loop() {
  delay(20);
  digitalWrite(LED_PIN, sip.getEmLigacao());
}
