
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <SPIFFS.h>
#include "FS.h"
#include <PubSubClient.h>

#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <TimeLib.h>
//#include "Scheduler.h>"
#include <SPI.h>
#include<Wire.h>



const char*   ntpServer           = "pool.ntp.org";
const long    gmtOffset_sec       = -5 * 60 * 60;   // -3h*60min*60s = -10800s
int8_t        cTimeZone;

const char* mqtt_server = "broker.mqtt-dashboard.com";
const char *ap_ssid = "CDB_ESP";
const char *ap_pass = "12345";


// Tamanho do Objeto JSON
const   size_t    JSON_SIZE = JSON_OBJECT_SIZE(5) + 130;

const byte        DNSSERVER_PORT                  = 53;

//IPAddress apIP(192, 168, 4, 1);
DNSServer         dnsServer;
WebServer server(80);   

WiFiClient espClient;
PubSubClient client(espClient);
      
// Variáveis Globais -----------------------------------
time_t        nextNTPSync         = 0;
time_t        loopNext            = 0;
//
char              id[30];       // Identificação do dispositivo
boolean           ledOn;        // Estado do LED
word              bootCount;    // Número de inicializações
char              ssid[30];     // Rede WiFi
char              pw[30];       // Senha da Rede WiFi


byte rxBuf[8];
char msgString[128];                        // Array to store serial string

// Agenda;
String            schedule;

// Último evento
String            lastEvent;

// Mapeamento de pinos
const byte        BUTTON_PIN                      = 0;
const byte        RELAY_PIN                       = 12;
const byte        LED_PIN                         = 13;

// Mapeamento HIGH/LOW do Relé
const byte        RELAY_HIGH                      = HIGH;
const byte        RELAY_LOW                       = LOW;

float sensorPin = 33;

//Mapeamento de pinos do NodeMCU
#define led0    21
#define led1    5
#define led2    4
#define led3    0
#define ledPwm 25


int valueD0 = HIGH;
int valueD1 = HIGH;
int valueD2 = HIGH;
int valueD3 = HIGH;
 
int cont = 0;
float valor;
float temperatura = ((valor*250)/1023);

// Funções auxiliares -----------------------------------
/*******************************************************************************
* FUNÇÕES DO SISTEMA DE AGENDA
* Opções:
*   - Agenda para data/hora específica
*     On (High): SH yyyy-mm-dd hh:mm
*     Off (Low): SL yyyy-mm-dd hh:mm
*   - Diário
*     On (High): DH hh:mm
*     Off (Low): DL hh:mm
*   - Intervalo
*     On (High): IH hh:mm
*     Off (Low): IL hh:mm
* Exemplos:
*   SH 2018-10-12 16:30  - Ligar em 12/10/2018 16:30
*   MH 12 16:30          - Ligar mensalmente no dia 12 às 16:30
*   WL 6 16:30           - Desligar semanalmente nas sextas-feiras às 16:30 
*   DH 16:30             - Ligar diariamente às 16:30
*   IH 00:30             - Desligar após estar ligado por 30 minutos
*   IL 00:10             - Ligar após estar desligado por 10 minutos
*******************************************************************************/
// Scheduller Variables
time_t  schLastCheck            = 0,
        schHighDT,
        schLowDT;
        
int8_t cfgTimeZone() {
  return cTimeZone;
}
byte relayRead() {
  // Lê o estado do relé
  return digitalRead(RELAY_PIN);
}

void relayWrite(const byte &state) {
  // Grava o estado do relé
  digitalWrite(RELAY_PIN, state);
}

String hhmmStr(const time_t &t) {
  // Retorna time_t como "hh:mm"
  String s = "";
  if (hour(t) < 10) {
    s += '0';
  }
  s += String(hour(t)) + ':';
  if (minute(t) < 10) {
    s += '0';
  }
  s += String(minute(t));
  return s;
}

void scheduleIntervalReset() {
  // Reinicia temporizador de intervalo
  schHighDT = now(),
  schLowDT = schHighDT;
}

