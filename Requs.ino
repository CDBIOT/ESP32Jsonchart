
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
void handleLog_view();
void handleJavaScript();
void logDelete();
void reboot();

//variables
int sensorPin = A0;

// Max log entries
const byte      LOG_ENTRIES                     = 50;

// System Log
String            logStr[LOG_ENTRIES];
byte              logIndex      = 0;


// Funções auxiliares -----------------------------------
String dateTimeStr(time_t t, int8_t tz = 0) {
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
   // strlcpy(ssid, s.c_str(), sizeof(ssid));

    // Grava pw
    s = server.arg("pw");
    s.trim();
    if (s != "") {
      // Atualiza senha, se informada
    //  strlcpy(pw, s.c_str(), sizeof(pw));
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

void handleLogGet() {
  // Download do Log
 // ledWrite(LED_HIGH);
  //if (chkWebAuth()) {
    byte bFn;
    String s = deviceID() +
               F(" - Log\r\nData/Hora;Tipo;Mensagem\r\n");
    for (bFn = logIndex; bFn < LOG_ENTRIES; bFn++) {
      if (logStr[bFn] != "") {
        s += logStr[bFn] + F("\r\n");
 //     }
    }
    for (bFn = 0; bFn < logIndex; bFn++) {
      if (logStr[bFn] != "") {
        s += logStr[bFn] + F("\r\n");
      }
    }
    server.sendHeader(F("Content-Disposition"), "filename=\"" +
                      deviceID() + F("Log.csv\""));
    server.send(200, F("text/csv"), s);
//    log(F("WebLogGet"), "Client: " + ipStr(server.client().remoteIP()));
  }
  //ledWrite(LED_LOW);
}

void handleLogReset() {
  // Reinicia Log
  //ledWrite(LED_HIGH);
  //if (chkWebAuth()) {
    // Deleta log
    logDelete();
    // Envia dados
    server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Log reiniciado.');window.location = 'log';</script></html>"));
  //  log(F("WebLogReset"), "Cliente: " + ipStr(server.client().remoteIP()));
 // }
  //ledWrite(LED_LOW);
}


void logDelete() {
  // Deleta log em memória
  for (byte bFn = 0; bFn < LOG_ENTRIES; bFn++) {
    logStr[bFn] = "";
  }
  logIndex = 0;
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
  hold(2000);
  ESP.restart();
}
