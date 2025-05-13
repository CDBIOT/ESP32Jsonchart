
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <SPIFFS.h>
#include "FS.h"

#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <TimeLib.h>


//#include "schedul.h"

#include "time.h"
#include "sntp.h"

#include <SPI.h>
#include<Wire.h>

//Funções de outros arquivos

void handleTemp();
void handleHome();
void handleConfig();
void handleConfigSave();
void handleReconfig();
void handleReboot();
void handleMonitor();
void handleRelogio();
void handleCSS();
void handleloggraf();
void getTemp();
void handleLog_view();
void handleJavaScript();
void logDelete();
void reboot();


// Tamanho do Objeto JSON
const   size_t    JSON_SIZE = JSON_OBJECT_SIZE(5) + 130;

const byte        DNSSERVER_PORT                  = 53;

//IPAddress apIP(192, 168, 1, 1);
DNSServer         dnsServer;
WebServer server(80);  
 

         
// Variáveis Globais ------------------------------------
char              id[30];       // Identificação do dispositivo
boolean           ledOn;        // Estado do LED
word              bootCount;    // Número de inicializações
char              ssid[30];     // Rede WiFi
char              pw[30];       // Senha da Rede WiFi


//const char* mqtt_server = "broker.mqtt-dashboard.com";

byte rxBuf[8];
char msgString[128];                        // Array to store serial string

#define LEDPIN    2     // LED pin

//Mapeamento de pinos do NodeMCU
#define led0    16
#define led1    5
#define led2    4
#define led3    0

int valueD0 = HIGH;
int valueD1 = HIGH;
int valueD2 = HIGH;
int valueD3 = HIGH;
 
int cont = 0;
int valor = analogRead(A0);
float temperatura = ((valor*250)/1023);

// Funções Genéricas ------------------------------------
void log(String s) {
  // Gera log na Serial
  Serial.println(s);
}


  
String hexStr(const unsigned long &h, const byte &l = 8) {
  // Retorna valor em formato hexadecimal
  String s;
  s= String(h, HEX);
  s.toUpperCase();
  s = ("00000000" + s).substring(s.length() + 8 - l);
  return s;
}


String deviceID() {
    return "CDB SCAN-" + hexStr(ESP.getEfuseMac());
}

String ipStr(const IPAddress &ip) {
  // Retorna IPAddress em formato "n.n.n.n"
  String sFn = "";
  for (byte bFn = 0; bFn < 3; bFn++) {
    sFn += String((ip >> (8 * bFn)) & 0xFF) + ".";
  }
  sFn += String(((ip >> 8 * 3)) & 0xFF);
  return sFn;
}
//
//void ledSet() {
//  // Define estado do LED
//  digitalWrite(LED_PIN, ledOn ? LED_ON : LED_OFF);
//}

void  configReset() {
  // Define configuração padrão
  strlcpy(id, "IeC Device", sizeof(id)); 
  strlcpy(ssid, "CDB", sizeof(ssid)); 
  strlcpy(pw, "abcde12345", sizeof(pw)); 
  ledOn     = false;
  bootCount = 0;
}
boolean configRead() {
  // Lê configuração
  StaticJsonDocument<JSON_SIZE> jsonConfig;

  File file = SPIFFS.open(F("/Configura.json"), "r");
  if (deserializeJson(jsonConfig, file)) {
    // Falha na leitura, assume valores padrão
    
  configReset();

    log(F("Falha lendo CONFIG, assumindo valores padrão."));
    return false;
  } else {
    // Sucesso na leitura
    strlcpy(id, jsonConfig["id"]      | "", sizeof(id)); 
    strlcpy(ssid, jsonConfig["ssid"]  | "", sizeof(ssid)); 
    strlcpy(pw, jsonConfig["pw"]      | "", sizeof(pw)); 
    ledOn     = jsonConfig["led"]     | false;
    bootCount = jsonConfig["boot"]    | 0;

    file.close();

    log(F("\nLendo config:"));
    serializeJsonPretty(jsonConfig, Serial);
    log("");

    return true;
  }
}

boolean configSave() {
  // Grava configuração
  StaticJsonDocument<JSON_SIZE> jsonConfig;

  File file = SPIFFS.open(F("/Configura.json"), "w+");
  if (file) {
    // Atribui valores ao JSON e grava
    jsonConfig["ssid"]  = ssid;
    jsonConfig["pw"]    = pw;

    serializeJsonPretty(jsonConfig, file);
    file.close();

    log(F("\nGravando config:"));
    serializeJsonPretty(jsonConfig, Serial);
    log("");

    return true;
  }
  return false;
}

// Write the sensor readings on the SD card
void logdata() {

File file = SPIFFS.open(F("/data.txt"), "a");
  if (file) {
      file.printf("Registro %u\r\n");
  }

  file.close();
}

// Write the sensor readings on the SD card
void readlog() {

File file = SPIFFS.open(F("/data.txt"), "r");
  if (file) {
      file.printf("Registro %u\r\n");
  }

  file.close();
  
  String json = "{\"readlog\"}";
  server.send (200, "application/json", json);
}



