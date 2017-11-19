#include <SPI.h>
#include <SD.h>

const int chipSelect = 4;
const char ETX = '#'; // Char(3);
byte packetBuffer[50];
float temper[10];
String temperStr;

String inputStr = "";
boolean isData = false;
boolean isGet = false;
boolean isFilename = false;
String filename = "";
char empfaenger = "";
File file;
char input ;
String in;
unsigned long timeOut = 0;
int index = 0;
long quad = -738000;
unsigned long aTime = 0;
String more = "";
File ff;
File root;


void setup() {
  Serial.begin(38400);
  if (!SD.begin(chipSelect)) {
    return;
  }

  timeOut = millis();
}

void loop() {

  if (millis() - timeOut > 10000) {
    isGet = false;
    isData = false;
    isFilename = false;
    filename = "";
    timeOut = millis();
  }
  safeQuadData();
}

void safeQuadData() {
  if ((millis() - quad) > 900000) {
    String d = unixtToString(aTime);
    d = d.substring(0, 10);
    d.replace("-", "");
    String f = "/dat/" + String(d) + ".TXT";
    file = SD.open(f, FILE_WRITE);
    file.print(unixtToString(aTime));
    file.print(",");
    for (int i = 0; i < 10; i++) {
      if (i < 9) {
        file.print( String(temper[i], 1) + ",");
      } else {
        file.println(  String(temper[i], 1) );
      }
    }
    quad = millis();
    timeOut = millis();
    file.close();
  }
}

void serialEvent() {
  timeOut = millis();
  if (Serial.available() > 0 ) {
    input = Serial.read();
    if ( !(isGet or isData )) {
      inputStr.concat(input);
      if (inputStr.length() > 6) {
        inputStr.remove(0, 1);
      }
      if (inputStr.endsWith("GET ") ) {
        isGet = true;
      }
      if (inputStr.endsWith("DATA,") ) {
        isData = true;
      }
      if (inputStr.endsWith("NEXT") ) {
        doSendData();
      }
    } else {
      if (isGet )isGet = doGet(input);
      if (isData)isData = doData(input);
    }
  }
}


boolean doSendData() {
  boolean fileclose = false;
  long sizeToSend = 0;
  if (more.length() > 0) {
    sizeToSend = more.length();
  } else {
    sizeToSend = file.size() - file.position();

    if (sizeToSend >= 50) {
      sizeToSend = 50;
    } else {
      fileclose = true;
    }
  }
  Serial.print("#" + String(sizeToSend) + "PUT");
  timeOut = millis();
  int i = 0;
  if (more.length() > 0) {
    Serial.print(more);
    more = "";
  } else {
    while (millis() - timeOut < 10000) {

      i = file.read(packetBuffer, sizeToSend);
      Serial.write(packetBuffer, i);
      sizeToSend = sizeToSend - i;
      if (sizeToSend == 0) {
        break;
      }
    }
    if (fileclose) {
      file.close();
    }
  }
  return   false;
}

boolean doGet(char in) {
  if (in != ETX ) {
    filename = filename + in;
  } else  if (SD.exists(filename)) {
    file = SD.open(filename, FILE_READ);
    doSendData();
    filename = "";
  }  else if (filename.endsWith( "/" ) ) {
    file = SD.open("/index.htm", FILE_READ);
    doSendData();
    filename = "";
  } else if (filename.endsWith( "/?" ) ) {
    String send = "";
    for (int i = 0; i < 6; i++) {
      if (i < 5) {
        send = send + String(intToTemp(analogRead(i)), 1) + ",";
      } else {
        send = send + String(intToTemp(analogRead(i)), 1)  ;
      }
    }
    Serial.print("#" + String(send.length()) + "PUT");
    Serial.print(send);
    filename = "";
  }  else if (filename.endsWith( "?" ) ) {
    String pin = filename.substring(filename.length() - 2, filename.length() - 1);
    String temp = String(intToTemp(analogRead(pin.toInt())));
    String send = String(temp);
    Serial.print("#" + String(send.length()) + "PUT");
    Serial.print(send.substring(0, 50));
    filename = "";
  } else if (filename.endsWith( "DIR" ) ) {
    if (SD.exists("LIST.TXT")) {
      SD.remove("LIST.TXT");
    }
    ff = SD.open("LIST.TXT", FILE_WRITE);
    root = SD.open("DAT/");
    while (true) {
      File entry =  root.openNextFile();
      if (! entry) {
        // no more files
        break;
      }
      ff.print(entry.name());
      ff.print(",");
      entry.close();
    }
    ff.close();
    delay(10);
    file = SD.open("LIST.TXT", FILE_READ);
    doSendData();
    filename = "";
  }
  return (in != ETX );
}