String scheduleChk(const String &sch) {
  // Função principal da Agenda

  // Verifica intervalo mínimo (10s)
  if (now() - schLastCheck < 10) {
    return "";
  }

  byte pinStatus;
  String event, events = "";

  // Obtem Data/Hora
  schLastCheck = now();
  time_t nowTZ = schLastCheck + 0 * 3600;
  
  // Processa Agenda
  event = "";

  // Data/Hora como "yyyy-mm-dd hh:mm"
  String dt = String(year(nowTZ)) + '-';
  if (month(nowTZ) < 10) {
    dt += '0';
  }
  dt += String(month(nowTZ)) + '-';
  if (day(nowTZ) < 10) {
    dt += '0';
  }
  dt += String(day(nowTZ)) + ' ';
  if (hour(nowTZ) < 10) {
    dt += '0';
  }
  dt += String(hour(nowTZ)) + ':';
  if (minute(nowTZ) < 10) {
    dt += '0';
  }
  dt += String(minute(nowTZ));

  // Verifica ligar em Data/Hora específica - SH yyyy-mm-dd hh:mm
  String s = "SH " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
  // LED_BUILTIN = HIGH;
    pinStatus = RELAY_HIGH;
    goto process;
  }

  // Verifica desligar em Data/Hora específica - SL yyyy-mm-dd hh:mm
  s = "SL " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    //LED_BUILTIN = LOW;
    pinStatus = RELAY_LOW;
    goto process;
  }
  
  // Data/Hora como "hh:mm"
  dt = dt.substring(2);

  s = "DH " + dt; // Verifica ligar diário - DH hh:mm
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = RELAY_HIGH;
    //LED_BUILTIN = HIGH;
    goto process;
  }

  s = "DL " + dt;// Verifica desligar diário - DL hh:mm
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = RELAY_LOW;
   
    goto process;
  }

  s = "IH " + hhmmStr(schLastCheck - schHighDT);  // Verifica ligar intervalado - IH hh:mm
  if (sch.indexOf(s) != -1 && relayRead() == RELAY_HIGH) {
    event = s;
    pinStatus = RELAY_LOW;
    goto process;
  }
  
  s = "IL " + hhmmStr(schLastCheck - schLowDT);// Verifica desligar intervalado - ID hh:mm
  if (sch.indexOf(s) != -1 && relayRead() == HIGH) {
    event = s;
    pinStatus = RELAY_HIGH;
   
  }

  process:  // Processa evento
  if (event != "" && pinStatus != relayRead()) {
    relayWrite(pinStatus);
    if (pinStatus == RELAY_HIGH) {
      schHighDT = schLastCheck; // Registra Data/Hora ligado
    } else {
      schLowDT = schLastCheck;  // Registra Data/Hora desligado
    }
    lastEvent = event + " - " + dateTimeNowTZStr();
    events += " " + event;
  }
  return events;
}

bool scheduleSet(const String &sch) {
  // Grava arquivo da agenda
  File file = SPIFFS.open(F("/Schedule.txt"), "w+");
  if (file) {
    file.print(sch);
    file.close();
    scheduleIntervalReset();
    return true;
  }
  return false;
}

String scheduleGet() {
  // Lê arquivo da agenda
  String s = "";
  File file = SPIFFS.open(F("/Schedule.txt"), "r");
  if (file) {
    file.setTimeout(100);
    s = file.readString();
    file.close();
  }
  return s;
}
String dateTimeStr(time_t t, int8_t tz = -3) {
  //time_t = seg desde 70 até hoje.
  // Formata time_t como "aaaa-mm-dd hh:mm:ss"
  if (t == 0) {
    return "N/D";
  } else {
    t += tz * 3600;                               // Ajusta fuso horário
    struct tm *ptm;
    ptm = gmtime(&t);
    String s;
    s = ptm->tm_year + 1900;
    s += "-";
    if (ptm->tm_mon < 9) {
      s += "0";
    }
    s += ptm->tm_mon + 1;
    s += "-";
    if (ptm->tm_mday < 10) {
      s += "0";
    }
    s += ptm->tm_mday;
    s += " ";
    if (ptm->tm_hour < 10) {
      s += "0";
    }
    s += ptm->tm_hour;
    s += ":";
    if (ptm->tm_min < 10) {
      s += "0";
    }
    s += ptm->tm_min;
    s += ":";
    if (ptm->tm_sec < 10) {
      s += "0";
    }
    s += ptm->tm_sec;
    return s;
  }
}