void getLog()
{
float t;
byte tdata[10];// = {0x44,0x44,0x55,0x55,0x55,0x55,0x55,0x55};
String json;
 
   // static byte pids[] = {PID_RPM,PID_SPEED,PID_COOLANT_TEMP ,PID_FUEL_PRESSURE,PID_INTAKE_MAP,
     //                     PID_THROTTLE , PID_FUEL_LEVEL ,PID_BAROMETRIC , PID_CONTROL_MODULE_VOLTAGE , PID_AMBIENT_TEMP};
   // static byte index = 0;
   // byte pid = pids[index];

    for (int i = 0; i<10; i++)
    {
    int valor = analogRead(A0);
    t = ((valor*250)/1023);
    tdata[i]=t;
    }
  //index = (index +1) % sizeof(pids);
  
    StaticJsonDocument<JSON_SIZE> doc;
   JsonObject LOG = doc.to<JsonObject>();

    JsonArray data1 = LOG.createNestedArray("Hora");
    JsonArray data2 = LOG.createNestedArray("Temp");
    for (int j = 0; j<10; j++){
    data1.add(j);
    data2.add(hexStr(tdata[j]));
     }
//   JsonArray data2 = LOG.createNestedArray("Temp");
//    
//    for (int k = 0; k<10; k++){
//    int valor = analogRead(A0);
//     float t = ((valor*250)/1023);
//    tdata[k]=t;
//    data2.add(hexStr(tdata[k]));
 //   }   
        
//    serializeJson(LOG, json);
//    Serial.println(json);

//json = "{\"LOG\":"+String(t)+"}";//Cria um json com os dados da temperatura
// String clientId = "cdbiot123";
//clientId += String(random(0xffff), HEX);// exemplo de conversão para HEX
//  server.send (200, "application/json", json);
  
//String json;

//int a = analogRead(A0);
//String adcValue = String(a);
//server.send(200, "text/plane", t);
//    // Envia dados
//    server.send(200, F("text/html"), json);
//    log("Configura - Cliente: "  + ipStr(server.client().remoteIP()));
//  } else {
//    server.send(500, F("text/plain"), F("Temperatura- ERROR 500"));
//    log(F("Temperatura - ERRO lendo arquivo"));

   
File file = SPIFFS.open(F("/Temperatura.html"), "a+");
   
//for (int k = 0; k<10; k++)
{
 int valor = analogRead(A0);
  t = ((valor*250)/1023);
  file.println (t);
  file.printf("temp %u\r\n",t);
}
//file.close();//

// File file = SPIFFS.open(F("/Temperatura.html"), "r");

  if (file) {
    file.setTimeout(100);
    while(file.available()){
    Serial.println (file.readStringUntil('\n'));
    }
file.close();//

String json = "{\"temperature\":"+String(t)+"}";//Cria um json com os dados da temperatura
server.send (200, "application/json", json);

Serial.println(json);
}
}


void handleRoot() 
{
  String html =
 " <html>"

 "</html>";
  //Envia o html para o cliente com o código 200, que é o código quando a requisição foi realizada com sucesso
  server.send(200, "text/html",html);
}

void setup(){

  Serial.begin(9600);
  while(!Serial);
  
  // SPIFFS
  if (!SPIFFS.begin()) {
    log(F("SPIFFS ERRO"));
    while (true);
  }
    
  pinMode(led0, OUTPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);

  digitalWrite(led0, HIGH);
  digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);
  digitalWrite(led3, HIGH);

  
  configRead();
  configSave();
   
    // Configura WiFi para ESP32
    WiFi.setHostname(deviceID().c_str());
    WiFi.softAP(deviceID().c_str(), deviceID().c_str());
 
  log("WiFi AP " + deviceID() + " - IP " + ipStr(WiFi.softAPIP()));

  // Habilita roteamento DNS
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(DNSSERVER_PORT, "*", WiFi.softAPIP());

  // WiFi Station
  
  WiFi.begin(ssid, pw);
  log("Conectando WiFi " + String(ssid));
  byte b = 0;
  while(WiFi.status() != WL_CONNECTED && b < 60) {
    b++;
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    // WiFi Station conectado
    log("WiFi conectado (" + String(WiFi.RSSI()) + ") IP " + ipStr(WiFi.localIP()));
  } else {
    log(F("WiFi não conectado"));
  }

  
Serial.println("Informações");
Serial.printf("totalBytes: %u\n usedBytes: %u\n freeBytes: %u\n",
  SPIFFS.totalBytes(),
  SPIFFS.usedBytes(),
  SPIFFS.totalBytes() - SPIFFS.usedBytes());
  ESP.getFreeHeap();
  
server.on("/Temperatura", handleTemp);  
server.on("/LOG", HTTP_GET, getLog); 
server.on("/Monitor",handleMonitor);
server.on("/configSave", handleConfigSave);
//server.on("/Config", handleConfigura);
server.on("/Relogio", handleRelogio);
server.on("/css" , handleCSS);
server.on("/Home", handleHome);
server.on("/reconfig", handleReconfig);
server.on("/", handleHome);
//server.onNotFound(onNotFound);
server.begin();
Serial.println("Servidor HTTP iniciado");
}

void loop() 
{
   // WatchDog ----------------------------------------
  yield();
 
 dnsServer.processNextRequest();
 server.handleClient(); 
}
  
