

#include <WiFiLink.h>
#include <WiFiUdp.h>
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[50]; //buffer to hold incoming and outgoing packets
char ssid[] = "XXXXXXXXXX";      //  your network SSID (name)
char pass[] = "XXXXXXXXXX";   // your network passwo
int keyIndex = 0;                 // your network key Index number (needed only for WEP)
String url = "";
unsigned long timeOut = 0;
unsigned long sTime = 0;
long timeSendData = -60000;

int status = WL_IDLE_STATUS;
WiFiUDP Udp;
WiFiServer server(80);
WiFiClient client;



void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  delay(2000);
  Serial.begin(38400);      // initialize serial communication
  delay(1000);


  //Check if communication with the wifi module has been established
  if (WiFi.status() == WL_NO_WIFI_MODULE_COMM) {
    Serial.println("Communication with WiFi module not established.");
    while (true); // don't continue:
  }

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Connect to Network : ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(5000);
  }
  Serial.print("UnixTime  :");
  sTime = setTime();
  while (sTime < 1504716686) {
    Serial.println(getTime());
    sTime = setTime();
  }
  Serial.println(getTime());
  Udp.stop();
  digitalWrite(2, LOW);
  delay(1000);
  digitalWrite(2, HIGH);
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status
  timeOut = millis();
}


void loop() {
  dooWeb();
  doSendData();
}

void doSendData() {
  if ((millis() - timeSendData) > 60000) {
    timeSendData = millis() ;
    Serial.print("DATA,");
    Serial.print(getTime());
    Serial.print(",");
    for (int analogPin = 0; analogPin < 4; analogPin++) {
      int sensor = analogRead(analogPin);
      Serial.print(intToTemp(sensor));
      Serial.print(",");
    }
    Serial.print("#");
  }
}

void dooWeb() {
  WiFiClient client = server.available();   // listen for incoming clients
  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        // Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Connection: close");
            client.println("Accept-Ranges: bytes");
            client.print("Content-type:");
            if (url.endsWith("jpg") ) client.println("image/jpg");
            else if (url.endsWith("ico") ) client.println("image/ico");
            else if (url.endsWith("png") ) client.println("image/png");
            else if (url.endsWith("svg") ) client.println("image/svg+xml");
            else client.println("text/html");
            client.println();
            if (url.endsWith("/?")) {
              for (int analogPin = 0; analogPin < 4; analogPin++) {
                int sensor = analogRead(analogPin);
                client.print(String(intToTemp(sensor), 1));
                client.print(",");
              }
            }
            // the content of the HTTP response follows the header:
            while (Serial.available() > 0) Serial.read();
            Serial.println(url + "#");
            timeOut = millis();
            String buffer = "";
            size_t cnt = 50;
            while ((millis() - timeOut) < 10000) {
              if (Serial.available() > 0) {
                timeOut = millis();
                char c = Serial.read();
                buffer.concat(c);
                if (buffer.length() > 8)buffer.remove(0, 1);
                if (buffer.endsWith("PUT")) {
                  buffer.replace("PUT", "");
                  buffer.remove(0, buffer.indexOf("#") + 1);
                  cnt = buffer.toInt();
                  size_t i = 0;
                  while ((millis() - timeOut) < 10000) {
                    if (Serial.available() >= cnt) {
                      i = Serial.readBytes(packetBuffer, cnt);
                      int clst = client.status();
                      if (clst == 4) {
                        client.write(packetBuffer, i);
                        if (i == 50) {
                          Serial.print("NEXT");
                          timeOut = millis();
                        }
                        break;
                      } else {
                        Serial.println("client.status :" + String(clst));
                        while (Serial.available() > 0) {
                          Serial.read();
                        }
                        client.stop();
                        client = false;
                        delay(100);
                        return;
                      }
                    }
                  }
                }
              }
              if (cnt != 50) break;
            }

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          if (currentLine.startsWith("GET") or currentLine.length() < 4 ) {
            currentLine += c;      // add it to the end of the currentLine
          }
        }

        if (currentLine.endsWith("HTTP/1.1")) {
          url =  currentLine;
          url.replace(" HTTP/1.1", "");
        }

      }
    }
    client.stop();
    Serial.println("client disonnected");
    delay(10);
  }
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
}

unsigned long getTime() {
  if (sTime == 0) {
    sTime = getTime();
  }
  return sTime + millis() / 1000;
}

float intToTemp(int x) {
  x = 1023 - x;
  float result;
  result = -9.01865E-13 * x * x * x * x * x + 2.47090E-09 * x * x * x * x - 2.74511E-6 * x * x * x + 1.55555E-3 * x * x - 5.40551E-1 * x + 1.22271E2;
  return result;
}

unsigned long setTime() {
  unsigned int localPort = 2390;      // local port to listen for UDP packets

  Udp.begin(localPort);

  sendNTPpacket( packetBuffer, NTP_PACKET_SIZE); // send an NTP packet to a time server
  delay(500);
  if ( Udp.parsePacket() ) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    Serial.write(packetBuffer, NTP_PACKET_SIZE);
    Serial.println();

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    return (epoch );
  }
}
unsigned long sendNTPpacket(  uint8_t *packetBuffer, const int NTP_PACKET_SIZE) {


  //Serial.println("1");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  //Serial.println("2");
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  //Serial.println("3");

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  char adr[] = "ch.pool.ntp.org";
  Udp.beginPacket(adr, 123); //NTP requests are to port 123
  //Serial.println("4");
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  //Serial.println("5");
  Udp.endPacket();
  //Serial.println("6");
}