String dateTimeNowTZStr() {
  // Retorna time_t como string considerando timezone
  return dateTimeStr(now(), cfgTimeZone());
}
//
//
//timeStatus() {
//  // Obtém o status da sinronização
//  if (nextNTPSync == 0) {
//    return "não definida";
//  } else if (time(NULL) < nextNTPSync) {
//    return "atualizada";
//  } else {
//    return "atualização pendente";
//  }
//}

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
    //return "CDB_esp" + hexStr(ESP.getEfuseMac());
        return "TOITECRIS" ;
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
  strlcpy(id, "Reseted", sizeof(id)); 
  strlcpy(ssid, "CDB-2.4G", sizeof(ssid)); 
  strlcpy(pw, "abcde12345", sizeof(pw)); 
  ledOn     = false;
  bootCount = 0;
  scheduleIntervalReset;
}

void reboot() {
  // Reinicia o dispositivo

  ESP.restart();
}

boolean configRead() {
  // Lê configuração
  StaticJsonDocument<JSON_SIZE> jsonConfig;

  File file = SPIFFS.open(F("/Configura.json"), "r");
  if (deserializeJson(jsonConfig, file)) {
    // Falha na leitura, assume valores padrão
    
 // configReset();

  //  log(F("Falha lendo CONFIG, assumindo valores padrão."));
    return false;
  } else {
    // Sucesso na leitura
    strlcpy(id, jsonConfig["id"]      | "IOTEC Device", sizeof(id)); 
    strlcpy(ssid, jsonConfig["ssid"]  | "", sizeof(ssid)); 
    strlcpy(pw, jsonConfig["pw"]      | "", sizeof(pw)); 
    ledOn     = jsonConfig["led"]     | false;
    bootCount = jsonConfig["boot"]    | 0;

    file.close();

   // log(F("\nLendo config:"));
    serializeJsonPretty(jsonConfig, Serial);
   // log("");
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
    //log(F("\nGravando config:"));
    serializeJsonPretty(jsonConfig, Serial);
    return true;
  }
  return false;


  
}

// Requisições Web --------------------------------------
void handleHome() {
  // Home
  File file = SPIFFS.open(F("/Home.htm"), "r");
  if (file) {
    file.setTimeout(100);
    String s = file.readString();
    file.close();
   
    // Atualiza conteúdo dinâmico
   // s.replace(F("#led#")  , ledOn ? F("Ligado") : F("Desligado"));
    s.replace(F("#id#")   , id);
    s.replace(F("#ssid#") , ssid);
    s.replace(F("#pw#")   , pw);
    s.replace(F("#sysIP#")    , ipStr(WiFi.status() == WL_CONNECTED ? WiFi.localIP() : WiFi.softAPIP()));
    s.replace(F("#clientIP#") , ipStr(server.client().remoteIP()));
    
    // Envia dados
    server.send(200, F("text/html"), s);
  //  log("Home - Cliente: " + ipStr(server.client().remoteIP()) + (server.uri() != "/" ? " [" + server.uri() + "]" : ""));
  } else {
    server.send(500, F("text/plain"), F("Home - ERROR 500"));
  //  log(F("Home - ERRO lendo arquivo"));
  }
}

void handleConfig() {
   // Configura
  File file = SPIFFS.open(F("/Config.html"), "r");
  if (file) {
    file.setTimeout(100);
    String s = file.readString();
    file.close();
    
    // Atualiza conteúdo dinâmico
   // s.replace(F("#id#")     , id);
   // s.replace(F("#ledOn#")  ,  ledOn ? " checked" : "");
   //s.replace(F("#ledOff#") , !ledOn ? " checked" : "");
    s.replace(F("#ssid#")   , ssid);
    s.replace(F("#pw#")   , pw);
    
     Serial.println("scan start");

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            delay(10);
        }
    }
    Serial.println("");

    // Wait a bit before scanning again
    delay(5000);
    // Envia dados
    server.send(200, F("text/html"), s);
    log("Configura - Cliente: "  + ipStr(server.client().remoteIP()));
  } else {
    server.send(500, F("text/plain"), F("Configura - ERROR 500"));
    log(F("Configura - ERRO lendo arquivo"));

}
}

