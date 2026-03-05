//Final version separated files with scheduler chatgpt helper

#include <WebServer.h> //Local WebServer used to serve the configuration portal ( https://github.com/zhouhan0126/WebServer-esp32 )
//#include <WiFi.h>
#include <WiFiClient.h>
#include <SPIFFS.h>
#include "FS.h"

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h> // WiFi Configuration Magic ( https://github.com/zhouhan0126/WIFIMANAGER-ESP32 ) >> https://github.com/tzapu/WiFiManager (ORIGINAL)

#include <DNSServer.h>//Local DNS Server used for redirecting all requests to the configuration portal ( https://github.com/zhouhan0126/DNSServer---esp32 )
#include <TimeLib.h>

#include <WiFiUdp.h>

#include "Scheduler.h"

#include "time.h"
#include "sntp.h"

#include <SPI.h>
#include<Wire.h>

#include <SPI.h>


//Funções de outros arquivos
void handleHome();
void handleConfig();
void handleConfigSave();
void handleReconfig();
void handleReboot();
void handleMonitor();
void handleRelogio();

void handleloggraf();
void handleTemp();
void handleLogView();
void handleFileRead();
void handleListFiles();
void handleLogOne();

void handleJavaScript();
void handleCSS();

void getTemp();
void deleteLog();
void reboot();


extern void scheduleIntervalReset();
extern String scheduleGet();
extern String scheduleChk(const String &sch) ;


//Schedule Variables 

//Extern Variables
// Agenda;
String schedule;
String s;

float t;

// Max log entries
extern const byte      LOG_ENTRIES                     = 50;

// System Log
extern String            logStr[LOG_ENTRIES];
extern byte              logIndex      = 0;

extern String dateTimeStr();

// NTP sync interval
extern const int       NTP_INT                         = 6 * 60 * 60; // 6h

const char*   ntpServer           = "pool.ntp.org";
const long    gmtOffset_sec       = -5 * 60 * 60;   // -3h*60min*60s = -10800s
int8_t        cTimeZone;

extern int8_t   cTimeZone;

const char* mqtt_server = "broker.mqtt-dashboard.com";
const char* ap_ssid = "CDB_ESP";
const char* ap_pass = "12345";

// Replace with your network credentials
//const char* ssid =  "CDB-2.4G";     // Rede WiFi
//const char* pw = "abcde12345" ;     // Senha da Rede WiFi

//const char* ssid =  "KAWA";     // Rede WiFi
//const char* pw = "abcd123321" ;     // Senha da Rede Wi

// Tamanho do Objeto JSON
const   size_t    JSON_SIZE = JSON_OBJECT_SIZE(5) + 130;

const byte        DNSSERVER_PORT                  = 53;

//IPAddress apIP(192, 168, 1, 1);
DNSServer         dnsServer;
WebServer server(80);  
 
//declaração do objeto wifiManager
WiFiManager wifiManager;

WiFiClient espClient;
PubSubClient client(espClient);

//const char* mqtt_server = "broker.mqtt-dashboard.com";
         
// Variáveis Globais ------------------------------------

time_t        nextNTPSync         = 0;
time_t        loopNext            = 0;
time_t lastLog = 0;
time_t lastCheck =0;

char              id[30];       // Identificação do dispositivo
boolean           ledOn;        // Estado do LED
word              bootCount;    // Número de inicializações
char              ssid[30];     // Rede WiFi
char              pw[30];       // Senha da Rede WiFi


byte rxBuf[8];
char msgString[128];                        // Array to store serial string


#define SCK 18
#define MISO 19
#define MOSI 23
#define CS 5


#define LEDPIN    2     // LED pin

//Mapeamento de pinos do NodeMCU
#define led0    16
#define led1    5
#define led2    4
#define led3    0


#define A1_PIN 33
#define A2_PIN 32
// Mapeamento de pinos
const byte        BUTTON_PIN                      = 0;
const byte        RELAY_PIN                       = 21;
const byte        LED_PIN                         = 13;

int valueD0 = HIGH;
int valueD1 = HIGH;
int valueD2 = HIGH;
int valueD3 = HIGH;
 
int cont = 0;


// Scheduller Variables
//time_t  schLastCheck,
//        schHighDT,
//        schLowDT;
        
