#include <WiFi.h>
#include <AsyncUDP.h>
#include <Ticker.h>
#include <MD5Builder.h>

#include "classe_sip.h"

SIPClient::SIPClient(const char* user, const char* pass, const char* server, int serverPort, int localPort)
  : user(user), pass(pass), server(server), serverPort(serverPort), localPort(localPort), cSeq(0), registered(false) {
  serverIP.fromString(this->server);
  callID = String(random(10000, 99999)) + "@esp32";
  emLigacao = false;
}

void SIPClient::begin() {
  localIP = WiFi.localIP().toString();

  if (udp.listen(localPort)) {
    Serial.println("SIP UDP escutando na porta " + String(localPort));

    udp.onPacket([this](AsyncUDPPacket packet) {
      this->handlePacket(packet);
    });

    sendRegister("");

    // 🔥 aqui o lambda resolve o problema
    registerTimer.attach_ms(30000, [this]() {
      this->sendRegister("");
    });
  }
}

bool SIPClient::getEmLigacao() {
  return emLigacao;
}
bool SIPClient::getRegistered() {
  return registered;
}

void SIPClient::sendRegister(String authLine) {
  String branch = "z9hG4bK-" + String(micros(), HEX);

  cSeq++;

  String msg =
    "REGISTER sip:" + server + " SIP/2.0\r\n"
    "Via: SIP/2.0/UDP " + localIP + ":" + String(localPort) + ";branch=" +branch+ "\r\n"
    "From: <sip:" + user + "@" + server + ">;tag=1234\r\n"
    "To: <sip:" + user + "@" + server + ">\r\n"
    "Call-ID: " + callID + "\r\n"
    "CSeq: " + String(cSeq) + " REGISTER\r\n"
    "Contact: <sip:" + user + "@" + localIP + ":" + String(localPort) + ">\r\n"
    "Max-Forwards: 10\r\n"
    "Expires: 60\r\n"
    "User-Agent: ESP8266-SIP\r\n";

  if (authLine != "")
    msg += authLine;

  msg += "Content-Length: 0\r\n\r\n";

  udp.writeTo((uint8_t*)msg.c_str(), msg.length(), serverIP, serverPort);

  Serial.println("REGISTER enviado: " + authLine);
  if (DEBUG) Serial.println(msg);
}

// Pequeno helper para MD5
String SIPClient::md5(String s) {
  MD5Builder md5;

  md5.begin();
  md5.add(s);
  md5.calculate();

  return md5.toString();
}

// Construir resposta digest Authorization
String SIPClient::buildAuthLine(String nonce) {
  // Digest: HA1 = MD5(username:realm:password)
  //         HA2 = MD5(method:uri)
  //         resp = MD5(HA1:nonce:HA2)

  String realm = "asterisk";
  String uri = "sip:" + server;
  String ha1 = md5(user + ":" + realm + ":" + pass);
  String ha2 = md5("REGISTER:" + uri);
  String response = md5(ha1 + ":" + nonce + ":" + ha2);

  String auth =
    "Authorization: Digest username=\"" + user +
    "\", realm=\"" + realm + "\", nonce=\"" + nonce + "\", uri=\"" + uri +
    "\", response=\"" + response + "\", algorithm=MD5\r\n";
  
  return auth;
}

void SIPClient::handlePacket(AsyncUDPPacket packet) {
  String msg = (const char*)packet.data();
  msg.trim();

  Serial.println("SIP recebido:");
  if (DEBUG) Serial.println(msg);
  else {
    int idx = msg.indexOf("\r\n");
    Serial.println(" > " + msg.substring(0, idx));
  }

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

  if (msg.startsWith("SIP/2.0 401")) {
    // extrai nonce
    int n1 = msg.indexOf("nonce=\"");
    if (n1 != -1) {
      int n2 = msg.indexOf("\"", n1+7);
      String nonce = msg.substring(n1+7, n2);
      Serial.println("Desafio recebido, nonce=" + nonce);

      sendRegister(buildAuthLine(nonce));
    }
  }

  else if (msg.startsWith("SIP/2.0 200 OK") && msg.indexOf("CSeq: " + String(cSeq) + " REGISTER") != -1) {
    registered = true;
    Serial.println("Registrado com sucesso!");
  }

  else if (msg.startsWith("OPTIONS")) {
    response = String("SIP/2.0 200 OK\r\n");
    response += "Via: " + via + "\r\n";
    response += "From: " + from + "\r\n";
    response += "To: " + to + "\r\n";
    response += "Call-ID: " + callID + "\r\n";
    response += "CSeq: " + cseq + "\r\n";
    response += "Allow: INVITE, OPTIONS, BYE\r\n";
    response += "Content-Length: 0\r\n\r\n";

    Serial.println("Respondido 200 OK ao OPTIONS");
  }

  else if (msg.startsWith("INVITE")) {
    // Aceita qualquer INVITE para teste
    String sdp = 
      "v=0\r\n"
      "o=- 0 0 IN IP4 " + localIP + "\r\n"
      "s=ESP32 SIP\r\n"
      "c=IN IP4 " + localIP + "\r\n"
      "t=0 0\r\n"
      "m=audio 4000 RTP/AVP 0\r\n"
      "a=rtpmap:0 PCMU/8000\r\n";

    response = String("SIP/2.0 200 OK\r\n");
    response += "Via: " + via + "\r\n";
    response += "From: " + from + "\r\n";
    response += "To: " + to + "\r\n";
    response += "Call-ID: " + callID + "\r\n";
    response += "CSeq: " + cseq + "\r\n";
    response += "Contact: <sip:esp32@" + localIP + ":5060>\r\n";
    response += "Content-Type: application/sdp\r\n";
    response += "Content-Length: " + String(sdp.length()) + "\r\n\r\n";
    response += sdp;

    emLigacao = true;
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

    emLigacao = false;
    Serial.println("Respondido 200 OK ao BYE, chamada encerrada");
  }

  yield();

  if (response != "") {
    udp.writeTo((uint8_t*)response.c_str(), response.length(), serverIP, serverPort);
  }
}
