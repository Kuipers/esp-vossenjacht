#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <Ticker.h>

// Definieer de maximale grootte van de JSON-payload
const size_t maxJsonSize = 1024;
bool isHidden = false;
Ticker timer1; // Declare the Ticker object

// JSON-object voor de vraag, het antwoord en de code
DynamicJsonDocument jsonData(maxJsonSize);

ESP8266WebServer server(80);

String enteredAnswer = "";
String combination = "";

IPAddress apIP(1, 2, 3, 4);
IPAddress netMsk(255, 255, 255, 0);

// DNS server 
const byte DNS_PORT = 53; 
DNSServer dnsServer;

// check if this string is an IP address
boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

// checks if the request is for the controllers IP, if not we redirect automatically to the
// captive portal 
boolean captivePortal() {
  if (!isIp(server.hostHeader())) {
    Serial.println("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send(302, "text/plain", "");   
    server.client().stop(); 
    return true;
  }
  return false;
}

void handleRoot() {
  if (captivePortal()) { 
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  server.send(200, "text/html", getQuestionPage());
}

void handleNotFound() {
  if (captivePortal()) { 
    return;
  }
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");

  for (uint8_t i = 0; i < server.args(); i++) {
    message += String(F(" ")) + server.argName(i) + F(": ") + server.arg(i) + F("\n");
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", message);
}

void setup() {
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);

  // Mount het SPIFFS-bestandssysteem
  if (!SPIFFS.begin()) {
    return;
  }

  // Laad de JSON-gegevens bij setup
  loadJsonFromFile("/data.json", jsonData);

  WiFi.softAPConfig(apIP, apIP, netMsk);
  // its an open WLAN access point without a password parameter
  const char* ssid = jsonData["ssid"];
  if (ssid) {
    WiFi.softAP(ssid);
  } else {
    WiFi.softAP("nieuwe vos");
  }
  delay(1000);

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  /* Setup the web server */
  server.on("/", handleRoot);
  server.on("/generate_204", handleRoot);
  server.on("/check", HTTP_POST, handleCheck);
  server.onNotFound(handleNotFound);
  // Endpoint om de JSON-gegevens voor de vraag, het antwoord en de code te ontvangen en bij te werken
  server.on("/update", HTTP_POST, handleUpdateJson);
  server.on("/disconnect", handleDisconnect);
  server.begin(); // Web server start

  int interval = jsonData["interval"];
  if (interval > 0) {
    timer1.attach_scheduled(30, toggleSSIDVisibility);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();
}

// Function to toggle SSID visibility
void toggleSSIDVisibility() {
  const char* ssid = jsonData["ssid"];
  if (isHidden) {
    // If the SSID is currently hidden, show it
    WiFi.softAP(ssid, NULL, 1, 0, 4, 100);
    isHidden = false;
    bool led = jsonData["led"];
    if (led) {
      digitalWrite(LED_BUILTIN, LOW);
    }
  } else {
    // If the SSID is currently visible, hide it
    WiFi.softAP(ssid, NULL, 1, 1, 4, 100);
    isHidden = true;
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void handleCheck() {
  if (server.hasArg("answer")) {
    enteredAnswer = server.arg("answer");
    // Converteer het ingevoerde antwoord naar lowercase en trim de spaties
    enteredAnswer.toLowerCase();
    enteredAnswer.trim();

    String correctAnswerStr = jsonData["answer"];
    bool accept_wrong = jsonData["accept_wrong"];
    correctAnswerStr.toLowerCase();
    correctAnswerStr.trim();

    if (enteredAnswer == correctAnswerStr) {
      server.send(200, "text/html", getSuccessPage());
    } else if (accept_wrong) {
      server.send(200, "text/html", getSuccessIshPage());
    } else {
      server.send(200, "text/html", getFailurePage());
    }
  } else {
    server.send(200, "text/html", getNoAnswerPage());
  }
}

void handleDisconnect() {
  const char* ssid = jsonData["ssid"];
  WiFi.softAPdisconnect(ssid);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  WiFi.softAP(ssid, NULL, 1, 0, 4, 100);
  digitalWrite(LED_BUILTIN, LOW);
}

String getQuestionPage() {
  String question = jsonData["question"];
  if (question.isEmpty()) {
    question = "~!Nog geen vraag geconfigureerd!~";
  }

  String page = "<html><head>";
  page += "<style>";
  page += "body { font-family: 'Arial', sans-serif; background-color: #f0f0f0; }"; // Achtergrondkleur en lettertype
  page += "h1 { font-size: 48px; color: #333; text-align: center; }"; // Stijl voor de hoofdtekst
  page += "p { font-size: 36px; color: #555; text-align: center; }"; // Stijl voor de paragrafen
  page += "input[type='text'] { width: 100%; padding: 15px; font-size: 36px; text-align: center; background-color: #fff; border: 2px solid #008CBA; }"; // Stijl voor het tekstveld
  page += "input[type='submit'] { width: 100%; padding: 15px; font-size: 36px; background-color: #008CBA; color: white; border: none; cursor: pointer; }"; // Stijl voor de knop
  page += "</style>";
  page += "</head><body>";
  page += "<h1>Vraag:</h1>";
  page += "<p>" + question + "</p>";
  page += "<form method='POST' action='/check'>";
  page += "<input type='text' name='answer' placeholder='Antwoord hier' />";
  page += "<br><br>";
  page += "<input type='submit' value='Controleer' />";
  page += "</form>";
  page += "</body></html>";
  return page;
}

String getSuccessPage() {
  String code = jsonData["code"];
  String page = "<html><head>";
  page += "<style>";
  page += "body { font-family: 'Arial', sans-serif; background-color: #e0f0e0; }"; // Achtergrondkleur en lettertype
  page += "h1 { font-size: 48px; color: #009900; text-align: center; }"; // Stijl voor de hoofdtekst
  page += "p { font-size: 36px; color: #555; text-align: center; }"; // Stijl voor de paragrafen
  page += ".button { display: block; width: 400px; height: 60px; margin: 0 auto; background-color: #008CBA; color: white; font-size: 24px; text-align: center; line-height: 60px; text-decoration: none; border-radius: 10px; }"; // Stijl voor de knop
  page += "</style>";
  page += "</head><body>";
  page += "<h1>Correct antwoord!</h1>";
  page += "<p>" + code + "</p>";
 
  page += "<a class='button' href='/disconnect'>Verbinding verbreken</a>";

  page += "</body></html>";
  return page;
}

String getSuccessIshPage() {
  String code = jsonData["code_wrong"];
  String page = "<html><head>";
  page += "<style>";
  page += "body { font-family: 'Arial', sans-serif; background-color: #e0f0e0; }"; // Achtergrondkleur en lettertype
  page += "h1 { font-size: 48px; color: #009900; text-align: center; }"; // Stijl voor de hoofdtekst
  page += "p { font-size: 36px; color: #555; text-align: center; }"; // Stijl voor de paragrafen
  page += ".button { display: block; width: 400px; height: 60px; margin: 0 auto; background-color: #008CBA; color: white; font-size: 24px; text-align: center; line-height: 60px; text-decoration: none; border-radius: 10px; }"; // Stijl voor de knop
  page += "</style>";
  page += "</head><body>";
  page += "<h1>Correct antwoord!</h1>";
  page += "<p>" + code + "</p>";
 
  page += "<a class='button' href='/disconnect'>Verbinding verbreken</a>";

  page += "</body></html>";
  return page;
}

String getFailurePage() {
  String page = "<html><head>";
  page += "<style>";
  page += "body { font-family: 'Arial', sans-serif; background-color: #f0e0e0; }"; // Achtergrondkleur en lettertype
  page += "h1 { font-size: 48px; color: #ff0000; text-align: center; }"; // Stijl voor de hoofdtekst
  page += "p { font-size: 36px; color: #555; text-align: center; }"; // Stijl voor de paragrafen
  page += ".button { display: block; width: 400px; height: 60px; margin: 0 auto; background-color: #008CBA; color: white; font-size: 24px; text-align: center; line-height: 60px; text-decoration: none; border-radius: 10px; margin-bottom:30px; }"; // Stijl voor de knop
  page += "</style>";
  page += "</head><body>";
  page += "<h1>Fout antwoord</h1>";
  page += "<p>Probeer opnieuw.</p>";

  // Voeg een knop toe om terug te gaan naar de vorige pagina
  page += "<a class='button' href='/'>Terug naar de vraag</a>";
  page += "<a class='button' href='/disconnect'>Verbinding verbreken</a>";

  page += "</body></html>";
  return page;
}

String getNoAnswerPage() {
  String page = "<html><head>";
  page += "<style>";
  page += "body { font-family: 'Arial', sans-serif; background-color: #f0e0e0; }"; // Achtergrondkleur en lettertype
  page += "h1 { font-size: 48px; color: #ff9900; text-align: center; }"; // Stijl voor de hoofdtekst
  page += "p { font-size: 36px; color: #555; text-align: center; }"; // Stijl voor de paragrafen
  page += "</style>";
  page += "</head><body>";
  page += "<h1>Antwoord niet ontvangen</h1>";
  page += "<p>Voer een antwoord in.</p>";
  page += "</body></html>";
  return page;
}

void loadJsonFromFile(const char* filename, DynamicJsonDocument& jsonDoc) {
  File file = SPIFFS.open(filename, "r");
  
  if (!file) {
    return;
  }

  // Parse de inhoud van het JSON-bestand
  DeserializationError error = deserializeJson(jsonDoc, file);

  file.close();
}

void saveJsonToFile(const char* filename, const DynamicJsonDocument& jsonDoc) {
  File file = SPIFFS.open(filename, "w");

  if (!file) {
    return;
  }

  // Serializeer de JSON-gegevens naar het bestand
  serializeJson(jsonDoc, file);
  file.close();
}

void handleUpdateJson() {
  String jsonPayload = server.arg("plain");
  
  // Parse de ontvangen JSON-payload
  DeserializationError error = deserializeJson(jsonData, jsonPayload);

  // Controleer op fouten bij het parseren van JSON
  if (error) {
    server.send(400, "text/plain", "Fout bij het parsen van JSON");
    return;
  }

  // Sla de bijgewerkte JSON-gegevens op in het bestand
  saveJsonToFile("/data.json", jsonData);

  server.send(200, "text/plain", "JSON succesvol bijgewerkt");
}