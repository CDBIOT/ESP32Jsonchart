
#include <TimeLib.h>

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
void readLog();
void sendLogMQTT();

void handleLog_view();
void handleJavaScript();
void handleFileRead();
void handleListFiles();
void testRead();
void deleteLog();
void reboot();
void hold();

//variables
int sensorPin = 34;

// Directory Max Number of Files
const byte      DIRECTORY_MAX_FILES             = 20;

extern String s;
extern int valor;
//extern float t;

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
     s.replace(F("#serial#") , hexStr(ESP.getEfuseMac()));
    //s.replace(F("#software#") , softwareStr());
    s.replace(F("#sysIP#")    , ipStr(WiFi.status() == WL_CONNECTED ? WiFi.localIP() : WiFi.softAPIP()));
    s.replace(F("#clientIP#") , ipStr(server.client().remoteIP()));
    
    // Envia dados
    server.send(200, F("text/html"), s);
    log("Home - Cliente: " + ipStr(server.client().remoteIP()) + (server.uri() != "/" ? " [" + server.uri() + "]" : ""));
  } else {
    server.send(500, F("text/plain"), F("Home - ERROR 500"));
    log(F("Home - ERRO lendo arquivo"));
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
    s.replace(F("#id#")     , id);
    s.replace(F("#ledOn#")  ,  ledOn ? " checked" : "");
    s.replace(F("#ledOff#") , !ledOn ? " checked" : "");
    s.replace(F("#ssid#")   , ssid);
    s.replace(F("#pw#")   , pw);
    
//     Serial.println("scan start");
//
//    // WiFi.scanNetworks will return the number of networks found
//    int n = WiFi.scanNetworks();
//    Serial.println("scan done");
//    if (n == 0) {
//        Serial.println("no networks found");
//    } else {
//        Serial.print(n);
//        Serial.println(" networks found");
//        for (int i = 0; i < n; ++i) {
//            // Print SSID and RSSI for each network found
//            Serial.print(i + 1);
//            Serial.print(": ");
//            Serial.print(WiFi.SSID(i));
//            Serial.print(" (");
//            Serial.print(WiFi.RSSI(i));
//            Serial.print(")");
//            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
//            delay(10);
//        }
//    }
//    Serial.println("");
//
//    // Wait a bit before scanning again
//    delay(5000);

    
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
  if (server.args() == 4) {

    String s;
    // Grava id
    s = server.arg("id");
    s.trim();
    if (s == "") {
      s = deviceID();
   }
   strlcpy(id, s.c_str(), sizeof(id));

    // Grava ssid
    s = server.arg("ssid");
    s.trim();
    strlcpy(ssid, s.c_str(), sizeof(ssid));
    Serial.print(ssid);
    
    // Grava pw
    s = server.arg("pw");
    s.trim();
    if (s != "") {
      // Atualiza senha, se informada
      strlcpy(pw, s.c_str(), sizeof(pw));
    }
    Serial.print(pw);

    // Grava ledOn
    ledOn = server.arg("led").toInt();
    // Atualiza LED
    ledSet();
    
configSave();

    // Grava configuração
    if (configSave()) {
      server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Configuração salva.');history.back()</script></html>"));
      log("ConfigSave - Cliente: " + ipStr(server.client().remoteIP()));
    } else {
     server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Falha salvando configuração.');history.back()</script></html>"));
      log(F("ConfigSave - ERRO salvando Config"));
    }
  } else {
    server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Erro de parâmetros.');history.back()</script></html>"));
  log(F("ConfigSave - ERRO de parametros"));
  }
}

void handleReconfig() {
  // Reinicia Config
  configReset();

  // Atualiza LED
  ledSet();

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

//    String s;
////    // Grava id
//    s = server.arg("id");
//    s.trim();
//    if (s == "") {
//      s = deviceID();
//   }

     
    // Envia dados
    server.send(200, F("text/html"), s);
//    log("JS - Cliente: " + ipStr(server.client().remoteIP()));
  } else {
    server.send(500, F("text/plain"), F("Home - ERROR 500"));
   // log(F("JS - ERRO lendo arquivo"));

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


void handleTemp()
{
File file = SPIFFS.open(F("/Temperatura.html"), "r");

 if (file) {
    file.setTimeout(100);
    String s = file.readString();
    file.close();
    
    int valor = analogRead(34);
    float t = ((valor*250)/1023);

  float temperatura = (valor * 250.0) / 1023.0;

  File file = SPIFFS.open("/Log/log.txt", FILE_APPEND);
  if (file) {
    file.printf("%s;%.2f\n", dateTimeStr(now()).c_str(), temperatura);
    file.close();
    Serial.printf("Logado: %s; %.2f\n", dateTimeStr(now()).c_str(), temperatura);
  } else {
    Serial.println("Erro ao abrir Log/log.txt");
  }
 
      Serial.print(s);
    // server.send(200, F("text/html"), s);
  }

String json;
int i;

  StaticJsonDocument<JSON_SIZE> doc;
  JsonObject Dados  = doc.to<JsonObject>();
  
  serializeJson(doc, json);
     
    server.send(200, F("application/json"), json);
//String json = "{\"temperature\":"+String(t)+"}";

}


void handleListFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  String json = "[";

  while (file) {
    if (!file.isDirectory()) {
      json += "{";
      json += "\"name\":\"" + String(file.name()) + "\",";
      json += "\"size\":" + String(file.size());
      json += "},";
    }
    file = root.openNextFile();
  }

  if (json.endsWith(",")) json.remove(json.length()-1);
  json += "]";

  server.send(200, "application/json", json);
}

