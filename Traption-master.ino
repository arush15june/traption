/*
    Traption - ESP8266 WiFi AP Trap to scar for life.
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "FS.h"
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <DNSServer.h>
ADC_MODE(ADC_VCC);

/* Constants */
//WiFI Consts
const char* ssid_AP = "Connect To Discover.com";
const char* password_AP = "";
IPAddress apIP(192, 168, 12, 24);
IPAddress netMsk(255, 255, 255, 0);
const char* ssid_WIFI = "Charge-Fi";
const char* password_WIFI = "19951999";
//Update Server Consts
const char* host = "discover";
const char* update_path = "/firmware";
const char* update_username = "charge";
const char* update_password = "7452";
//DNS Server Consts
const byte DNS_PORT = 53;
DNSServer dnsServer;

/* Global Objects */
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
/* API Helpers */

void returnOK() {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "");
}

void returnOKs() {
  server.sendHeader("Connection", "refresh,20");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "");
}

void returnFail(String msg) {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

bool loadFromSpiffs(String path) {
  String dataType = "text/plain";
  if (path.endsWith("/")) path += "index.htm";

  if (path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if (path.endsWith(".htm")) dataType = "text/html";
  else if (path.endsWith(".css")) dataType = "text/css";
  else if (path.endsWith(".js")) dataType = "application/javascript";
  else if (path.endsWith(".png")) dataType = "image/png";
  else if (path.endsWith(".gif")) dataType = "image/gif";
  else if (path.endsWith(".jpg")) dataType = "image/jpeg";
  else if (path.endsWith(".ico")) dataType = "image/x-icon";
  else if (path.endsWith(".xml")) dataType = "text/xml";
  else if (path.endsWith(".pdf")) dataType = "application/x-pdf";
  else if (path.endsWith(".zip")) dataType = "application/x-zip";
  else if (path.endsWith(".gz")) {
    dataType = "application/x-gzip";
  }
  File dataFile = SPIFFS.open(path.c_str(), "r");
  if (server.hasArg("download")) dataType = "application/octet-stream";
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
  }

  dataFile.close();
  return true;
}

bool printOnRequest(String endpoint) {
  if (Serial.println("[API] Request for " + endpoint))
    return 1;
  return 0;
}
String getVisitCount() {

  if (!SPIFFS.exists("visits.txt"))
  {
    File visits_file = SPIFFS.open("visits.txt", "w");
    visits_file.println("0");
    visits_file.close();
  }
  File visits_file = SPIFFS.open("visits.txt", "r");
  String visitsCountStr = visits_file.readString();
  visits_file.close();
  return visitsCountStr;

}
void printVisitCount(int visits) {
  Serial.printf("Current Visit Count : %i\n", visits);
}

bool updateVisitCount() {

  int visits = getVisitCount().toInt();
  printVisitCount(visits);
  visits += 1;
  File visits_file = SPIFFS.open("visits.txt", "w");
  visits_file.println(visits);
  visits_file.close();

}

bool deleteComment(int no) {

File comments_file = SPIFFS.open("comments.csv","r+");
File comments_file_temp = SPIFFS.open("commentsCopy.csv","w+");
bool deleted = false;
int i = 0;
while (comments_file.position() != comments_file.size()) {
  String message = comments_file.readStringUntil(',');
  String author = comments_file.readStringUntil('\n');
  i += 1;
  if(i == no) {
    deleted = true;
    continue;
  }
  comments_file_temp.print(message);
  comments_file_temp.print(",");
  comments_file_temp.print(author);
  comments_file_temp.print("\n");
  comments_file_temp.close();
}
comments_file.close();
comments_file_temp.close();

SPIFFS.remove("comments.csv");
SPIFFS.rename("commentsCopy.csv","comments.csv");

return deleted;
  
}



/* API Handlers */

void handleNotFound() {
  printOnRequest("Not Found 404");
  if (loadFromSpiffs(server.uri())) return; //Load External Files if possible
  server.send(404, "text/plain", "404 PAGE NOT FOUND");
}

void handleRoot() {
  printOnRequest("Root");
  updateVisitCount();
  if (loadFromSpiffs("/")) return; // Send Index on Root
  else {
    String failMessage = "Couldn't load index from Spiffs";
    server.send(404, "text/plain", failMessage);
  }
}

void handleVCC() {
  printOnRequest("VCC");
  uint32_t VCC = ESP.getVcc();
  String vccJSON = "{ 'vcc' : '";
  vccJSON += VCC;
  vccJSON += "'}";
  server.send(200, "text/json", vccJSON);
}

void handleRestart() {
  printOnRequest("VCC");
  server.send(200, "text/plain", "Restarting Device");
  ESP.restart();
}

void handleStats() {
  printOnRequest("Stats");
  int visits = getVisitCount().toInt();
  Serial.println(visits);
  String statMsg = "Current Visit Count : ";
  statMsg += visits;
  server.send(200, "text/plain", statMsg);
}

void handleClearStats() {
  printOnRequest("ClearStats");
  File visits_file = SPIFFS.open("visits.txt", "w");
  visits_file.println("0");
  visits_file.close();
  server.send(200, "text/plain", "Cleared Visitor Count");
}

void addCommentHandler() {
  if (server.hasArg("message") && server.hasArg("author")) {
    String message = server.arg("message");
    String author = server.arg("author");
    File comments_file = SPIFFS.open("comments.csv", "a");
    if (comments_file) {
      comments_file.print(message);
      comments_file.print(",");
      comments_file.print(author);
      comments_file.print("\n");
      comments_file.close();
      Serial.println("Recieved comment : " + message + " " + author);
      server.send(200, "text/plain", "Recieved comment : " + message + " " + author);
    }
    else {
      Serial.printf("couldn't add comment");
      server.send(200, "text/plain", "couldn't add comment");
    }
  }
}

void allCommentsHandler() {
  printOnRequest("CommentHandler");
  if (SPIFFS.exists("comments.csv")) {
    String commentsJson = "{ \"comments\" : [";
    File comments_file = SPIFFS.open("comments.csv", "r");
    while (comments_file.position() != comments_file.size()) {
      commentsJson += "{ \"message\" : \"";
      String message = comments_file.readStringUntil(',');
      String author = comments_file.readStringUntil('\n');
      commentsJson += message;
      commentsJson += "\", \"author\" : \"";
      commentsJson += author;
      commentsJson += "\"}";
      if (comments_file.position() == comments_file.size()) continue;
      commentsJson += ",";
    }
    comments_file.close();
    commentsJson += "] }";
    Serial.println(commentsJson);
    server.send(200, "text/json", commentsJson);
  }

  Serial.printf("[Comment Handler] comments file does not exist\n");

}

void deleteCommentsHandler() {
  if(server.hasArg("comment")) {
    int no = server.arg("comment").toInt();
    if(deleteComment(no)) server.send(200,"text/plain","Deleted comment no  : "+no);
    else server.send(200,"text/plain","Couldn't delete comment no : "+no);
  } else server.send(200,"text/plain","Wrong Arguments");

}


void setup(void) {
  /* WiFi init */
  Serial.begin(115200);
  Serial.println();
  Serial.println("[Boot] Booted Device.");
  WiFi.mode(WIFI_AP_STA);
  // Connect to Home Network
  WiFi.begin(ssid_WIFI, password_WIFI);
  // Serve a WiFi Access Point
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(ssid_AP, password_AP);
  /* SPIFFS Init */
  if (!SPIFFS.begin()) {
    Serial.println("[I/O] SPIFFS Mount Failed");
    ESP.restart();
  }
  else {
    Serial.println("[I/O] SPIFFS Mount Successful");
  }
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  String str;
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    str += dir.fileName();
    str += " / ";
    str += dir.fileSize();
    str += "\r\n";
  }
  Serial.print(str);
  Serial.printf("Flash total bytes: %u\n", fs_info.totalBytes);
  Serial.printf("Flash used bytes: %u\n", fs_info.usedBytes);
  Serial.printf("Flash free bytes: %u\n", fs_info.totalBytes - fs_info.usedBytes);

  /* mDNS service init */
  MDNS.begin(host);

  httpUpdater.setup(&server, update_path, update_username, update_password);

  MDNS.addService("http", "tcp", 80);
  //Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);

  /* DNS Service Init */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  /* API Routes */

  server.on("/", handleRoot);
  server.on("/restart", handleRestart);
  server.on("/stats", handleStats);
  server.on("/clearStats", handleClearStats);
  server.on("/VCC", HTTP_GET, handleVCC);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/add", HTTP_POST, addCommentHandler);
  server.on("/comments", HTTP_GET, allCommentsHandler);
  server.on("/delete", HTTP_POST, deleteCommentsHandler);
  server.onNotFound ( handleNotFound );
  server.begin(); // Web server start



}

void loop(void) {
  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();
}