void handleConfigSave() {
  // Grava Config
  // ESP32 envia apenas os 4 campos
  if (server.args() == 2) {

    String s;
//    // Grava id
//    s = server.arg("id");
//    s.trim();
//    if (s == "") {
//      s = deviceID();
//   }
//   strlcpy(id, s.c_str(), sizeof(id));

    // Grava ssid
    s = server.arg("ssid");
    s.trim();
    strlcpy(ssid, s.c_str(), sizeof(ssid));

    // Grava pw
    s = server.arg("pw");
    s.trim();
    if (s != "") {
      // Atualiza senha, se informada
      strlcpy(pw, s.c_str(), sizeof(pw));
    }

    // Grava ledOn
  //  ledOn = server.arg("led").toInt()
    // Atualiza LED
    //ledSet();
    // Grava configuração
    if (configSave()) {
      server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Configuração salva.');history.back()</script></html>"));
//      log("ConfigSave - Cliente: " + ipStr(server.client().remoteIP()));
    } else {
     server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Falha salvando configuração.');history.back()</script></html>"));
     // log(F("ConfigSave - ERRO salvando Config"));
    }
  } else {
    server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Erro de parâmetros.');history.back()</script></html>"));
  }
}

void handleReconfig() {
  // Reinicia Config
  configReset();

  // Atualiza LED
 // ledSet();

  // Grava configuração
  if (configSave()) {
    server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Configuração reiniciada.');window.location = '/'</script></html>"));
   // log("Reconfig - Cliente: " + ipStr(server.client().remoteIP()));
  } else {
    server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Falha reiniciando configuração.');history.back()</script></html>"));
  //  log(F("Reconfig - ERRO reiniciando Config"));
  }
}

void handleReboot() 
{
  // Root
  File file = SPIFFS.open(F("/reboot.html"), "r");
  if (file) {
    file.setTimeout(100);
    String s = file.readString();
    file.close();
    
// --- Excluindo arquivo ---
  Serial.println("\nExcluindo Arquivo ----------------------------");
  if (SPIFFS.remove("/Log/Log.txt")) {
    Serial.println("Arquivo '/Log/Log.txt' excluído");
  } else {
    Serial.println("Exclusão '/Log/Log.txt' falhou");
  }
  reboot();
    // Envia dados
    server.send(200, F("text/html"), s);
  //  log("Home - Cliente: " + ipStr(server.client().remoteIP()) + (server.uri() != "/" ? " [" + server.uri() + "]" : ""));
  } else {
    server.send(500, F("text/plain"), F("Home - ERROR 500"));
  //  log(F("Home - ERRO lendo arquivo"));
  }
}

void handleMonitor() {
   // Monitor
  File file = SPIFFS.open(F("/Monitor.html"), "r");
  if (file) {
    file.setTimeout(100);
    String s = file.readString();
    file.close();

    // Envia dados
    server.send(200, F("text/html"), s);
//    log("JS - Cliente: " + ipStr(server.client().remoteIP()));
  } else {
    server.send(500, F("text/plain"), F("Monitor - ERROR 500"));
 //   log(F("JS - ERRO lendo arquivo"));

  }
}

void handleRelogio() {
  
   // Configura
  File file = SPIFFS.open(F("/Relogio.html"), "r");
  if (file) {
    file.setTimeout(100);
    String s = file.readString();
    file.close();

    // Envia dados
    server.send(200, F("text/html"), s);
//    log("JS - Cliente: " + ipStr(server.client().remoteIP()));
  } else {
    server.send(500, F("text/plain"), F("Home - ERROR 500"));
   // log(F("JS - ERRO lendo arquivo"));

}
}

void handleCSS() {
  // Arquivo CSS
  File file = SPIFFS.open(F("/Style.css"), "r");
  if (file) {
    // Define cache para 3 dias
    server.sendHeader(F("Cache-Control"), F("public, max-age=172800"));
    server.streamFile(file, F("text/css"));
    file.close();
  //  log("CSS - Cliente: " + ipStr(server.client().remoteIP()));
  } else {
    server.send(500, F("text/plain"), F("CSS - ERROR 500"));
 //   log(F("CSS - ERRO lendo arquivo"));
  }
}