void  testRead(){
  File file = SPIFFS.open("/1969-12-31.txt", FILE_READ);
  if(!file){
    Serial.println("ERRO abrindo arquivo");
    return;
  }
  Serial.println("Conteudo:");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();

  String arquivo = file.readString();
  server.send(200, "application/json",arquivo );
  
//  File arquivo = SPIFFS.open("/1969-12-31.txt","r");
//
//  while(arquivo.available()){
//    Serial.write(arquivo.read());
//  }
//  arquivo.close();
  
  //server.send(200, "application/json", arquivo);


  
}


void handleFileRead() {
  
  StaticJsonDocument<JSON_SIZE> doc;
  JsonObject arquivo  = doc.to<JsonObject>();

  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Arquivo não informado");
    Serial.printf("Erro arquivo não informado");
    return;
  }
  
//  
//  File arquivo = SPIFFS.open("/1969-12-31.txt","r");
//
//  while(arquivo.available()){
//    Serial.write(arquivo.read());
//  }
//  arquivo.close();
  
  //server.send(200, "application/json", arquivo);

  String fileName = server.arg("file");
  //Serial.printf("Arquivo: %s tamanho: %d bytes\n", file.name(), file.size());

  if(!fileName.startsWith("/")){
    fileName = "/" + fileName;
  }
  
  Serial.printf("Abrindo arquivo: %s \n", fileName);
  
  
  File file = SPIFFS.open(fileName + ".txt", FILE_READ);
  if (!file) {
    Serial.println("Erro abrindo arquivo");
    server.send(500, "text/plain", "Erro abrindo arquivo");
    return;
  }


    
  //String json  = "[";
  String content = file.readString();
//  
//  bool first = true;
//  
//while (file.available()) {
//
//    String line = file.readStringUntil('\n');
//    line.trim();
//    
//    Serial.println(line);
//    if (line.length() == 0) continue;
//
//    if (!first) json += ",";
//    json += line;
//    first = false;
//  }
//  
 // json += "]";
   file.println();
   file.close();
  //Serial.println(content);
  //serializeJson(arquivo, json);

 // server.send(200, "application/json", json);
   
//  String content = "";
//
//  while(file.available()){
//    char c = file.read();
//    content += c;
//  }

  //File f = SPIFFS.open(fileName, "r");
//
//  if (!f) {
//    server.send(404,"text/plain","Not found");
//    return;
//  }
String json = "["  + content + "]";
   Serial.println(json);
   json.replace("}\n{","},{");
    //server.send(200, "text/plain",content);
    server.send(200, "text/plain",json);
  //server.streamFile(json, "application/json");
      
}


void handleLogView() {
  
File file = SPIFFS.open(F("/log.htm"), "r");

 if (file) {
    file.setTimeout(100);
    String s = file.readString();
    file.close();
     
      //Serial.print(s);
   server.send(200, F("text/html"), s);
  }
}



void handleLogReset() {
  // Reinicia Log
  //ledWrite(LED_HIGH);
  //if (chkWebAuth()) {
    // Deleta log
    //logDelete();
    // Envia dados
    server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Log reiniciado.');window.location = 'log';</script></html>"));
  //  log(F("WebLogReset"), "Cliente: " + ipStr(server.client().remoteIP()));
 // }
  //ledWrite(LED_LOW);
}


void deleteLog() {
  // Deleta log em memória
  for (byte bFn = 0; bFn < LOG_ENTRIES; bFn++) {
  //  logStr[bFn] = "";
  }
  logIndex = 0;
  
// --- Excluindo arquivo ---
  Serial.println("\nExcluindo Arquivo ----------------------------");
  if (SPIFFS.remove("/log.txt")) {
    Serial.println("Arquivo '/log.txt' excluído");
  } else {
    Serial.println("Exclusão '/log.txt' falhou");
  }
}

//
//
//void log(const String &type, const String &msg) {
//  // Generate log in memory
//  logStr[logIndex] = dateTimeStr(now()) + ";" + type + ";" + msg;
//  Serial.println(logStr[logIndex]);
//  // Increment log index;
//  logIndex = (logIndex + 1) % LOG_ENTRIES;
//}

void reboot() {
  // Reinicia o dispositivo
 // ledWrite(led0);
  delay(2000);
  ESP.restart();
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

void sortArray(String a[], String &s) {
  // Sort a String Array
  boolean flFn = true;
  String sFn;
  while (flFn) {
    flFn = false;
    byte bFn = 0;
    while(a[bFn + 1] != "") {
      if(a[bFn] > a[bFn + 1]) {
        sFn = a[bFn + 1];
        a[bFn + 1] = a[bFn];
        a[bFn] = sFn;
        flFn = true;
      }
      bFn++;
    }
    bFn = 0;
    s = "";
    while (a[bFn] != "") {
      s += a[bFn];
      bFn++;
    }
  }
}

String fsSpaceStr() {
  // Return information about FileSystem space
  size_t total = SPIFFS.totalBytes();
  size_t used = SPIFFS.usedBytes();
  size_t freeSpace = total - used;
  
  return  "Total: " +
            String((total) / 1048576.0, 2) + "mb<br>" +
          "Usado: " +
            String(used / 1048576.0, 2) + "mb<br>" +
          "Disponível: " +
            String((freeSpace) / 1048576.0, 2) + "mb<br>";
}
