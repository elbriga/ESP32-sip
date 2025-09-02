#include <WiFi.h>
#include <AsyncUDP.h>
#include <Ticker.h>
#include <MD5Builder.h>

class SIPClient {
private:
  AsyncUDP udp;
  String user, pass, server, localIP, callID;
  int port;
  uint32_t cseq;
  bool registered;
  Ticker registerTimer;

public:
  SIPClient(const char* user, const char* pass, const char* server, int port = 5060)
    : user(user), pass(pass), server(server), port(port), cseq(1), registered(false) {
    callID = String(random(10000, 99999)) + "@esp32";
  }

  void begin() {
    localIP = WiFi.localIP().toString();
    if (udp.listen(port)) {
      Serial.println("SIP UDP escutando na porta " + String(port));
      udp.onPacket([this](AsyncUDPPacket packet) {
        this->handlePacket(packet);
      });
      sendRegister();
    }
  }

private:
  void sendRegister(const char* authHeader = nullptr) {
    String msg = "REGISTER sip:" + server + " SIP/2.0\r\n";
    msg += "Via: SIP/2.0/UDP " + localIP + ":5060;branch=z9hG4bK-1234\r\n";
    msg += "Max-Forwards: 70\r\n";
    msg += "From: <sip:" + user + "@" + server + ">;tag=1234\r\n";
    msg += "To: <sip:" + user + "@" + server + ">\r\n";
    msg += "Call-ID: " + callID + "\r\n";
    msg += "CSeq: " + String(cseq++) + " REGISTER\r\n";
    msg += "Contact: <sip:" + user + "@" + localIP + ":5060>\r\n";
    msg += "Expires: 60\r\n";
    if (authHeader) msg += String(authHeader);
    msg += "Content-Length: 0\r\n\r\n";

    udp.writeTo((uint8_t*)msg.c_str(), msg.length(),
                IPAddress().fromString(server), port);

    Serial.println("REGISTER enviado:");
    Serial.println(msg);
  }

  String computeDigestAuth(String method, String uri, String realm, String nonce) {
    String ha1, ha2, resp;
    MD5Builder md5;

    md5.begin();
    md5.add(user + ":" + realm + ":" + pass);
    md5.calculate();
    ha1 = md5.toString();

    md5.begin();
    md5.add(method + ":" + uri);
    md5.calculate();
    ha2 = md5.toString();

    md5.begin();
    md5.add(ha1 + ":" + nonce + ":" + ha2);
    md5.calculate();
    resp = md5.toString();

    String hdr = "Authorization: Digest username=\"" + user + "\", realm=\"" + realm +
                 "\", nonce=\"" + nonce + "\", uri=\"" + uri + "\", response=\"" + resp + "\"\r\n";
    return hdr;
  }

  void handlePacket(AsyncUDPPacket packet) {
    String msg = (const char*)packet.data();
    Serial.println("SIP recebido:");
    Serial.println(msg);

    if (msg.startsWith("SIP/2.0 401")) {
      int r1 = msg.indexOf("realm=\"");
      int r2 = msg.indexOf("\"", r1 + 7);
      String realm = msg.substring(r1 + 7, r2);

      int n1 = msg.indexOf("nonce=\"");
      int n2 = msg.indexOf("\"", n1 + 7);
      String nonce = msg.substring(n1 + 7, n2);

      String auth = computeDigestAuth("REGISTER", "sip:" + server, realm, nonce);
      sendRegister(auth.c_str());
    }
    else if (msg.startsWith("SIP/2.0 200 OK")) {
      if (msg.indexOf("REGISTER") > 0) {
        Serial.println("REGISTER OK! Registrado no servidor SIP.");
        registered = true;
        registerTimer.detach();
        registerTimer.attach(50, [this]() { sendRegister(); });
      }
    }
    else if (msg.startsWith("OPTIONS")) {
      Serial.println("Respondendo OPTIONS...");
      // aqui poderia implementar resposta 200 OK
    }
    else if (msg.startsWith("INVITE")) {
      Serial.println("Recebido INVITE (não tratado ainda).");
      // aqui poderia implementar resposta 200 OK com SDP
    }
    else if (msg.startsWith("BYE")) {
      Serial.println("Recebido BYE.");
      // aqui poderia encerrar chamada
    }
  }
};
