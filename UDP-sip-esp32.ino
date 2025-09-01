#include <WiFi.h>
#include <AsyncUDP.h>
#include <Ticker.h>

#include "teste_audio.h"

const char* ssid = "Ligga Gabriel 2.4g";
const char* pass = "09876543";
const int LED_PIN = 2;

AsyncUDP sipUDP;   // Porta 5060 SIP
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

  // agenda RTP a cada 20ms
  rtpTimer.attach_ms(20, sendRTP);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());

  // --- SIP UDP ---
  if (sipUDP.listen(5060)) {
    Serial.println("SIP UDP escutando na porta 5060");
    sipUDP.onPacket([](AsyncUDPPacket packet) {
      String msg = (const char*)packet.data();
      msg.trim();
      Serial.println("SIP recebido:");
      Serial.println(msg);

      auto extractHeader = [&](const char* name) -> String {
          int idx = msg.indexOf(name);
          if (idx == -1) return "";
          int end = msg.indexOf("\r\n", idx);
          return msg.substring(idx + strlen(name), end);
        };
      
      String via    = extractHeader("Via: ");
      String from   = extractHeader("From: ");
      String to     = extractHeader("To: ");
      String callID = extractHeader("Call-ID: ");
      String cseq   = extractHeader("CSeq: ");

      String response = "";

      if (msg.startsWith("OPTIONS")) {
        response = String("SIP/2.0 200 OK\r\n");
        response += "Via: " + via + "\r\n";
        response += "From: " + from + "\r\n";
        response += "To: " + to + "\r\n";
        response += "Call-ID: " + callID + "\r\n";
        response += "CSeq: " + cseq + "\r\n";
        response += "Content-Length: 0\r\n\r\n";

        Serial.println("Respondido 200 OK ao OPTIONS");
      }

      else if (msg.startsWith("INVITE")) {
        // Aceita qualquer INVITE para teste
        String sdp = 
          "v=0\r\n"
          "o=- 0 0 IN IP4 " + WiFi.localIP().toString() + "\r\n"
          "s=ESP32 SIP Echo\r\n"
          "c=IN IP4 " + WiFi.localIP().toString() + "\r\n"
          "t=0 0\r\n"
          "m=audio 4000 RTP/AVP 0\r\n"
          "a=rtpmap:0 PCMU/8000\r\n";

        response = String("SIP/2.0 200 OK\r\n");
        response += "Via: " + via + "\r\n";
        response += "From: " + from + "\r\n";
        response += "To: " + to + "\r\n";
        response += "Call-ID: " + callID + "\r\n";
        response += "CSeq: " + cseq + "\r\n";
        response += "Contact: <sip:esp32@" + WiFi.localIP().toString() + ":5060>\r\n";
        response += "Content-Type: application/sdp\r\n";
        response += "Content-Length: " + String(sdp.length()) + "\r\n\r\n";
        response += sdp;

        Serial.println("Respondido 200 OK ao INVITE");
      }
      
      else if (msg.startsWith("BYE")) {
        response = String("SIP/2.0 200 OK\r\n");
        response += "Via: " + via + "\r\n";
        response += "From: " + from + "\r\n";
        response += "To: " + to + "\r\n";
        response += "Call-ID: " + callID + "\r\n";
        response += "CSeq: " + cseq + "\r\n";
        response += "Content-Length: 0\r\n\r\n";

        lastPort = 0;
        digitalWrite(LED_PIN, LOW);
        Serial.println("Respondido 200 OK ao BYE, chamada encerrada");
      }

      if (response != "") {
        sipUDP.writeTo((uint8_t*)response.c_str(), response.length(),
                       packet.remoteIP(), packet.remotePort());
      }
    });
  }

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
  }
}

void loop() {
  delay(20);
}