void handleloggraf()
{ 
   // Grafico
  File file = SPIFFS.open(F("/loggraf.html"), "r");
  if (file) {
    file.setTimeout(100);
    String s = file.readString();
    file.close();
    
    // Envia dados
    server.send(200, F("text/html"), s);
//    log("JS - Cliente: " + ipStr(server.client().remoteIP()));
  } else {
    server.send(500, F("text/plain"), F("Grafico - ERROR 500"));
  //  log(F("JS - ERRO lendo arquivo"));
  }
}

void getTemp()
{
byte t;
String json;
int i;

  StaticJsonDocument<JSON_SIZE> doc;
  JsonObject Dados  = doc.to<JsonObject>();
  JsonObject Tempo = Dados.createNestedObject("Tempo");
     for (int i = 0; i < 1; i++){
    //JsonArray LOG = doc.createNestedArray("data1");
    //JsonArray Tempo = doc.to<JsonArray>();
    
     int valor = analogRead(sensorPin);
     t = ((valor*250)/1023);
    Tempo["Hora"] = (dateTimeStr(time(NULL)));
    Tempo["Temp"] = i;
   
    }
     serializeJson(doc, json);
    server.send(200, F("text/html"), json);
//String json = "{\"temperature\":"+String(t)+"}";

}

void handleLog_view() {
byte t;
String json;
int i;

  StaticJsonDocument<JSON_SIZE> doc;
  JsonObject Dados  = doc.to<JsonObject>();
  JsonObject Tempo = Dados.createNestedObject("Tempo");
   {
     int valor = analogRead(sensorPin);
     t = ((valor*250)/1023);
    
    Tempo["Hora"] = (dateTimeStr(time(NULL)));
    Tempo["Temp"] = t;
    Tempo["Day"]=(day());
    Tempo["Month"]=(month());
    Tempo["Year"]=(year()); 
    }
     serializeJson(doc, json);

  File datafile = SPIFFS.open(F("/log_view.txt"),"a"); // Now write data in FS
  if (datafile) {
      datafile.setTimeout(100);
      datafile.println(json);
      datafile.close();
  }
     datafile = SPIFFS.open(F("/log_view.txt"),"r"); // Now read data from FS
  if (datafile) {
     String json = datafile.readString();
     datafile.close();
     
      Serial.print(json);
     server.send(200, F("text/html"), json);
  }
}


void handleJavaScript() {
  File file = SPIFFS.open(F("/js.js"), "r");
  if (file) {
    file.setTimeout(100);
    String s = file.readString();
    file.close();
 //handleStream(F("js.js"), F("application/javascript"));
 server.send(200, F("text/html"), s);
}
}


void setup_wifi() 
{
  //client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);
  
  // WiFi Station
  WiFi.begin(ssid, pw);
//  log("Conectando WiFi " + String(ssid));
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
 
// Configura WiFi AP para ESP32
   
   // WiFi.mode(WIFI_AP);
   
   WiFi.setHostname("CDB_ESP1");
   // WiFi.softAP(deviceID().c_str(), deviceID().c_str());
   //WiFi.softAP(ap_ssid,ap_pass);
   WiFi.softAP("CDB_ESP1","12345678");
    IPAddress myIP = WiFi.softAPIP();
    log("WiFi AP " + deviceID() + " - IP " + ipStr(WiFi.softAPIP()));
 
  // Habilita roteamento DNS
  //dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  //dnsServer.start(DNSSERVER_PORT, "*", WiFi.softAPIP());
  
   Serial.println(dateTimeStr(time(NULL)) + "\tStatus: " + timeStatus());
  
  
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.println(WiFi.macAddress());
  Serial.println("");
  }
}