int8_t cfgTimeZone() {
  return cTimeZone;
}


byte buttonRead() {
  // Lê o estado do botão
  return digitalRead(BUTTON_PIN);
}

byte relayRead() {
  // Lê o estado do relé
  return digitalRead(RELAY_PIN);
}

void ledWrite(const byte &state) {
  // Grava o estado do LED
  digitalWrite(LED_PIN, state);
}



//
//void log(const String &type, const String &msg) {
//  // Generate log in memory
//  logStr[logIndex] = dateTimeStr(now()) + ";" + type + ";" + msg;
//  Serial.println(logStr[logIndex]);
//  // Increment log index;
//  logIndex = (logIndex + 1) % LOG_ENTRIES;
//}

////
//String timeStatus() {
//  // Obtém o status da sinronização
//  if (nextNTPSync == 0) {
//    return "não definida";
//  } else if (time(NULL) < nextNTPSync) {
//    return "atualizada";
//  } else {
//    return "atualização pendente";
//  }
//}

// Callback de sincronização
#ifdef ESP8266
  // ESP8266
  void ntpSync_cb() {
#else
  // ESP32
  void ntpSync_cb(struct timeval *tv) {
#endif
  time_t t;
  t = time(NULL);
  // Data/Hora da próxima atualização
  nextNTPSync = t + (SNTP_UPDATE_DELAY / 1000) + 60;
  //nextNTPSync = t + 60;

  Serial.println("Sincronizou com NTP em " + dateTimeStr(t));
  Serial.println("Limite para próxima sincronização é " +
                  dateTimeStr(nextNTPSync));
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
void ledSet() {
  // Define estado do LED
//  digitalWrite(LED_PIN, ledOn ? LED_ON : LED_OFF);
}

void  configReset() {
  // Define configuração padrão
 // strlcpy(id, "IeC Device", sizeof(id)); 
//  strlcpy(ssid, "CDB-2.4G", sizeof(ssid)); 
//  strlcpy(pw, "abcde12345", sizeof(pw)); 
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


void setScheduler(){

  StaticJsonDocument<JSON_SIZE> json_agenda;
  
  File file = SPIFFS.open(F("/setScheduler.json"), "w+");
  if (file) {

  }
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");

    if (scheduleSet(cmd)) {
      scheduleIntervalReset(); 
        Serial.println(cmd);
      server.send(200, "text/plain", "Agendamento salvo: " + cmd);
    } else {
      server.send(500, "text/plain", "Erro ao salvar agendamento.");
    }
  } else {
    server.send(400, "text/plain", "Parâmetro 'cmd' ausente.");
  }
  
}


void cleanupOldLogs() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  String today = getTodayFile();

  while (file) {
    String name = file.name();
    if (name.startsWith("/log_") && name != today) {
      SPIFFS.remove(name);
      Serial.println("Removido: " + name);
    }
    file = root.openNextFile();
  }
}

String getTodayFile() {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  char name[32];
  sprintf(name, "/Log/%04d-%02d-%02d.txt",
          t->tm_year + 1900,
          t->tm_mon + 1,
          t->tm_mday);
  return String(name);
}

void getLog()
{
String fileName = getTodayFile();

  StaticJsonDocument<JSON_SIZE> dados;
  
  StaticJsonDocument<JSON_SIZE> datas;
  
byte tdata[10];// = {0x44,0x44,0x55,0x55,0x55,0x55,0x55,0x55};

   // static byte pids[] = {PID_RPM,PID_SPEED,PID_COOLANT_TEMP ,PID_FUEL_PRESSURE,PID_INTAKE_MAP,
     //                     PID_THROTTLE , PID_FUEL_LEVEL ,PID_BAROMETRIC , PID_CONTROL_MODULE_VOLTAGE , PID_AMBIENT_TEMP};
   // static byte index = 0;
   // byte pid = pids[index];

byte t;
byte p;
String json;
int i;
  
  //JsonObject Dados  = doc.to<JsonObject>();
  //JsonObject Tempo = Dados.createNestedObject("Temp");
     for (int i = 0; i < 1; i++){
  //JsonArray Pressure = Dados.createNestedArray("Pressure");
    //JsonArray Tempo = doc.to<JsonArray>();
    
     int valor1 = analogRead(A1_PIN);
     float t = (valor1 *(3.3 / 4095.0));
     float temp = t * 100.0;
     
     int valor2 = analogRead(A2_PIN);
     float p = (valor2 * (3.3 / 4095.0));
      float pres = p;

     dados["hora"] = dateTimeStr(time(NULL));
     dados["temp"] = temp;
     dados["press"] = pres;
    //Tempo["Hora"] = (dateTimeStr(time(NULL)));
    //Temperature["Temp"] = t;
    //Pressure["Press"] = p;
   
    }
     serializeJson(dados, json);

//    for (int i = 0; i<10; i++)
//    {
//    int valor = analogRead(33);
//    t = ((valor*250)/1023);
//    tdata[i]=t;
//    }
  //index = (index +1) % sizeof(pids);
  

  //for (int k = 0; k<10; k++)
  {
   // int valor = analogRead(33);
   // t = ((valor*250)/1023);
  //  file.println (t);
   // file.printf("temp %u\r\n",t);
  }

    //JsonArray data1 = doc.createNestedArray("Hora");
    //JsonArray data2 = doc.createNestedArray("Temp");
    for (int j = 0; j<10; j++){
   // data1.add(j);
   // data2.add(hexStr(tdata[j]));
     }
//   JsonArray data2 = LOG.createNestedArray("Temp");
//    
//    for (int k = 0; k<10; k++){
//    int valor = analogRead(34);
//     float t = ((valor*250)/1023);
//    tdata[k]=t;
//    data2.add(hexStr(tdata[k]));
 //   }   
        
//    serializeJson(LOG, json);
//    Serial.println(json);

//json = "{\"LOG\":"+String(t)+"}";//Cria um json com os dados da temperatura


  //float temperatura = (valor * 250.0) / 1023.0;

  File file = SPIFFS.open(fileName, FILE_APPEND);
  if (file) {
    file.println(json);
    file.close();
    
    Serial.printf("Log Salvo");
  } else {
    Serial.println("Erro ao abrir fileName");
  }

//String json = "{\"temperature\":"+String(t)+"}";//Cria um json com os dados da temperatura

    server.send(200, F("application/json"), json);
    Serial.println(json);
}

void sendLogMQTT(const char* filename) {
  File f = SPIFFS.open(filename, "r");
  if (!f) return;

  while (f.available()) {
    String chunk = f.readStringUntil('\n');
    client.publish("esp/logs", chunk.c_str(), false);
  }

  f.close();
}

void sendDailyLog(String date) {

  String fileName = "/Log/log_" + date + ".json";

  File file = SPIFFS.open(fileName, "r");
  if (!file) {
    Serial.println("Erro abrindo arquivo diario");
    return;
  }

  String jsonPayload = file.readString();
  file.close();

  if (client.publish("esp32/dailylog", jsonPayload.c_str())) {
    Serial.println("Log diario enviado com sucesso!");
  } else {
    Serial.println("Erro ao enviar log MQTT");
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



//void ntpSync_cb(struct timeval *tv) {
//  time_t t;
//  t = time(NULL);
//  // Data/Hora da próxima atualização
//  //nextNTPSync = t + (SNTP_UPDATE_DELAY / 1000) + 60;
//nextNTPSync = t + 60;
//  Serial.println("Sincronizou com NTP em " + dateTimeStr(t));
//  Serial.println("Limite para próxima sincronização é " +
//                  dateTimeStr(nextNTPSync));

                  
//}


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

  client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);
  
  configRead();
  configSave();
   
    // Configura WiFi para ESP32 Access Point
    WiFi.setHostname(deviceID().c_str());
    WiFi.softAP(deviceID().c_str()), "1234567890";// seta rede ap e senha
 
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

Serial.println();
log("WiFi connected (" + String(WiFi.RSSI()) + ") IP " + ipStr(WiFi.localIP()));


Serial.println("Informações");
Serial.printf("totalBytes: %u\n usedBytes: %u\n freeBytes: %u\n",
  SPIFFS.totalBytes(),
  SPIFFS.usedBytes(),
  SPIFFS.totalBytes() - SPIFFS.usedBytes());
  ESP.getFreeHeap();
  

  // Intervalo de sincronização - definido pela bibioteca lwIP
  Serial.printf("\n\nNTP sincroniza a cada %d segundos\n",
                SNTP_UPDATE_DELAY / 1000);
  
 sntp_set_time_sync_notification_cb(ntpSync_cb);
  
  // Função para inicializar o cliente NTP
  configTime(gmtOffset_sec,0, ntpServer);
    
    Serial.println("Sincronizing");
    
//    setSyncProvider(timeNTP);
    setSyncInterval(NTP_INT);


server.on("/getlog",getLog );
server.on("/setSchedule",setScheduler );
server.on("/Temperatura", handleTemp);  
server.on("/graphic", handleloggraf);  
server.on("/fileList", handleListFiles); 
server.on("/fileRead", handleFileRead); 
server.on("/logReadOne", handleLogOne);

server.on("/log/deleteLog", deleteLog); 
server.on("/log", handleLogView); 
server.on("/Monitor",handleMonitor);
server.on("/Relogio", handleRelogio);

server.on("/css" , handleCSS);
server.on("/Home", handleHome);
server.on("/configSave", handleConfigSave);
server.on("/Config", handleConfig);
server.on("/reconfig", handleReconfig);
server.on("/reboot", handleReboot);
server.on("/", handleHome);
//server.onNotFound(onNotFound);
server.begin();
Serial.println("Servidor HTTP iniciado");

  // Agenda
  scheduleIntervalReset();
  schedule = scheduleGet();
  
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
  // DEFINE ENTRADAS DE AGENDA - APENAS PARA TESTE
 
  time_t t = now() + 61 + 0 * 3600;
  // schedule = //"SH 2022-08-01 19:20";  //- Ligar em 30/07/2022 13:00
    
  schedule = scheduleGet();
  
  // SET SCHEDULE ENTRIES - DEBUG ONLY
// // time_t t = now() + 61;
//  schedule = "SH"   + dateTimeStr( t      , false).substring(0, 16) +
//             "\nSL" + dateTimeStr( t + 60 , false).substring(0, 16) +
//             "\nMH" + dateTimeStr( t + 120, false).substring(8, 16) +
//             "\nML" + dateTimeStr( t + 180, false).substring(8, 16) +
//             "\nWH" + weekday( t + 240) + " " + dateTimeStr( t + 240, false).substring(11, 16) +
//             "\nWL" + weekday( t + 300) + " " + dateTimeStr( t + 300, false).substring(11, 16) +
//             "\nDH" + dateTimeStr( t + 360, false).substring(11, 16) +
//             "\nDL" + dateTimeStr( t + 420, false).substring(11, 16) +
//             "\nIH00:10\nIL00:10";//IL -Ligar após estar desligado por 10 minutos -IH -Desligar após estar ligado

            
             
  // Schedule Check
 // s = scheduleChk(schedule, RELAY_PIN);
//    // Verifica Agenda
//String s = scheduleChk(schedule);
//    if (s != "") {
//      Serial.println("Agendamento");
//      Serial.println(s);
//     getLog();
//    }    
//    Serial.println("Agendamento");
//    Serial.println(s);
}

void loop() 
{
   // WatchDog ----------------------------------------
  yield();

 dnsServer.processNextRequest();
 server.handleClient(); 
 
time_t nowTime = now();
String clientId = "cdbiot123"/

  if (!client.connected()) {
  client.connect(clientId);
  }
  client.loop();
  if(client.connected(clientId.c_str()){
    
    client.publish("esp/dailylog",jsonPayload.c_str());
    
  }
  
  
  
if (hora == 0 && minuto == 5) {
   sendDailyLog(dataOntem);
}



  // checa agenda a cada 5s
  if (nowTime - lastCheck >= 5) {
    lastCheck = nowTime;

    
 //  📌 LOG A CADA 10m
  if (nowTime - lastLog >= 360) {
    lastLog = nowTime;
    getLog();
  }
    // Verifica Agenda
    String s = scheduleChk(schedule);
    if (s != "") {
                Serial.println("Agendamento" + s);
      if (s.startsWith("SH")) getLog();   // só loga quando for evento SH
    }   
 }
  
 loopNext = now()+ 60;
 }
 
 

  