boolean doData(char in) {
  if (in != ETX) {
    if (isFilename ) {
      file.write(in);
      temperStr.concat(in);
      if (temperStr.length() > 8) {
        temperStr.remove(0, 1);
      }
      if (temperStr.endsWith(",")) {
        temperStr.remove(",");
        temper[index] = temperStr.toFloat();
        temperStr = "";
        index++;
      }
    } else {
      if (in == ',') {
        aTime = filename.toInt();
        String d = unixtToString(aTime);
        d = d.substring(0, 10);
        d.replace("-", "");
        String f = "/data/" + String(d) + ".TXT";
        file = SD.open(f, FILE_WRITE);
        isFilename = true;
        file.print(unixtToString(aTime ) +  in);
      }
      filename = filename + in;
    }
  } else {
    for (int analogPin = 0; analogPin < 6; analogPin++) {
      int sensor = analogRead(analogPin);
      file.print(intToTemp(sensor));
      if (analogPin < 5) {
        file.print(",");
      } else {
        file.println("");
      }
      temper[index] = intToTemp(sensor);
      index++;
    }
    index = 0;
    isFilename = false;
    filename = "";
    file.close();
  }
  return  in != ETX;
}

float intToTemp(int x) {
  x = 1023 - x;
  float result;
  result = -9.01865E-13 * x * x * x * x * x + 2.47090E-09 * x * x * x * x - 2.74511E-6 * x * x * x + 1.55555E-3 * x * x - 5.40551E-1 * x + 1.22271E2;
  return result;
}

String unixtToString(uint32_t t) {
  t = isSommer(t) ? t + 2 * 3600 : t + 3600;
  int wochentag = ((t + 345600) % (604800)) / 86400;
  int data [6] = {0, 0, 0, 0, 0, 0};
  data[5] = t % 60;
  t = t / 60;
  data[4] = t % 60 ;
  t = t / 60;
  data[3] = t % 24;
  int h = t % 24;
  t = t / 24;

  unsigned yr = t / 365 + 1970;
  t = t % 365; // Year
  unsigned ly;
  for (ly = 1972; ly < yr; ly += 4) {
    if (!(ly % 100) && (ly % 400)) continue;
    --t;
  }
  if (t < 0) t += 365, ++yr;
  static uint8_t const dm[2][12] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
  };
  int day = t;
  int month = 0;
  ly = (yr == ly) ? 1 : 0;
  while (day >= dm[ly][month]) day -= dm[ly][month++];
  data[2] = day + 1;
  data[1] = month + 1;
  data[0] = yr;
  return String(data[0]) + "-"
         + (data[1] <= 9 ? "0" + String(data[1]) : String(data[1])) + "-"
         + (data[2] <= 9 ? "0" + String(data[2]) : String(data[2])) + "T"
         + (data[3] <= 9 ? "0" + String(data[3]) : String(data[3])) + ":"
         + (data[4] <= 9 ? "0" + String(data[4]) : String(data[4])) + ":"
         + (data[5] <= 9 ? "0" + String(data[5]) : String(data[5]));
}
boolean isSommer(uint32_t t) {
  int wochentag = ((t + 345600) % (604800)) / 86400;
  t = t / 360;
  int h = t % 24;
  t = t / 24;
  unsigned yr = t / 365 + 1970;
  t = t % 365; // Year
  unsigned ly;
  for (ly = 1972; ly < yr; ly += 4) {
    if (!(ly % 100) && (ly % 400)) continue;
    --t;
  }
  if (t < 0) t += 365, ++yr;
  static uint8_t const dm[2][12] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
  };
  int day = t;
  int month = 0;
  ly = (yr == ly) ? 1 : 0;
  while (day >= dm[ly][month]) day -= dm[ly][month++];

  boolean sommer = false;
  if (month > 2 && month < 9)sommer=true ;
  if ((month == 2) && (31 - day + wochentag < 7) && (wochentag > 0))sommer = true; 
  if ((month == 2) && (31 - day + wochentag < 7) && (wochentag == 0) && (h > 0 ))sommer = true;
  if ((month == 9) && (31 - day + wochentag > 7) )sommer = true;
  if ((month == 9) && (31 - day + wochentag < 7) && (wochentag == 0) && (h < 1 )) sommer = true;
  return sommer;
}
