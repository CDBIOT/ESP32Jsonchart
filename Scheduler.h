

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include <TimeLib.h>


extern int8_t cfgTimeZone();
extern byte relayRead();

extern const byte LED_PIN;
extern const byte RELAY_PIN;

// Mapeamento HIGH/LOW do Relé
const byte        RELAY_HIGH                      = HIGH;
const byte        RELAY_LOW                       = LOW;

// Mapeamento HIGH/LOW do LED
const byte        LED_HIGH                        = LOW;
const byte        LED_LOW                         = HIGH;


// Último evento
String            lastEvent;

// Funções auxiliares -----------------------------------
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

void relayWrite(const byte &state) {
  // Grava o estado do relé
  digitalWrite(RELAY_PIN, state);
}


String dateTimeStr(time_t t, int8_t tz = -3) {
  //time_t em seg desde 1970 até hoje.
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




// Scheduller Variables
time_t  schLastCheck            = 0,
        schHighDT,
        schLowDT;

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

String dateTimeNowTZStr() {
  // Retorna time_t como string considerando timezone
  return dateTimeStr(now());
}


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
  time_t nowTZ = schLastCheck + cfgTimeZone() * 3600;
  
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
    pinStatus =  RELAY_HIGH;
    goto process;
  }

  // Verifica desligar em Data/Hora específica - SL yyyy-mm-dd hh:mm
  s = "SL " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = RELAY_LOW;
    goto process;
  }

  // Data/Hora como "dd hh:mm"
  dt = dt.substring(8);

  // Verifica ligar mensal - MH dd hh:mm
  s = "MH " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = RELAY_HIGH;
    goto process;
  }

  // Verifica desligar mensal - ML dd hh:mm
  s = "ML " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = RELAY_LOW;
    goto process;
  }

  // Data/Hora como "d hh:mm"
  dt = String(weekday(nowTZ)) + dt.substring(2);
  
  // Verifica ligar semanal - WH d hh:mm
  s = "WH " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = RELAY_HIGH;
    goto process;
  }

  // Verifica desligar semanal - WL d hh:mm
  s = "WL " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = RELAY_LOW;
    goto process;
  }

  // Data/Hora como "hh:mm"
  dt = dt.substring(2);

  // Verifica ligar diário - DH hh:mm
  s = "DH " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = RELAY_HIGH;
    goto process;
  }

  // Verifica desligar diário - DL hh:mm
  s = "DL " + dt;
  if (sch.indexOf(s) != -1) {
    event = s;
    pinStatus = RELAY_LOW;
    goto process;
  }

  // Verifica ligar intervalado - IH hh:mm
  s = "IH " + hhmmStr(schLastCheck - schHighDT);
  if (sch.indexOf(s) != -1 && relayRead() == RELAY_HIGH) 
  {
    event = s;
    pinStatus = RELAY_LOW;
    goto process;
  }

  // Verifica desligar intervalado - ID hh:mm
  s = "IL " + hhmmStr(schLastCheck - schLowDT);
  if (sch.indexOf(s) != -1 && relayRead() == RELAY_LOW) 
  {
    event = s;
    pinStatus = RELAY_HIGH;
  }

  process:  // Processa evento
  if (event != "" && pinStatus != relayRead()) {
    relayWrite(pinStatus);
    if (pinStatus == RELAY_HIGH)
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

#endif  //SCHEDULER_H