void ntpSync_cb(struct timeval *tv) {
  time_t t;
  t = time(NULL);
  // Data/Hora da próxima atualização
  //nextNTPSync = t + (SNTP_UPDATE_DELAY / 1000) + 60;
nextNTPSync = t + 60;
  Serial.println("Sincronizou com NTP em " + dateTimeStr(t));
  Serial.println("Limite para próxima sincronização é " +
                  dateTimeStr(nextNTPSync));

handleLog_view();
                  
}


void setup(){

  Serial.begin(9600);
  while(!Serial);
  
   sntp_set_time_sync_notification_cb(ntpSync_cb);
 
  // Intervalo de sincronização - definido pela bibioteca lwIP
  Serial.printf("\n\nNTP sincroniza a cada %d segundos\n",
                SNTP_UPDATE_DELAY / 1000);
  
  // Função para inicializar o cliente NTP
  configTime(gmtOffset_sec,0, ntpServer);
  
  // SPIFFS
  if (!SPIFFS.begin()) {
  //  log(F("SPIFFS ERRO"));
    while (true);
  }

  pinMode(sensorPin, INPUT);
  pinMode(led0, OUTPUT);
  pinMode(led1, OUTPUT);
 
  digitalWrite(led0, HIGH);
  digitalWrite(led1, HIGH);
  
  configRead();
  configSave();
  
  setup_wifi();
 

  
Serial.println("Informações");
Serial.printf( "MAC: ", WiFi.macAddress());
Serial.printf("totalBytes: %u\n usedBytes: %u\n freeBytes: %u\n",
  SPIFFS.totalBytes(),
  SPIFFS.usedBytes(),
  SPIFFS.totalBytes() - SPIFFS.usedBytes());

server.on("/jsonTemp", HTTP_GET, getTemp); 
server.on("/log_view",HTTP_GET,handleLog_view);

server.on("/Monitor",handleMonitor);
server.on("/loggraf",handleloggraf);
server.on("/configSave", handleConfigSave);
server.on("/Config", handleConfig);
server.on("/Relogio", handleRelogio);

server.on("/Home", handleHome);
server.on("/reconfig", handleReconfig);
server.on("/reboot", handleReboot);
server.on("/", handleHome);

//server.on("/home_btn.png",HTTP_GET,handleLogo);
server.on("/css" , handleCSS);
server.on("/js.js",HTTP_GET, handleJavaScript);
//server.onNotFound(onNotFound);
server.begin();
Serial.println("Servidor HTTP iniciado");

  // Agenda
  scheduleIntervalReset();
  schedule = scheduleGet();

  // DEFINE ENTRADAS DE AGENDA - APENAS PARA TESTE
 
    time_t t = now() + 61 + 0 * 3600;
    schedule = //"SH 2022-08-01 19:20";  //- Ligar em 30/07/2022 13:00
    
              "SH " + dateTimeStr( t      ,  false).substring(0, 16) +
              "\nSL " + dateTimeStr( t + 60 ,  false).substring(0, 16) +
              "\nMH " + dateTimeStr( t + 120,  false).substring(8, 16) +
              "\nML " + dateTimeStr( t + 180,  false).substring(8, 16) +
              "\nWH " +     weekday( t + 240) + " " + dateTimeStr( t + 240,  false).substring(11, 16) +
              "\nWL " +     weekday( t + 300) + " " + dateTimeStr( t + 300,  false).substring(11, 16) +
              "\nDH " + dateTimeStr( t + 360, false).substring(11, 16) +
              "\nDL " + dateTimeStr( t + 420, false).substring(11, 16) +
              "\nIH 00:01\nIL 00:01";
             
    // Verifica Agenda
    String s = scheduleChk(schedule);
    if (s != "") {
      Serial.println("Agendamento");
 // log(F("Agendamento"), s);
      Serial.println(s);
      handleLog_view();
    }    
    Serial.println("Agendamento");
    Serial.println(s);
   
}
  
void loop() 
{
   // WatchDog ----------------------------------------
  yield();

 dnsServer.processNextRequest();
 server.handleClient(); 

 if (now() > loopNext) {           
    // Verifica Agenda
    String s = scheduleChk(schedule);
    if (s != "") {
      Serial.println("Agendamento");
 // log(F("Agendamento"), s);
      Serial.println(s);
      handleLog_view();
    }    

 loopNext = now();
 }
}
  
