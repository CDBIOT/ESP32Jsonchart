#include <TimeLib.h>

void scheduleIntervalReset();

String scheduleGet();

String scheduleChk() ;

byte relayRead() {
  // Lê o estado do relé
  //return digitalRead(RELAY_PIN);
}

/*******************************************************************************
* FUNCTIONS
*******************************************************************************/
void hold(const unsigned int &ms) {
  // Non blocking delay
  unsigned long m = millis();
  while (millis() - m < ms) {
    yield();
  }
}

int timeZone() {
  // Return Time Zone config value
  //return int8(EEPROM.read(CFG_TIME_ZONE));
}
/*******************************************************************************
* FUNÇÕES DO SISTEMA DE AGENDA
* 10/2018 - Andre Michelon
* Opções:
*   - Agenda para data/hora específica
*     On (High): SH yyyy-mm-dd hh:mm
*     Off (Low): SL yyyy-mm-dd hh:mm
*   - Mensal
*     On (High): MH dd hh:mm
*     Off (Low): ML dd hh:mm
*   - Semanal
*     On (High): WH d hh:mm
*     Off (Low): WL d hh:mm
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
        
//int8_t   cTimeZone;

time_t timeNTP() {
  // Return time_t from NTP Server
 // if (wifiStatus != WL_CONNECTED) {
    // No WiFi connection
   // return 0;
  //}

  // NTP Server
  const char  NTP_SERVER[]    = "pool.ntp.br";
  // NTP Packet Size
  const byte  NTP_PACKET_SIZE = 48;
  // UDP Port
  const int   UDP_LOCALPORT   = 2390;
  // NTP Packet
  byte        ntp[NTP_PACKET_SIZE];

  memset(ntp, 0, NTP_PACKET_SIZE);
  ntp[ 0] = 0b11100011; // LI, Version, Mode
  ntp[ 1] = 0;          // Stratum, or type of clock
  ntp[ 2] = 6;          // Polling Interval
  ntp[ 3] = 0xEC;       // Peer Clock Precision
  ntp[12] = 49;
  ntp[13] = 0x4E;
  ntp[14] = 49;
  ntp[15] = 52;
  // Get time from server
  WiFiUDP udp;
  udp.begin(UDP_LOCALPORT);
  udp.beginPacket(NTP_SERVER, 123);
  udp.write(ntp, NTP_PACKET_SIZE);
  udp.endPacket();
  //hold(1000);
  unsigned long l;
  if (udp.parsePacket()) {
    // Success
    udp.read(ntp, NTP_PACKET_SIZE);
    l = word(ntp[40], ntp[41]) << 16 | word(ntp[42], ntp[43]);
    l -= 2208988800UL;      // Calculate from 1900 to 1970
    l += -3 * 3600; // Adjust time zone (+- timeZone * 60m * 60s)
   // logStr[logIndex] = dateTimeStr(l) + F(";NTP;Ok");
  } else {
    //Error
   // logStr[logIndex] = dateTimeStr(1) + F(";NTP;ERRO");
    l = 0;
  }
  // Simulate Log
 // Serial.println(logStr[logIndex]);
 // logIndex = (logIndex + 1) % LOG_ENTRIES;
  // Return DateTime
  return l;
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
  time_t nowTZ = schLastCheck  * 3600;
  //time_t nowTZ = schLastCheck + cfgTimeZone() * 3600;
  // Processa Agenda
  event = "";

  // Get DateTime as "yyyy-mm-dd hh:mm" String
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
    pinStatus = LEDPIN; //RELAY_HIGH;
    goto process;
  }

  // Verifica desligar em Data/Hora específica - SL yyyy-mm-dd hh:mm
  s = "SL " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = LEDPIN; //RELAY_LOW;
    goto process;
  }

  // Data/Hora como "dd hh:mm"
  dt = dt.substring(8);

  // Verifica ligar mensal - MH dd hh:mm
  s = "MH " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = LEDPIN; // RELAY_HIGH;
    goto process;
  }

  // Verifica desligar mensal - ML dd hh:mm
  s = "ML " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = LEDPIN; //RELAY_LOW;
    goto process;
  }

  // Data/Hora como "d hh:mm"
  dt = String(weekday(nowTZ)) + dt.substring(2);
  
  // Verifica ligar semanal - WH d hh:mm
  s = "WH " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = LEDPIN; //RELAY_HIGH;
    goto process;
  }

  // Verifica desligar semanal - WL d hh:mm
  s = "WL " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = LEDPIN; //RELAY_LOW;
    goto process;
  }

  // Data/Hora como "hh:mm"
  dt = dt.substring(2);

  // Verifica ligar diário - DH hh:mm
  s = "DH " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = LEDPIN; //RELAY_HIGH;
    goto process;
  }

  // Verifica desligar diário - DL hh:mm
  s = "DL " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = LEDPIN; //RELAY_LOW;
    goto process;
  }

  // Verifica ligar intervalado - IH hh:mm
  s = "IH " + hhmmStr(schLastCheck - schHighDT);
  if (sch.indexOf(s) != -1 && relayRead() == LEDPIN; //RELAY_HIGH) 
  {
    event = s;
    pinStatus = LEDPIN; //RELAY_LOW;
    goto process;
  }

  // Verifica desligar intervalado - ID hh:mm
  s = "IL " + hhmmStr(schLastCheck - schLowDT);
  
  if (sch.indexOf(s) != -1  == LEDPIN; //RELAY_LOW)
  //if (sch.indexOf(s) != -1 && relayRead() == LEDPIN; //RELAY_LOW)
  {
    event = s;
    pinStatus = LEDPIN; //RELAY_HIGH;
  }
}
  process:  // Processa evento
  if (event != "" && pinStatus != relayRead()) {
    relayWrite(pinStatus);
    if (pinStatus == LEDPIN; //RELAY_HIGH) 
    {
      // Registra Data/Hora ligado
      schHighDT = schLastCheck;
    } else {
      // Registra Data/Hora desligado
      schLowDT = schLastCheck;
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
